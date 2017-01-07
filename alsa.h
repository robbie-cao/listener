#include <alsa/asoundlib.h>

#define BUFFER_SIZE 8192

snd_pcm_t *init_alsa(char *pcm_name, int *rate, int n_channels);
void get_audio_1s(snd_pcm_t * pcm_handle, short *buffer, int rate, int channels);
