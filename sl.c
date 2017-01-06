#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dlfcn.h>

#include <portaudio.h>

#include "paudio.h"
#include "listener.h"
#include "error.h"
#include "utils.h"
#include "lib.h"

/* 1/8th of the max. amplitude */
#define MAX_CHECK_LEVEL 4096
#define SAMPLE_RATE 44100

int n_filter_lib = 0;                                   /* filters */
filter_lib filters[MAX_N_LIBRARIES];

void init_curses(void)
{
	initscr();
	keypad(stdscr, TRUE);
	cbreak();
	intrflush(stdscr, FALSE);
	leaveok(stdscr, FALSE);
	noecho();
	nonl();
	refresh();
	nodelay(stdscr, FALSE);
	meta(stdscr, TRUE);	/* enable 8-bit input */
#if 0
	raw();	/* to be able to catch ctrl+c */
#endif
}

int main(int argc, char *argv[])
{
	WINDOW *win;
	PaStream *pcm_handle;
	int level = 0, prevlevel = -1;
	int detect_level = 0;
	char *configfile = CONFIGFILE;
	int loop;
	FILE *fh;
	short *buffer;
	int buffer_size, rc;
	int rate = SAMPLE_RATE, channels = 1;

	/* load configfile */
        fh = fopen(configfile, "rb");
	if (!fh)
	{
		fh = fopen("listener.conf", "rb");
		if (fh)
			printf("Using listener.conf from current directory\n");
	}
        if (!fh)
                error_exit("error opening configfile %s\n", configfile);
                                                                                                                             
        for(;;)
        {
                char *i_is, *cmd, *par = NULL;
                char *line = read_line(fh);
                if (!line)
                        break;
                                                                                                                             
                /* find '=' */
                i_is = strchr(line, '=');
                if (i_is)
                {
                        /* find start of parameter(s) */
                        par = i_is+1;
                        while ((*par) == ' ') par++;
                }
                cmd = get_token(line);

                if (strcasecmp(cmd, "filter") == 0)
                {
                        if (n_filter_lib < MAX_N_LIBRARIES)
                        {
				char *dummy = strchr(par, ' ');

				dlerror();

                                filters[n_filter_lib].library = dlopen(par, RTLD_NOW);
                                if (!filters[n_filter_lib].library)
                                        error_exit("Failed to load filter library %s, reason: %s\n", par, dlerror());
                                                                                                                             
                                filters[n_filter_lib].init_library = dlsym(filters[n_filter_lib].library, "init_library");
                                if (!filters[n_filter_lib].init_library)
                                        error_exit("The filter library %s is missing the 'init_library' function");
                                                                                                                             
                                filters[n_filter_lib].do_filter    = dlsym(filters[n_filter_lib].library, "do_filter");
                                if (!filters[n_filter_lib].do_filter)
                                        error_exit("The filter library %s is missing the 'do_library' function");

                                filters[n_filter_lib].par = NULL;
                                dummy = strchr(par, ' ');
				if (dummy)
                                	filters[n_filter_lib].par = *(dummy+1) ? strdup(dummy+1) : NULL;

				n_filter_lib++;
                        }
                        else
                                error_exit("Too many filters defined, only %d possible.\n", MAX_N_LIBRARIES);
                }

                free(cmd);
                free(line);
        }
        fclose(fh);

	/* open audio-device */
	pcm_handle = paudio_init (&rate, channels);

	/* start curses library */
	init_curses();
	if (COLS < 80 || LINES < 24)
		error_exit("Min. terminal size is 80x24!\n");

	start_color();

	/* create window with a border */
	win = subwin(stdscr, 24, 80, 0, 0);
	if (!win)
		error_exit("Failed to create window!\n");

	if (wborder(win, 0, 0, 0, 0, 0, 0, 0, 0) == ERR)
		error_exit("wborder failed\n");

	/* display some info */
	wattron(win, A_REVERSE);
	mvwprintw(win, 2, 2, "Setup listener");
	wattroff(win, A_REVERSE);
	mvwprintw(win, 4, 2, "With this program you can define at what soundlevel");
	mvwprintw(win, 5, 2, "the program listener should start recording.");

	wattron(win, A_REVERSE);
	mvwprintw(win, 8, 1, " ");
	mvwprintw(win, 8, 78, " ");
	wattroff(win, A_REVERSE);

	mvwprintw(win, 13, 2, "What                    | Configurationfile parameter");
	mvwprintw(win, 14, 2, "------------------------+----------------------------");
	mvwprintw(win, 15, 2, "Current detection level | detect_level");
	mvwprintw(win, 16, 2, "Number of triggers      | min_duration");
	mvwprintw(win, 18, 2, "Number of filters loaded: %d", n_filter_lib);

	init_pair(1, COLOR_YELLOW, COLOR_BLACK);
	wattron(win, COLOR_PAIR(1));
	mvwprintw(win, 6, 2, "Current detection level:");
	mvwprintw(win, 11, 2, "Number of triggers:");
	wattroff(win, COLOR_PAIR(1));
	mvwprintw(win, 9, 2, "Current average level:");

	mvwprintw(win, 22, 42, "(C) 2003-2010 folkert@vanheusden.com");

        /* initialize filters */
        for(loop=0; loop<n_filter_lib; loop++)
        {
                filters[loop].lib_data = filters[loop].init_library(sizeof(short), 1, rate, filters[loop].par);
        }

	buffer_size = rate * sizeof(short);
	buffer = (short *) malloc(buffer_size);
	if (!buffer)
		error_exit("out of memory!");

	for(;;)
	{
		int c;
		struct timeval tv;
		fd_set rfds;
		/* right now I'm not sure if a short is always 2 bytes. check this on your
		 * platform
		 */
		long unsigned int tot_level = 0; /* for calculating average soundlevel */
		int max_level = 0, min_level = 65535;
		int n_threshold = 0;	/* number of samples with level above threshold */

		/* print something which indicates the current recording level */
		if (prevlevel != level)
		{
			if (prevlevel != -1)
				mvwprintw(win, 8, 2 + prevlevel, " ");

			prevlevel = level;
		}
		mvwprintw(win, 8, 2 + level, "|");
		wnoutrefresh(win);
		doupdate();

		/* check audio-level */
		/* read data */
		paudio_get_1s (pcm_handle, buffer, rate, channels);

		/* init structure which makes select() wait for one tenth of a second */
	sel:
		tv.tv_sec = 0;
		tv.tv_usec = 1;
		/* init structure which makes select() wait for keypresses */
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);

		/* wait */
		rc = select(1, &rfds, NULL, NULL, &tv);
		if (rc == -1)
			error_exit("select()-call failed\n");

		/* key pressed? */
		if (rc > 0 && FD_ISSET(0, &rfds))
		{
			c = getch();

			if (c == KEY_LEFT && level > 0)
			{
				level--;
			}
			else if (c == KEY_RIGHT && level < (76-1))
			{
				level++;
			}
			else if (c == KEY_ENTER || c == 13 || toupper(c) == 'Q' || toupper(c) == 'X')
			{
				break;
			}
			else
			{
				flash();
			}

			detect_level = (level * MAX_CHECK_LEVEL) / 76;
			mvwprintw(win, 6, 27, "%d     ", detect_level);
			goto sel;
		}

		/* do filters (if any) */
		/* do filtering AFTER writing sample to disk */
		for(loop=0; loop<n_filter_lib; loop++)
		{
			filters[loop].do_filter(filters[loop].lib_data, (void *)buffer, rate);
		}

		/* check data */
		for(loop=0; loop<(rate * channels); loop++)
		{
			int sample = abs(buffer[loop]);

			if (sample > detect_level)
				n_threshold++;

			tot_level += sample;
			min_level = min(min_level, sample);
			max_level = max(max_level, sample);
		}

		if (n_threshold)
		{
			wattron(win, A_REVERSE);
			mvwprintw(win, 7, 2, "-SOUND!-");
			wattroff(win, A_REVERSE);
			waddch(win, ' ');
		}
		else
		{
			mvwprintw(win, 7, 2, "<silence>");
		}

		mvwprintw(win, 9, 25, "%d     ", tot_level / (rate * channels));
		mvwprintw(win, 10, 2, "Level min: %d, max: %d          ", min_level, max_level);
		mvwprintw(win, 11, 22, "%d     ", max(1, n_threshold));

		/* redraw screen */
		wnoutrefresh(win);
		doupdate();
	}

	delwin(win);
	endwin();

	return 0;
}
