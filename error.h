#include <portaudio.h>

void error_syslog(char *format, ...);
void error_exit(const char *format, ...);
void error_check(PaError err, char *msg);
