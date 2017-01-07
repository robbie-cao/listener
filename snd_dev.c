/*
 * $Id: snd_dev.c,v 1.1 2010-06-22 13:26:33 folkert Exp $
 * $Log: not supported by cvs2svn $
 * Revision 2.1  2003/02/06 22:07:00  folkert
 * added logging
 *
 * Revision 2.0  2003/01/27 17:47:02  folkert
 * *** empty log message ***
 *
 */

#include <sys/ioctl.h>
#if defined(__FreeBSD__)
#include <sys/soundcard.h>
#else
#include <linux/soundcard.h>
#endif
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "error.h"

int set_soundcard_sample_format(int fd)
{
    int format = AFMT_S16_NE;   /* machine-endian 16 bit signed */

    if (ioctl(fd, SNDCTL_DSP_SETFMT, &format) == -1)
        error_exit("set_soundcard_sample_format::ioctl: failed: %d", errno);

    if (format != AFMT_S16_NE)
        error_exit("set_soundcard_sample_format: sampleformat unexpected: %d", format);

    return 0;
}

int set_soundcard_number_of_channels(int fd, int n_channels_req, int *got_n_channels)
{
    *got_n_channels = n_channels_req;

    if (ioctl(fd, SNDCTL_DSP_CHANNELS, got_n_channels) == -1)
        error_exit("set_soundcard_number_of_channels::ioctl: failed: %d", errno);

#if 0
    if (*got_n_channels != n_channels_req)
        fprintf(stderr, "set_soundcard_number_of_channels::ioctl: number of channels unexpected: %d\n",
                *got_n_channels);
#endif

    return 0;
}

int set_soundcard_sample_rate(int fd, int hz)
{
    int dummy = hz;

    if (ioctl(fd, SNDCTL_DSP_SPEED, &dummy) == -1)
        error_exit("set_soundcard_sample_rate::ioctl: failed: %d", errno);

    if (dummy != hz)
        error_exit("set_soundcard_sample_rate::ioctl: samplerate unexpected: %d", dummy);

    return 0;
}
