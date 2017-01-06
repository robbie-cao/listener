#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

void error_exit(char *format, ...)
{
	char buffer[4096];
	va_list ap;

	va_start(ap, format);
	(void)vsnprintf(buffer, 4096, format, ap);
	va_end(ap);
	fprintf(stderr, "%s\n", buffer);
	fprintf(stderr, "errno: %d/%s (if applicable)\n", errno, strerror(errno));
	syslog(LOG_ERR, "%s: %m", buffer);

	exit(EXIT_FAILURE);
}

