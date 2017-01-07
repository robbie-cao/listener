#include <portaudio.h>

#include "paudio.h"
#include "error.h"
#include "utils.h"

PaStream *paudio_init(int *rate, int n_channels)
{
    PaError err;
    PaStream *pcm_handle;
    PaStreamParameters pcm_param;

    /* Initialize PCM
     */
    err = Pa_Initialize();
    error_check(err, "Error initializing audio");

    pcm_param.device = 0;       /* default device should be the first */
    pcm_param.channelCount = n_channels;
    pcm_param.sampleFormat = paInt16;
    pcm_param.suggestedLatency = 0.5;   /* 0.5 seconds, in order to eliminate glitches */
    pcm_param.hostApiSpecificStreamInfo = NULL;

    /* Open PCM stream:
     * PCM handle,
     * Input parameters,
     * No output parameter,
     * sampling rate,
     * frames per buffer (0 = auto),
     * flags (none: auto-clipping, auto-dither, etc),
     * no callback (synchronous mode),
     * no user data for callback
     */
    err = Pa_OpenStream(&pcm_handle, &pcm_param, NULL, *rate, 0, paNoFlag, NULL, NULL);
    error_check(err, "Error opening audio stream");

    /* Start PCM stream
     */
    err = Pa_StartStream(pcm_handle);
    error_check(err, "Error starting audio capture");

    return pcm_handle;
}

void paudio_get_1s(PaStream * pcm_handle, short *buffer, int rate, int channels)
{
    int err;
    static int sec = 0;

    /* Read 1 second of data
     */
    for (err = -1; err < 0 && err != paInputOverflowed;) {
        err = Pa_ReadStream(pcm_handle, buffer, rate);
        if (err < 0 && err != paInputOverflowed) {
            printf("(%li frames) ", Pa_GetStreamReadAvailable(pcm_handle));
            printf("(%i: %s)\n", err, Pa_GetErrorText(err));
        }
    }
    if (++sec < 0)
        sec--;
    /*
     * printf("(%g latency) ", (Pa_GetStreamInfo (pcm_handle))->inputLatency );
     * printf("(%04i) ", sec);
     */
}
