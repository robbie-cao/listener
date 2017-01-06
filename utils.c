#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include "error.h"

ssize_t READ(int fd, char *whereto, size_t len)
{
        ssize_t cnt=0;

        while(len>0)
        {
                ssize_t rc;

                rc = read(fd, whereto, len);

                if (rc == -1)
                {
                        if (errno != EINTR)
                                error_exit("error while read()\n");
                }
                else if (rc == 0)
                {
                        break;
                }
                else
                {
                        whereto += rc;
                        len -= rc;
                        cnt += rc;
                }
        }

        return cnt;
}

ssize_t WRITE(int fd, char *wherefrom, size_t len)
{
        ssize_t cnt=0;

        while(len>0)
        {
                ssize_t rc;

                rc = write(fd, wherefrom, len);

                if (rc == -1)
                {
                        if (errno != EINTR)
                                error_exit("error while write()\n");
                }
                else if (rc == 0)
                {
                        break;
                }
                else
                {
                        wherefrom += rc;
                        len -= rc;
                        cnt += rc;
                }
        }

        return cnt;
}

int resize(void **pnt, size_t n, size_t *len, size_t size)
{
        if (n == *len)
        {
                size_t dummylen = (*len) == 0 ? 2 : (*len) * 2;
                void *dummypnt = (void *)realloc(*pnt, dummylen * size);

                if (!dummypnt)
                        error_exit("resize::realloct: Cannot (re-)allocate %d bytes of memory\n", dummylen * size);

		*len = dummylen;
		*pnt = dummypnt;
        }
	else if (n > *len || n<0 || *len<0)
		error_exit("resize: fatal memory corruption problem: n > len || n<0 || len<0!");

	return 0;
}

char * read_line(FILE *fh)
{
	char *str = NULL;
	size_t n_in=0, size=0;

	for(;;)
	{
		int c;

		/* read one character */
		c = fgetc(fh);
		if (c == EOF)
			break;

		/* resize input-buffer */
		if (resize((void **)&str, n_in, &size, sizeof(char)) == -1)
		{
			free(str);
			error_exit("read_line_fd::resize: malloc failed (for %d bytes)", size);
		}

		/* EOF or \n == end of line */
		if (c == 10 || c == EOF)
			break;

		/* ignore CR */
		if (c == 13)
			continue;

		/* add to string */
		str[n_in++] = c;
	}

	if (n_in)
	{
		/* terminate string */
		if (resize((void **)&str, n_in, &size, sizeof(char)) == -1)
		{
			free(str);
			error_exit("read_line_fd::resize: malloc failed (for %d bytes)", size);
		}
		str[n_in] = 0x00;
	}
	else
	{
		free(str);
		str = NULL;
	}

	return str;
}

char * get_token(char *in)
{
        char *out, *dummy;

        // skip leading spaces/tabs
        while(*in == ' ' || *in == '\t')
                in++;

        // make copy of string
        dummy = out = strdup(in);
        if (!out)
                error_exit("get_token::strdup: strdup-malloc failure");

        // remove trailing spaces/tabs
        while (*dummy != ' ' && *dummy != '\t' && *dummy != 0x00)
                dummy++;

        *dummy = 0x00;

        return out;
}

void * mymalloc(size_t size, char *what)
{
	void *dummy = malloc(size);
	if (!dummy)
		error_exit("failed to allocate %d bytes for %s\n", size, what);

	return dummy;
}

double get_ts(void)
{
        struct timeval ts;

        if (gettimeofday(&ts, NULL) == -1)
                error_exit("gettimeofday failed");

        return (((double)ts.tv_sec) + ((double)ts.tv_usec)/1000000.0);
}
