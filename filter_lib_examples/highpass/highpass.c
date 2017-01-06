/* High-pass filter, second order
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Filtering parameters
 */
#define F0 (88.2) /* corner frequency */
#define CSI (0.6) /* damping */

typedef struct
{
	int n_channels;
	int sample_rate;
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
	((info_struct *)pnt)->sample_rate = sample_rate;

	return pnt;
}

/* Warning: sample_data is n_samples * n_channels in size!
 */
void do_filter(void *lib_data, void *sample_data, int n_samples)
{
	/* High-pass filter coeficients */
	const double Q = 1/(2*CSI);
	double fs;
	double w0;
	double alpha;
	double a0;
	double a1;
	double a2;
	double b0;
	double b1;
	double b2;

	/* Processing variables */
	long nch = ((info_struct *)lib_data) -> n_channels;
	short *y = (short *)sample_data;
	short *x;
	size_t bufsize;

	int channel;
	long i_stream;
	long i, i_1, i_2;

	/* Filter init
	 */
	fs = (double) ((info_struct *)lib_data) -> sample_rate;
	w0 = 2*3.141592*F0/fs;
	alpha = sin(w0)/(2*Q);
	a0 = 1 + alpha;
	a1 = (-2*cos(w0)) / a0;
	a2 = (1 - alpha) / a0;
	b1 = (-(1 + cos(w0))) / a0;
	b0 = (-b1 / 2);
	b2 = b0;

	/* Main loop
	 */
	bufsize = (unsigned) (nch * n_samples) * sizeof(short);
	x = (short *) malloc (bufsize);
	if (x == NULL)
	{
		printf ("Memory allocation error: highpass filter\n");
		exit (1);
	}
	memcpy (x, y, bufsize);
	for(channel = 0; channel < nch; channel++)
	{
		/* Filter loop
		 */
		for (i_stream = 2; i_stream < n_samples; i_stream++)
		{
			i = (i_stream * nch) + channel;	
			i_1 = i - nch;
			i_2 = i - 2*nch;
			y[i] = (short) round(b0*x[i] + b1*x[i_1] + b2*x[i_2] - a1*y[i_1] - a2*y[i_2]);
		}

		/* Fade in first 2 points
		 */
		i = nch + channel;	
		i_1 = i - nch;
		y[i_1] = (short) round(1.0/3.0*y[i]);
		y[i] = (short) round(2.0/3.0*y[i]);

		/* Fade out last 2 points
		 */
		i = (n_samples-1 * nch) + channel;	
		i_1 = i - nch;
		y[i] = (short) round(1.0/3.0*y[i_1]);
		y[i_1] = (short) round(2.0/3.0*y[i_1]);
	}
}
