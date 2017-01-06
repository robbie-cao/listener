#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <portaudio.h>

void error_syslog (char *format, ...)
{
	char buffer[4096];
	va_list ap;

	va_start (ap, format);
	(void) vsnprintf (buffer, 4096, format, ap);
	va_end (ap);

	/* If you want to write stuff to system log... at your own risk!
	syslog (LOG_INFO, "%s: %m", buffer);
	 */
}

void error_exit(char *format, ...)
{
	char buffer[4096];
	va_list ap;

	va_start (ap, format);
	(void) vsnprintf (buffer, 4096, format, ap);
	va_end (ap);

	fprintf (stderr, "%s\n", buffer);
	fprintf (stderr, "errno: %d/%s (if applicable)\n", errno, strerror(errno));
	error_syslog ("%s: %m", buffer);

	exit(EXIT_FAILURE);
}

void error_check (PaError err, char *msg)
{
	if (err < 0)
		error_exit ("%s (%s)\n", msg, Pa_GetErrorText (err));
}
