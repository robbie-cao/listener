#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct {

	int n_channels;

	/* ... */

} my_data_struct;

/* par is an ascii string containing parameters given on the 'filter=' line
 * in the configuration file
 */
void * init_library(int bits_per_sample, int n_channels, int sample_rate, char *par)
{
	void *pnt;

	/* init library */

	/* you might allocate some datastructure here or so and then
	 * return a point to it which will be then given for each call
	 * to do_filter as the first parameter
	 */

	pnt = malloc(sizeof(my_data_struct));
	if (!pnt)
		exit(1);

	memset(pnt, 0x00, sizeof(my_data_struct));

	((my_data_struct *)pnt) -> n_channels = n_channels;

	return pnt;
}

void do_filter(void *lib_data, void *sample_data, int n_samples)
{
	my_data_struct *mds = (my_data_struct *)lib_data;
	short *pnt = (short *)sample_data;
	int channel;

	/* do the filtering */
	/* warning: sample_data is n_samples * n_channels in size! */

	/* code taken from http://musicdsp.org/archive.php?classid=3#28 */
	/* - Three one-poles combined in parallel
	 * - Output stays within input limits
	 * - 18 dB/oct (approx) frequency response rolloff
	 * - Quite fast, 2x3 parallel multiplications/sample, no internal buffers
	 * - Time-scalable, allowing use with different samplerates
	 * - Impulse and edge responses have continuous differential
	 * - Requires high internal numerical precision
	 */

	/* Parameters */
        /* Number of samples from start of edge to halfway to new value */
        const double        scale = 100;
        /* 0 < Smoothness < 1. High is better, but may cause precision problems */
        const double        smoothness = 0.999;

        /* Precalc variables */
        double                a = 1.0-(2.4/scale); /* Could also be set directly */
        double                b = smoothness;      /*         -"-                */
        double                acoef = a;
        double                bcoef = a*b;
        double                ccoef = a*b*b;
        double                mastergain = 1.0 / (-1.0/(log(a)+2.0*log(b))+2.0/
                        (log(a)+log(b))-1.0/log(a));
        double                again = mastergain;
        double                bgain = mastergain * (log(a*b*b)*(log(a)-log(a*b)) /
                            ((log(a*b*b)-log(a*b))*log(a*b))
                            - log(a)/log(a*b));
        double                cgain = mastergain * (-(log(a)-log(a*b)) /
                        (log(a*b*b)-log(a*b)));

        /* Runtime variables */
        long                streamofs;
        double                areg = 0;
        double                breg = 0;
        double                creg = 0;

	/* Main loop */
	for(channel = 0; channel < mds -> n_channels; channel++)
	{
		for (streamofs = 0; streamofs < n_samples; streamofs++)
		{
			long temp;

			/* Update filters */
			areg = acoef * areg + pnt[(streamofs * mds -> n_channels) + channel];
			breg = bcoef * breg + pnt[(streamofs * mds -> n_channels) + channel];
			creg = ccoef * creg + pnt[(streamofs * mds -> n_channels) + channel];

			/* Combine filters in parallel */
			temp =  again * areg
			      + bgain * breg
			      + cgain * creg;

			/* Check clipping */
			if (temp > 32767)
			{
				temp = 32767;
			}
			else if (temp < -32768)
			{
				temp = -32768;
			}

			/* Store new value */
			pnt[(streamofs * mds -> n_channels) + channel] = temp;
		}
	}
}
