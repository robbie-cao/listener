#include <stdio.h>
#include <unistd.h>
#include <time.h>

ssize_t READ(int fd, char *whereto, size_t len);
ssize_t WRITE(int fd, char *wherefrom, size_t len);
char *read_line(FILE * fh);
char *get_token(char *in);
void *mymalloc(size_t size, char *what);
double get_ts(void);

#define min(x, y)	((x) < (y) ? (x) : (y))
#define max(x, y)	((x) > (y) ? (x) : (y))
