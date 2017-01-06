#include "alsa.h"
#include "error.h"
#include "utils.h"

const snd_pcm_uframes_t periodsize;

snd_pcm_t *init_alsa(char *pcm_name, int *rate, int n_channels)
{
	/* Handle for the PCM device */ 
	snd_pcm_t *pcm_handle;          

	/* Capture stream */
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

	/* This structure contains information about    */
	/* the hardware and can be used to specify the  */      
	/* configuration to be used for the PCM stream. */ 
	snd_pcm_hw_params_t *hwparams;

	/* Name of the PCM device, like plughw:0,0          */
	/* The first number is the number of the soundcard, */
	/* the second number is the number of the device.   */
	// char *pcm_name;

	/* Init pcm_name. Of course, later you */
	/* will make this configurable ;-)     */
	// pcm_name = strdup("hw:0");

	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
	snd_pcm_hw_params_alloca(&hwparams);

	/* Open PCM. The last parameter of this function is the mode. */
	/* If this is set to 0, the standard mode is used. Possible   */
	/* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */ 
	/* If SND_PCM_NONBLOCK is used, read / write access to the    */
	/* PCM device will return immediately. If SND_PCM_ASYNC is    */
	/* specified, SIGIO will be emitted whenever a period has     */
	/* been completely processed by the soundcard.                */
	if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0) {
		error_exit("Error opening PCM device %s\n", pcm_name);
	}

	/* Init hwparams with full configuration space */
	if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
		error_exit("Can not configure this PCM device.\n");
	}

	// int rate = 44100; /* Sample rate */
	unsigned int exact_rate;   /* Sample rate returned by */
	/* snd_pcm_hw_params_set_rate_near */ 
	int dir;          /* exact_rate == rate --> dir = 0 */
	/* exact_rate < rate  --> dir = -1 */
	/* exact_rate > rate  --> dir = 1 */
	int periods = 2;       /* Number of periods */
	// snd_pcm_uframes_t periodsize = 8192; /* Periodsize (bytes) */ 

	/* Set access type. This can be either    */
	/* SND_PCM_ACCESS_RW_INTERLEAVED or       */
	/* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
	/* There are also access types for MMAPed */
	/* access, but this is beyond the scope   */
	/* of this introduction.                  */
	if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		error_exit("Error setting access.\n");
	}

	/* Set sample format */
	if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
		error_exit("Error setting format.\n");
	}

	/* Set sample rate. If the exact rate is not supported */
	/* by the hardware, use nearest possible rate.         */ 
	exact_rate = *rate;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0) < 0) {
		error_exit("Error setting rate.\n");
	}
	if (*rate != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n ==> Using %d Hz instead.\n", *rate, exact_rate);
		*rate = exact_rate;
	}

	/* Set number of channels */
	if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, n_channels) < 0)
		error_exit("Error setting channels.\n");

	/* Set number of periods. Periods used to be called fragments. */ 
	if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0) < 0) {
		error_exit("Error setting periods.\n");
	}

	/* Set buffer size (in frames). The resulting latency is given by */
	/* latency = periodsize * periods / (rate * bytes_per_frame)     */
	if (snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, *rate * 2 * n_channels) < 0) {
		error_exit("Error setting buffersize.\n");
	}

	/* Apply HW parameter settings to */
	/* PCM device and prepare device  */
	if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
		error_exit("Error setting HW params.\n");
	}

	return pcm_handle;
}

void get_audio_1s(snd_pcm_t *pcm_handle, short *buffer, int rate, int channels)
{
	int samples = channels * rate, index = 0;

	while(samples > 0)
	{
		int cur = samples, pcmreturn;

		while ((pcmreturn = snd_pcm_readi(pcm_handle, &buffer[index], cur)) < 0) {
			snd_pcm_prepare(pcm_handle);
printf("                                 underrun\n");
		}

		index += pcmreturn;
		samples -= pcmreturn;
	}
}
