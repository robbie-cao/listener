#define SAMPLE_RATE     44100
#define WAV_PATH        "listen_wav_out"
#define DSP_PATH        "/dev/dsp"
#define CONFIGFILE      "/usr/local/etc/listener.conf"
#define MAX_N_LIBRARIES	16


/* code taken from linux kernel */
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef __builtin_expect
#define __builtin_expect(x, expected_value) (x)
#endif
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
