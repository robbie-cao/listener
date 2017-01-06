/* Try to remove glitches
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Parameters
 */
#define SLEW 3000 /* Minimum jump to catch */

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
	long j_stream;
	long i, i1, j;
	int delta;
	int cond_neg;
	int cond_pos;

	nch = ((info_struct *)lib_data) -> n_channels;

	/* Main loop */
	for(channel = 0; channel < nch; channel++)
	{
		for (i_stream = 0; i_stream < n_samples-1; i_stream++)
		{

			/* Test for steep movement
			 */ 
			i = (i_stream * nch) + channel;	
			i1 = i + nch;
			delta = x[i1] - x[i];
			if (abs(delta) > SLEW)
			{
				/* Damp backward */
				j_stream = i_stream;
				j = i;
				cond_pos = x[j] > 0;
				cond_neg = x[j] < 0;
				x[j] = 0;
				while (j_stream > 0)
				{
					j_stream--;
					j = (j_stream * nch) + channel;	
					if (cond_pos && x[j] < 0)
						break;
					if (cond_neg && x[j] > 0)
						break;
					x[j] = (short)(x[j] * (1.0 - 9.0/((double)(i_stream-j_stream)+9.0)));
				}

				/* Damp forward */
				j_stream = i_stream + 1;
				j = (j_stream * nch) + channel;	
				cond_pos = x[j] > 0;
				cond_neg = x[j] < 0;
				x[j] = 0;
				while (j_stream < n_samples-1)
				{
					j_stream++;
					j = (j_stream * nch) + channel;	
					if (cond_pos && x[j] < 0)
						break;
					if (cond_neg && x[j] > 0)
						break;
					x[j] = (short)(x[j] * (1.0 - 9.0/((double)(j_stream-i_stream)+8.0)));
				}
			}
		}
	}
}
