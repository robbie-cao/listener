#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "error.h"
#include "snd_dev.h"
#include "utils.h"

#define BUFFER_SIZE 2048

void usage(void)
{
    printf("-l pipe    pipe for left\n"
           "-r pipe    pipe for right\n"
           "-d device  sounddevice to read from\n"
           "-s khz     samplerate (default is 44100)\n"
           "-f         don't become daemon\n" "-v         be verbose\n" "-h         this help\n");
}

int main(int argc, char *argv[])
{
    int c;
    char *left = "left.pipe";
    char *right = "right.pipe";
    char *device = "/dev/dsp";
    char verbose = 0;
    char go_daemon = 1;
    int sample_rate = 44100;
    int fd, fdl, fdr;
    int got_n_channels;

    while ((c = getopt(argc, argv, "l:r:d:s:fvh")) != -1) {
        switch (c) {
            case 'l':
                left = optarg;
                break;

            case 'r':
                right = optarg;
                break;

            case 'd':
                device = optarg;
                break;

            case 's':
                sample_rate = atoi(optarg);
                break;

            case 'f':
                go_daemon = 0;
                break;

            case 'v':
                verbose = 1;
                break;

            default:
            case 'h':
                usage();
                return 1;
        }
    }

    if (verbose) {
        printf("Reading from  : %s\n", device);
        printf("Pipe for left : %s\n", left);
        printf("Pipe for right: %s\n", right);
    }

    fd = open(device, O_RDONLY);
    if (fd == -1)
        error_exit("cannot open sounddevice %s", device);

    /* setup soundcard */
    set_soundcard_sample_format(fd);
    set_soundcard_number_of_channels(fd, 2, &got_n_channels);
    set_soundcard_sample_rate(fd, sample_rate);

    if (got_n_channels != 2)
        error_exit("The soundcard (%s) does not support stereo recording (only %d channels).", device, got_n_channels);

    /* become daemon */
    if (go_daemon == 1 && daemon(-1, -1) == -1)
        error_exit("failed to become daemon");

    /* open pipes after becoming daemon as the open blocks when there
     * are no listener processes waiting
     */
    fdl = open(left, O_WRONLY);
    if (fdl == -1)
        error_exit("cannot open pipe for left %s", left);

    fdr = open(right, O_WRONLY);
    if (fdr == -1)
        error_exit("cannot open pipe for right %s", right);

    for (;;) {
        short buffer_in[BUFFER_SIZE];
        short buffer_l[BUFFER_SIZE / 2];
        short buffer_r[BUFFER_SIZE / 2];
        int rc;
        int loop;
        int index = 0;

        /* read from audio device */
        rc = READ(fd, (char *)buffer_in, BUFFER_SIZE * sizeof(short));
        if (rc == -1)
            error_exit("failed reading from sounddevice");

        /* split left & right channel */
        for (loop = 0; loop < BUFFER_SIZE; loop += 2) {
            buffer_l[index] = buffer_in[loop];
            buffer_r[index] = buffer_in[loop + 1];
            index++;
        }

        /* send to pipes */
        if (WRITE(fdl, (char *)buffer_l, (BUFFER_SIZE / 2) * sizeof(short)) == -1)
            error_exit("failed writing to left pipe");

        if (WRITE(fdr, (char *)buffer_r, (BUFFER_SIZE / 2) * sizeof(short)) == -1)
            error_exit("failed writing to right pipe");
    }

    return 0;
}
