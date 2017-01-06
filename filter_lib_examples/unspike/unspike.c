/* Try to remove spikes
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Parameters
 */
#define SLEW 5000 /* Minimum jump to catch */
#define VMIN (-4900) /* Minimum initial level to consider on detecting a jump */
#define DMAX 4410 /* Maximum width after spike, to reach 0 again */

typedef struct
{
	int n_channels;
	/* ... */
} info_struct;

/* par is an ascii string containing parameters given on the 'filter=' line
 * in the configuration file
 */
void * init_library(int bits_per_sample, int n_channels, int sample_rate, char *par)
{
	void *pnt;

	/* init library */

	/* you might allocate some data structure here or so and then
	 * return a point to it which will be then given for each call
	 * to do_filter as the first parameter
	 */

	pnt = malloc(sizeof(info_struct));
	if (!pnt)
		exit(1);

	memset(pnt, 0x00, sizeof(info_struct));

	((info_struct *)pnt)->n_channels = n_channels;

	return pnt;
}

/* Warning: sample_data is n_samples * n_channels in size!
 */
void do_filter(void *lib_data, void *sample_data, int n_samples)
{
	int channel;
	short *x = (short *)sample_data;
	int nch;
	long i_stream;
	int j_stream;
	int k_stream;
	long i, i1;
	long i_plus_j;
	long i_plus_k;
	int delta;
	float m;
	int cond_neg;
	int cond_pos;

	nch = ((info_struct *)lib_data) -> n_channels;

	/* Main loop */
	for(channel = 0; channel < nch; channel++)
	{
		for (i_stream = 0; i_stream < n_samples; i_stream++)
		{

			/* Test for steep movement
			 */ 
			i = (i_stream * nch) + channel;	
			i1 = i + nch;
			delta = x[i1] - x[i];
			cond_pos = delta > SLEW && x[i] > VMIN;
			cond_neg = delta < -SLEW && x[i] < -VMIN;
			if (!cond_pos && !cond_neg)
				continue;
			if (cond_pos && cond_neg) /* this condition is impossible... */
				continue;

			/* Found steep movement: go down (or up) to zero!
			 */
			for (j_stream = 2; j_stream < DMAX; j_stream++)
			{
				if (i_stream + j_stream >= n_samples)
				{
					i_stream = n_samples;
					break;
				}
				i_plus_j = (i_stream + j_stream) * nch + channel;
				if ( (cond_pos && (x[i_plus_j] < 0)) ||
				     (cond_neg && (x[i_plus_j] > 0)) )
				{
					m = - (float)x[i] / (float)(j_stream-1);
					for (k_stream = 1; k_stream < j_stream; k_stream++)
					{
						i_plus_k = (i_stream + k_stream) * nch + channel;
						x[i_plus_k] = (short)(x[i] + (int)(m * (float)k_stream));
					}
					i_stream += j_stream - 1;
					break;
				}
			}
		}
	}
}
