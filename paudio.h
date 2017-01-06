#include <portaudio.h>

#define BUFFER_SIZE 4096

PaStream *paudio_init (int *rate, int n_channels);
void paudio_get_1s (PaStream *pcm_handle, short *buffer, int rate, int channels);
