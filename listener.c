#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <time.h>
#include <syslog.h>
#include <string.h>
#include <dlfcn.h>
#include <math.h>

extern char *optarg;

#include <sndfile.h>

#include "alsa.h"
#include "listener.h"
#include "error.h"
#include "utils.h"
#include "lib.h"

char *wav_path = NULL; 					/* path for wav-files */
char *fname_template = NULL;				/* template for filename */
char *configfile = NULL;				/* configurationfile */

int detect_level = 754;					/* threshold for sound-detection */
double rec_silence = 1;					/* number of seconds to keep on reading when silence is detected */
double min_duration = 1;				/* minimum length of a recording */
double max_duration = -1;				/* max. duration IN SECONDS(!) of one file */
int min_triggers = 2;					/* how many times the signal should be above the trigger level */
int compression = SF_FORMAT_PCM_16;			/* default is no compression */
int format = SF_FORMAT_WAV;				/* default is WAV output */
char *exec = NULL;					/* what to execute after recording */
char *on_event_start = NULL;				/* what to exec when the recording starts */
char amplify = 1;					/* default amplify is on */
double start_amplify = 1.0;				/* start factor */
double max_amplify = 2.0;				/* maximum amplify factor */
char do_not_fork = 0;					/* wether to fork into the background or not */
int pr_n_seconds = 2;					/* number of seconds to record before the sound starts */
int silent = 0;						/* be silent on stdout? */
char fixed_ampl_fact = 0;				/* use fixed amplification factor? */
char *pidfile = NULL;					/* file to write the pid into */
int one_shot = 0;					/* exit after 1 recording */
char *output_pipe = NULL;				/* process to output recorded audio to (bladeenc/ogg_encoder/etc.) */

short **pr_buffers;					/* buffers for recording before the sound start */

int n_filter_lib = 0;					/* filters */
filter_lib filter[MAX_N_LIBRARIES];
char safe_after_filter = 0;

char do_exit = 0;

void gettime(double *dst)
{
	struct timezone tz;
	struct timeval tv;

	if (gettimeofday(&tv, &tz) == -1)
	{
		syslog(LOG_CRIT, "error obtaining timestamp: %m");
		exit(1);
	}

	*dst = ((double)tv.tv_sec) + ((double)tv.tv_usec) / 1000000.0;
}

int get_compression_type( const char *pcCompression )
{
	if (strcasecmp(pcCompression, "u-law") == 0)
		return SF_FORMAT_ULAW;
	if (strcasecmp(pcCompression, "a-law") == 0)
		return SF_FORMAT_ALAW;
	if (strcasecmp(pcCompression, "IMA-ADPCM") == 0)
		return SF_FORMAT_IMA_ADPCM;
	if (strcasecmp(pcCompression, "MS-ADPCM") == 0)
		return SF_FORMAT_MS_ADPCM;
	if (strcasecmp(pcCompression, "GSM-6.10") == 0)
		return SF_FORMAT_GSM610;
	if (strcasecmp(pcCompression, "G721_32") == 0)
		return SF_FORMAT_G721_32;
	if (strcasecmp(pcCompression, "G723_24") == 0)
		return SF_FORMAT_G723_24;
	if (strcasecmp(pcCompression, "G723_40") == 0)
		return SF_FORMAT_G723_40;

	return -1;
} /* int get_compression_type( ) */

int get_format_type(const char *type)
{
	if (strcasecmp(type, "wav") == 0)
		return SF_FORMAT_WAV;
	if (strcasecmp(type, "aiff") == 0)
		return SF_FORMAT_AIFF;
	if (strcasecmp(type, "au") == 0)
		return SF_FORMAT_AU;
	if (strcasecmp(type, "raw") == 0)
		return SF_FORMAT_RAW;
	if (strcasecmp(type, "svx") == 0)
		return SF_FORMAT_SVX;
	if (strcasecmp(type, "nist") == 0)
		return SF_FORMAT_NIST;
	if (strcasecmp(type, "voc") == 0)
		return SF_FORMAT_VOC;
	if (strcasecmp(type, "ircam") == 0)
		return SF_FORMAT_IRCAM;
	if (strcasecmp(type, "w64") == 0)
		return SF_FORMAT_W64;
	if (strcasecmp(type, "mat4") == 0)
		return SF_FORMAT_MAT4;
	if (strcasecmp(type, "mat5") == 0)
		return SF_FORMAT_MAT5;
	if (strcasecmp(type, "pvf") == 0)
		return SF_FORMAT_PVF;
	if (strcasecmp(type, "xi") == 0)
		return SF_FORMAT_XI;
	if (strcasecmp(type, "htk") == 0)
		return SF_FORMAT_HTK;
	if (strcasecmp(type, "sds") == 0)
		return SF_FORMAT_SDS;
	if (strcasecmp(type, "avr") == 0)
		return SF_FORMAT_AVR;
	if (strcasecmp(type, "wavex") == 0)
		return SF_FORMAT_WAVEX;

	return -1;
}

void start_wavfile(SNDFILE **sndfile_out, char *fname, int sample_rate, int channels)
{
	SF_INFO sfinfo_out;
	time_t now;
	char *ts, *dummy;
	struct tm *tmdummy;

	/* create filename */
	time(&now);
	tmdummy = localtime(&now);
	if (fname_template)
	{
		int len = strlen(fname_template), loop;
		int index;

		strcpy(fname, wav_path);
		index = strlen(fname);

		for(loop=0; loop<len; loop++)
		{
			if (fname_template[loop] == '%')
			{
				switch(fname_template[++loop]) {
				case 'h':		/* hostname */
					if (gethostname(&fname[index], PATH_MAX - index) == -1)
					{
						syslog(LOG_CRIT, "Error finding hostname: %m");
						exit(1);
					}
					break;
				case 'H':		/* hour */
					sprintf(&fname[index], "%02d", tmdummy -> tm_hour);
					break;
				case 'M':		/* minutes */
					sprintf(&fname[index], "%02d", tmdummy -> tm_min);
					break;
				case 'S':		/* seconds */
					sprintf(&fname[index], "%02d", tmdummy -> tm_sec);
					break;
				case 's':		/* seconds since epoch*/
					sprintf(&fname[index], "%d", (int)now);
					break;
				case 'y':		/* year */
					sprintf(&fname[index], "%02d", tmdummy -> tm_year + 1900);
					break;
				case 'm':		/* month */
					sprintf(&fname[index], "%02d", tmdummy -> tm_mon + 1);
					break;
				case 'd':		/* day */
					sprintf(&fname[index], "%02d", tmdummy -> tm_mday);
					break;
				case '%':
					strcat(&fname[index], "%%");
					break;
				default:
					syslog(LOG_ERR, "%%%c is unknown", fname_template[loop]);
				}

				index = strlen(fname);
			}
			else
			{
				fname[index++] = fname_template[loop];
				fname[index] = 0x00;
			}
		}
	}
	else
	{
		sprintf(fname, "%s/%04d-%02d-%02d_%02d%02d%02d.wav",
			wav_path,
			tmdummy -> tm_year + 1900,
			tmdummy -> tm_mon + 1,
			tmdummy -> tm_mday,
			tmdummy -> tm_hour,
			tmdummy -> tm_min,
			tmdummy -> tm_sec);
	}

	/* create file */
	memset(&sfinfo_out, 0x00, sizeof(sfinfo_out));
        sfinfo_out.samplerate = sample_rate;
        sfinfo_out.channels = channels;
        sfinfo_out.format = format | compression;
        *sndfile_out = sf_open(fname, SFM_WRITE, &sfinfo_out);
	if (!*sndfile_out)
		error_exit("Could not create file %s!", fname);

	/* add string describing 'listener' to file */
	(void)sf_set_string(*sndfile_out, SF_STR_SOFTWARE, "Generated with listener v" VERSION ", http://www.vanheusden.com/listener/");
	/* add timestamp to file */
	ts = ctime(&now);
	dummy = strchr(ts, '\n');
	if (dummy) *dummy = 0x00;
	(void)sf_set_string(*sndfile_out, SF_STR_DATE, ts);

	syslog(LOG_INFO, "Started recording to %s", fname);
	if (do_not_fork) printf("Started recording to %s\n", fname);
}

void start_wav_file_processor(char *fname)
{
	if (exec)
	{
		pid_t pid;

		syslog(LOG_INFO, "Starting childprocess: %s", exec);
		if (do_not_fork) printf("Starting childprocess: %s\n", exec);

		pid = fork();

		if (pid == -1)
		{
			syslog(LOG_ERR, "Failed to fork! %m");
		}
		else if (pid == 0)
		{
			if (-1 == execlp(exec, exec, fname, (void *)NULL))
				error_exit("Failed to start childprocess %s with parameter %s: %m", exec, fname);
		}
	}
}

void start_on_event_start(void)
{
	if (on_event_start)
	{
		pid_t pid;

		syslog(LOG_INFO, "Starting childprocess: %s", exec);
		if (do_not_fork) printf("Starting childprocess: %s\n", exec);

		pid = fork();

		if (pid == -1)
		{
			syslog(LOG_ERR, "Failed to fork! %m");
		}
		else if (pid == 0)
		{
			if (-1 == execlp(on_event_start, on_event_start, (void *)NULL))
				error_exit("Failed to start childprocess %s: %m", exec);
		}
	}
}

int check_for_sound(short *buffer, int n_samples)
{
	int loop;
	int threshold_count = 0;

	/* see if it became silent again */
	for(loop=0; loop<n_samples; loop++)
	{
		if (likely(abs(buffer[loop]) > detect_level))
		{
			threshold_count++;
		}
	}

	return threshold_count;
}

void do_amplify(short *buffer, int n_samples, double *factor)
{
	int loop;

	if (!fixed_ampl_fact)
	{
		double amp = 9999999.0;

		/* cal. new ampl. factor */
		for(loop=0; loop<n_samples; loop++)
		{
			amp = min(amp, fabs(32767.0 / (double)buffer[loop]));
		}

		amp = min(max_amplify, amp);
		*factor = (*factor + amp) / 2;

		if (do_not_fork) printf("New factor: %f\r", *factor);
	}

	/* amplify */
	for(loop=0; loop<n_samples; loop++)
	{
		buffer[loop] = (short)((double)buffer[loop] * *factor);
	}
}

int start_piped_proc(char *what)
{
	int fds[2];
	pid_t pid;

	if (pipe(fds) == -1)
		error_exit("error creating pipe\n");

	pid = fork();
	if (pid == -1)
		error_exit("error forking\n");
	if (pid == 0)
	{
		close(0);
		dup(fds[0]);

		if (-1 == execlp("/bin/sh", "/bin/sh", "-c", what, (void *)NULL)) error_exit("execlp of %s failed\n", what);
		exit(1);
	}

	close(fds[0]);

	return fds[1];
}

void record(snd_pcm_t *pcm_handle, int cur_buffer, int sample_rate, int channels)
{
	SNDFILE *sndfile_out = NULL;
	char fname[PATH_MAX];
	double start_of_recording_ts, sound_detected_ts, start_of_wavfile_ts, now_ts;
	short *buffer, *temp_buffer = NULL;
	double factor = start_amplify;
	int buffer_size = sample_rate * channels * sizeof(short);
	int loop;
	int pipe_fd = -1;

	start_on_event_start();

	buffer = (short *)mymalloc(buffer_size, "recording buffer(2)");
	if (!safe_after_filter) temp_buffer = (short *)mymalloc(buffer_size, "recording buffer(3)");

	if (!silent && do_not_fork) printf("Sound detected\n");

	if (output_pipe)
		pipe_fd = start_piped_proc(output_pipe);
	else
		start_wavfile(&sndfile_out, fname, sample_rate, channels);

	gettime(&start_of_recording_ts);
	start_of_wavfile_ts = sound_detected_ts = now_ts = start_of_recording_ts;

	/* write the samples to disk which we already had recorded before the sound started */
	if (!silent && do_not_fork) printf("Writing pre-sound data to disk\n");
	for(loop=0; loop<pr_n_seconds; loop++)
	{
		int n_wr;
		short *p_cur_buffer = pr_buffers[(cur_buffer + 1 + loop) % pr_n_seconds];

		if (!p_cur_buffer)
			continue;

		/* amplify sound before store */
		if (amplify)
			do_amplify(p_cur_buffer, sample_rate * channels, &factor);

		if (output_pipe)
		{
			WRITE(pipe_fd, (char *)p_cur_buffer, sample_rate * channels * sizeof(short));
		}
		else
		{
			if ((n_wr = sf_write_short(sndfile_out, p_cur_buffer, sample_rate * channels)) != (sample_rate * channels))
				error_exit("failed to write to wav-file: %s (%d)\n", sf_strerror(sndfile_out), n_wr);
		}
	}

	if (!silent && do_not_fork) printf("Started recording...\n");

	for(; do_exit == 0; )
	{
		int n_wr;
		int loop, threshold_count = 0;

		/* read sample */
		get_audio_1s(pcm_handle, buffer, sample_rate, channels);

		/* when we want to save unfiltered sound, make a copy first */
		if (!safe_after_filter)
		{
			memcpy(temp_buffer, buffer, buffer_size);
		}

		/* do filters (if any) */
		for(loop=0; loop<n_filter_lib; loop++)
		{
			printf("* filter * ");
			filter[loop].do_filter(filter[loop].lib_data, (void *)buffer, sample_rate);
		}

		/* after filtering, do we still hear sound? */
		threshold_count = check_for_sound(buffer, sample_rate * channels) / channels;
		if (do_not_fork)
		{
			if (threshold_count < min_triggers)
				printf("Silence?\n");
			else
				printf("Sound %f\n", get_ts());
		}

		/* amplify sound before store */
		if (amplify)
			do_amplify(safe_after_filter?buffer:temp_buffer, sample_rate * channels, &factor);

		/* write sample to disk */
		if (output_pipe)
		{
			WRITE(pipe_fd, (char *)(safe_after_filter?buffer:temp_buffer), sample_rate * channels * sizeof(short));
		}
		else
		{
			if ((n_wr = sf_write_short(sndfile_out, safe_after_filter?buffer:temp_buffer, sample_rate * channels)) != sample_rate * channels)
				error_exit("failed to write to wav-file: %s (%d)\n", sf_strerror(sndfile_out), n_wr);
		}

		gettime(&now_ts);

		/* did things got silent AND we recorded longer then the minimum duration AND the silence is longer then some thresholdvalue? */
		if (threshold_count < min_triggers && (now_ts - start_of_recording_ts) > min_duration && (now_ts - sound_detected_ts) > rec_silence)
		{
			if (do_not_fork) printf("Stopped recording: %d peaks\n", threshold_count);
			/*  then exit */
			break;
		}

		/* heard any sound? */
		if (threshold_count >= min_triggers)
		{
			sound_detected_ts = now_ts;
		}

		/* took too long? then start new file */
		if (max_duration != -1 && (now_ts - start_of_wavfile_ts) > max_duration)
		{
			if (do_not_fork) printf("Splitting up audio-file\n");

			if (!output_pipe)
				sf_close(sndfile_out);

			start_wav_file_processor(fname);

			if (!output_pipe)
				start_wavfile(&sndfile_out, fname, sample_rate, channels);

			start_of_wavfile_ts = now_ts;
		}
	}

	if (sndfile_out)
	{
		if (!output_pipe)
			sf_close(sndfile_out);

		start_wav_file_processor(fname);
	}

	free(temp_buffer);
	free(buffer);

	/* empty the ring buffer */
	for(loop=0; loop<pr_n_seconds; loop++)
	{
		free(pr_buffers[loop]);
		pr_buffers[loop] = NULL;
	}

	if (pipe_fd != -1)
		close(pipe_fd);

	syslog(LOG_INFO, "Stopped recording (%d seconds)", (int)(now_ts - start_of_recording_ts));
	if (do_not_fork) printf("Stopped recording (%d seconds)\n", (int)(now_ts - start_of_recording_ts));
}

void sigh(int sig)
{
	if (sig == SIGTERM)
		do_exit = 1;
}


void usage(void)
{
	printf( "\n"
		"Usage: listener [options]\n"
		"-C<compression>  Set WAV compression    -e<command>      Script to call after recording\n"
		"-r<rate>         Sample rate            -m<min_duration> Mini. duration to record (samples)\n"
		"-b<rec_silence>  how many seconds to keep recording after no sound is heard\n"
		"-c<configfile>   Configfile to use      -x<max_duration> Max. duration to record (seconds)\n"
		"-w<wave-dir>     Where to write .WAVs   -d<device>       DSP device to use (hw:0)\n"
		"-z<channels>     Number of channels (1 (default) or 2)\n"
		"-t<format>       Output format (see manual)\n"
		"-y<command>      Script to call as soon as the recording starts\n"
		"-F               use a fixed amplification factor\n"
		"-p               Read from pipe (together with splitaudio)\n"
		"-f               don't fork into the background\n"
		"-l<detect_level> Detect level           -a<pidfile>      file to write the pid in\n"
		"-s               Be silent              -h               This help text\n\n"
		"-o               exit after 1 recording\n");
}

int main(int argc, char *argv[])
{
	int	c;
	char	show_help = 0;
	char	from_pipe = 0;
	char	set_from_pipe = 0, set_sample_rate = 0, set_detect_level = 0, set_max_duration = 0, set_rec_silence = 0, 
		set_min_duration = 0, set_compression = 0, set_exec = 0, set_dev_name = 0, set_wav_path = 0, set_channels = 0,
		set_format = 0, set_faf = 0, set_on_event_start = 0, set_pidfile = 0, set_one_shot = 0, set_output_pipe = 0;
	int	loop;
	FILE	*fh;
	snd_pcm_t	*pcm_handle;
	char	*pcm_name = "hw:0";
	int	sample_rate = SAMPLE_RATE;				/* speaks for itself */
	char	channels = 1;					/* mono or stereo */
	/* right now I'm not sure if a short is always 2 bytes. check this on your
	 * platform
	 */
	int	cur_buffer = 0;

	/* Check command-line options;
	 */
	while ( (c = getopt( argc, argv, "oy:b:c:shS::r:l:m:C:e:w:d:x:t:fpz:Fa:" )) != -1 )
	{
		switch (c)
		{
	          case 'o':
			one_shot = 1;
			set_one_shot = 1;
			break;

		  case 'a':	/* pidfile */
			set_pidfile = 1;
			pidfile = optarg;
			break;

		  case 'f':	/* do not fork */
			do_not_fork = 1;
			break;

		  case 'w':	/* WAV directory */
			wav_path = optarg;
			set_wav_path = 1;
		 	break;

		  case 'd':	/* Device */
			pcm_name = optarg;
			set_dev_name = 1;
			break;

		  case 'b':	/* rec_silence */
			rec_silence = atof(optarg);
			set_rec_silence = 1;
			break;

		  case 'e':	/* Exec */
			exec = optarg;
			set_exec = 1;
			break;

		  case 'y':	/* on_event_start */
			on_event_start = optarg;
			set_on_event_start = 1;
			break;

		  case 'F':	/* use fixed amplification factor */
			fixed_ampl_fact = 1;
			set_faf = 1;
			break;

		  case 'C':	/* Compression */
			if ((compression = get_compression_type(optarg)) == -1)
			{
				fprintf(stderr, "%s is not a known compression type\n", optarg);
				return 2;
			}
			set_compression = 1;
			break;

                  case 't':	/* format type */
			if ((format = get_format_type(optarg)) == -1)
			{
				fprintf(stderr, "%s is an unknown output format\n", optarg);
				return 2;
			}
			set_format = 1;
			break;

		  case 'm':	/* Min duration */
			min_duration = atof(optarg);
			set_min_duration = 1;
			break;

		  case 'x':	/* Max duration */
			max_duration = atof(optarg);
			set_max_duration = 1;
			break;

		  case 'l':	/* Detect level */
			detect_level = atoi(optarg);
			set_detect_level = 1;
			break;

		  case 'r':	/* Sample rate */
			sample_rate = atoi(optarg);
			set_sample_rate = 1;
			break;
			
		  case 'h':	/* Help -- display usage */
			show_help = 1;
			break;

		  case 'c':	/* Configuration-file */
			configfile = strdup(optarg);
			break;

		  case 's':	/* Be silent.. */
			silent = 1;
			break;

		  case 'p':	/* read from pipe */
			from_pipe = 1;
			set_from_pipe = 1;
			break;

		  case 'z':	/* number of channels */
			channels = atoi(optarg);
			set_channels = 1;
			break;

		  default:
			/* unknown switch */
			show_help = 1;
			break;
		} /* switch.. */	

	} /* while.. */

	if (silent == 0)
		printf("listener v" VERSION ", (C)2003-2005 by folkert@vanheusden.com\n");

	if (from_pipe && channels > 1)
	{
		fprintf(stderr, "You can only monitor 1 channel when reading from a pipe!\n");
		return 1;
	}

	if (show_help == 1)
	{
		usage();	
		return 1;
	}
	
	/* Apply default settings;
	 */
	if (configfile == NULL) configfile = strdup( CONFIGFILE );
	if (wav_path == NULL) wav_path = strdup( WAV_PATH );

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
			int len;

                        /* find start of parameter(s) */
                        par = i_is+1;
                        while ((*par) == ' ') par++;

			len = strlen(par) - 1;
			while(len > 0 && par[len] == ' ')
			{
				par[len]=0x00;
				len--;
			}

			*i_is = 0x00;
                }
                cmd = get_token(line);

		if (strcasecmp(cmd, "wav_path") == 0)
		{
			if (!set_wav_path)
				wav_path = strdup(par);
		}
		else if (strcasecmp(cmd, "output_pipe") == 0)
		{
			if (!set_output_pipe)
				output_pipe = strdup(par);
		}
		else if (strcasecmp(cmd, "pidfile") == 0)
		{
			if (!set_pidfile)
				pidfile = strdup(par);
		}
		else if (strcasecmp(cmd, "devname") == 0)
		{
			if (!set_dev_name)
				pcm_name = strdup(par);
		}
		else if (strcasecmp(cmd, "detect_level") == 0)
		{
			if (!set_detect_level)
				detect_level = atoi(par);
		}
		else if (strcasecmp(cmd, "rec_silence") == 0)
		{
			if (!set_rec_silence)
				rec_silence = atof(par);
		}
		else if (strcasecmp(cmd, "min_triggers") == 0)
		{
			min_triggers = atoi(par);
		}
		else if (strcasecmp(cmd, "min_duration") == 0)
		{
			if (!set_min_duration)
				min_duration = atof(par);
		}
		else if (strcasecmp(cmd, "max_duration") == 0)
		{
			if (!set_max_duration)
				max_duration = atof(par);
		}
		else if (strcasecmp(cmd, "sample_rate") == 0)
		{
			if (!set_sample_rate)
				sample_rate = atoi(par);
		}
		else if (strcasecmp(cmd, "fname_template") == 0)
		{
			fname_template = strdup(par);
		}
		else if (strcasecmp(cmd, "from_pipe") == 0)
		{
			if (!set_from_pipe)
			{
				if (strcasecmp(par, "on") ==0 || strcasecmp(par, "yes") == 0 || strcasecmp(par, "1") == 0)
					from_pipe = 1;
				else
					from_pipe = 0;
			}
		}
		else if (strcasecmp(cmd, "one_shot") == 0)
		{
			if (!set_one_shot)
			{
				if (strcasecmp(par, "on") ==0 || strcasecmp(par, "yes") == 0 || strcasecmp(par, "1") == 0)
					one_shot = 1;
				else
					one_shot = 0;
			}
		}
		else if (strcasecmp(cmd, "compression") == 0)
		{
			if (!set_compression)
			{
				if ((compression = get_compression_type( par )) == -1)
					error_exit("%s is an unknown compression format\n");
			}
		}
		else if (strcasecmp(cmd, "format") == 0)
		{
			if (!set_format && (format = get_format_type( par )) == -1)
				error_exit("%s is an unknown output format\n");
		}
		else if (strcasecmp(cmd, "exec") == 0)
		{
			if (!set_exec)
				exec = strdup(par);
		}
		else if (strcasecmp(cmd, "on_event_start") == 0)
		{
			if (!set_on_event_start)
				on_event_start = strdup(par);
		}
		else if (strcasecmp(cmd, "channels") == 0)
		{
			if (!set_channels)
				channels = atoi(par);
		}
		else if (strcasecmp(cmd, "fixed_amplify") == 0)
		{
			if (!set_faf)
			{
				if (strcasecmp(par, "on") ==0 || strcasecmp(par, "yes") == 0 || strcasecmp(par, "1") == 0)
					fixed_ampl_fact = 1;
				else
					fixed_ampl_fact = 0;
			}
		}
		else if (strcasecmp(cmd, "amplify") == 0)
		{
			if (strcasecmp(par, "on") ==0 || strcasecmp(par, "yes") == 0 || strcasecmp(par, "1") == 0)
				amplify = 1;
			else
				amplify = 0;
		}
		else if (strcasecmp(cmd, "start_amplify") == 0)
		{
			start_amplify = atof(par);
		}
		else if (strcasecmp(cmd, "max_amplify") == 0)
		{
			max_amplify = atof(par);
		}
		else if (strcasecmp(cmd, "filter") == 0)
		{
			if (n_filter_lib < MAX_N_LIBRARIES)
			{
                                char *dummy = strchr(par, ' ');

                                dlerror();

                                if (dummy) *dummy = 0x00;
				filter[n_filter_lib].library = dlopen(par, RTLD_NOW);
				if (!filter[n_filter_lib].library)
					error_exit("Failed to load filter library %s, reason: %s\n", par, dlerror());

				filter[n_filter_lib].init_library = dlsym(filter[n_filter_lib].library, "init_library");
				if (!filter[n_filter_lib].init_library)
					error_exit("The filter library %s is missing the 'init_library' function");

				filter[n_filter_lib].do_filter    = dlsym(filter[n_filter_lib].library, "do_filter");
				if (!filter[n_filter_lib].do_filter)
					error_exit("The filter library %s is missing the 'do_library' function");

                                filter[n_filter_lib].par = dummy+1 ? strdup(dummy+1) : NULL;

				n_filter_lib++;
			}
			else
				error_exit("Too many filters defined, only %d possible.\n", MAX_N_LIBRARIES);
		}
		else if (strcasecmp(cmd, "safe_after_filter") == 0)
		{
			if (strcasecmp(par, "yes") == 0 ||
			    strcasecmp(par, "on") == 0 ||
			    strcasecmp(par, "1") == 0)
			{
				safe_after_filter = 1;
			}
			else
			{
				safe_after_filter = 0;
			}
		}
		else if (strcasecmp(cmd, "prerecord_n_seconds") == 0)
		{
			pr_n_seconds = atoi(par);
			if (pr_n_seconds < 1)
				error_exit("prerecord_n_seconds must be 1 or bigger (not %d)\n", pr_n_seconds);
		}
		else if (cmd[0] != '#')
			error_exit("'%s=%s' is not a known configuration-statement\n", cmd?cmd:"(none)", par?par:"(none)");

		free(cmd);
		free(line);
	}
	fclose(fh);

	if (min_duration < 1)
		error_exit("min_duration must be at least 1!");

	/* set audio-device to 44.1KHz 16bit mono */
	if (silent == 0)
	{
		printf("Path:          %s\n", wav_path);
		printf("Device:        %s\n", pcm_name);
		printf("Level:         %d\n", detect_level);
		printf("Min duration:  %f\n", min_duration);
		printf("Max duration:  %f\n", max_duration);
		printf("Channels:      %d\n", channels);
		printf("Number of seconds record before sound starts: %d\n", pr_n_seconds);
		if (amplify)
		{
			if (fixed_ampl_fact) printf("Using fixed amplification\n");
			printf("Start amplify: %f\n", start_amplify);
			if (!fixed_ampl_fact) printf("Max. amplify:  %f\n", max_amplify);
		}

		if (from_pipe)
			printf("Reading from pipe\n");
	}

	/* open audio-device */
	pcm_handle = init_alsa(pcm_name, &sample_rate, channels);

	if (!silent)
		printf("Samplerate:    %d\n", sample_rate);

	/* initialize filters */
	for(loop=0; loop<n_filter_lib; loop++)
	{
		filter[loop].lib_data = filter[loop].init_library(sizeof(short), channels, sample_rate, filter[loop].par);
	}

	if (!do_not_fork)
	{
		FILE *pfh;

		if (daemon(-1, -1) == -1)
			error_exit("problem becoming daemon process");

		if (pidfile)
		{
			pfh = fopen(pidfile, "w");
			if (!pfh)
				error_exit("cannot access pid-file %s", pidfile);

			fprintf(pfh, "%d", getpid());

			fclose(pfh);
		}
	}

	/* allocate & clear ringbuffer */
	pr_buffers = (short **)mymalloc(sizeof(short *) * pr_n_seconds, "ring buffer");
	memset(pr_buffers, 0x00, sizeof(short *) * pr_n_seconds);

	syslog(LOG_INFO, "listener started");

	signal(SIGTERM, sigh);
	signal(SIGCHLD, SIG_IGN);

	for(;do_exit == 0;)
	{
		int threshold_count = 0;
		int loop;

		/* allocate recording buffer */
		if (!pr_buffers[cur_buffer])
		{
			pr_buffers[cur_buffer] = (short *)mymalloc(sample_rate * channels * sizeof(short), "recording buffer(1)");
		}

		/* check audio-level */
		/* read data */
		get_audio_1s(pcm_handle, pr_buffers[cur_buffer], sample_rate, channels);

		/* do filters (if any) */
		for(loop=0; loop<n_filter_lib; loop++)
		{
			filter[loop].do_filter(filter[loop].lib_data, (void *)pr_buffers[cur_buffer], sample_rate);
		}

		/* check audio-levels */
		for(loop=0; loop<(sample_rate * channels); loop++)
		{
			if (unlikely(abs(pr_buffers[cur_buffer][loop]) > detect_level))
			{
				threshold_count++;

				if (threshold_count >= min_duration)
				{
					record(pcm_handle, cur_buffer, sample_rate, channels);

					if (one_shot)
						do_exit = 1;

					break;
				}
			}
		}

		if (++cur_buffer == pr_n_seconds)
			cur_buffer = 0;
	}

	if (pidfile)
	{
		if (unlink(pidfile) == -1)
			error_exit("failed to delete pid-file (%s)", pidfile);
	}

	return 0;
}
