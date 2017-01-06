#include <stdlib.h>
#include <math.h>
#include <syslog.h>

typedef struct {

	int n_channels;

	double coef[9];
	double d[4];
	double omega; //peak freq
	double g;     //peak mag

} my_data_struct;

/* par is an ascii string containing parameters given on the 'filter=' line
 * in the configuration file
 */
void * init_library(int bits_per_sample, int n_channels, int sample_rate, char *par)
{
	void *pnt;
	my_data_struct	*data;
	char *dummy;
	double omega, g;
	char type;

	syslog(LOG_DEBUG, "parameter is: %s", par);

	/* init library */

	/* you might allocate some datastructure here or so and then
	 * return a point to it which will be then given for each call
	 * to do_filter as the first parameter
	 */

	pnt = malloc(sizeof(my_data_struct));
	if (!pnt)
		exit(1);

	memset(pnt, 0x00, sizeof(my_data_struct));

	data = (my_data_struct *)pnt;
	data -> n_channels = n_channels;

	type = atoi(par);

	dummy = strchr(par, ' ');
	if (!dummy) { syslog(LOG_ERR, "parameter 2 missing"); exit(1); }
	omega = data -> omega = atof(dummy + 1);
	dummy = strchr(dummy + 1, ' ');
	if (!dummy) { syslog(LOG_ERR, "parameter 3 missing"); exit(1); }
	g = data -> g = atof(dummy);

	// calculating coefficients:
	double k,p,q,a;
	double a0,a1,a2,a3,a4;

	k=(4.0*g-3.0)/(g+1.0);
	p=1.0-0.25*k;p*=p;

	/* LP: */
	if (type == 0)
	{
		a=1.0/(tan(0.5*omega)*(1.0+p));
		p=1.0+a;
		q=1.0-a;
		
		a0=1.0/(k+p*p*p*p);
		a1=4.0*(k+p*p*p*q);
		a2=6.0*(k+p*p*q*q);
		a3=4.0*(k+p*q*q*q);
		a4=    (k+q*q*q*q);
		p=a0*(k+1.0);
        
		(data -> coef)[0]=p;
		(data -> coef)[1]=4.0*p;
		(data -> coef)[2]=6.0*p;
		(data -> coef)[3]=4.0*p;
		(data -> coef)[4]=p;
		(data -> coef)[5]=-a1*a0;
		(data -> coef)[6]=-a2*a0;
		(data -> coef)[7]=-a3*a0;
		(data -> coef)[8]=-a4*a0;
	}
	else
	{
		/* or HP: */
		a=tan(0.5*omega)/(1.0+p);
		p=a+1.0;
		q=a-1.0;
			
		a0=1.0/(p*p*p*p+k);
		a1=4.0*(p*p*p*q-k);
		a2=6.0*(p*p*q*q+k);
		a3=4.0*(p*q*q*q-k);
		a4=    (q*q*q*q+k);
		p=a0*(k+1.0);
			
		(data -> coef)[0]=p;
		(data -> coef)[1]=-4.0*p;
		(data -> coef)[2]=6.0*p;
		(data -> coef)[3]=-4.0*p;
		(data -> coef)[4]=p;
		(data -> coef)[5]=-a1*a0;
		(data -> coef)[6]=-a2*a0;
		(data -> coef)[7]=-a3*a0;
		(data -> coef)[8]=-a4*a0;
	}

	return pnt;
}

void do_filter(void *lib_data, void *sample_data, int n_samples)
{
	my_data_struct *data = (my_data_struct *)lib_data;
	short *pnt = (short *)sample_data;
	int channel, loop;

	/* do the filtering */
	/* warning: sample_data is n_samples * n_channels in size! */

	/* code taken from http://musicdsp.org/archive.php?classid=3#181 */

	/* Main loop */
	for(channel = 0; channel < data -> n_channels; channel++)
	{
		for (loop = 0; loop < n_samples; loop++)
		{
			/* short to double */
			long temp;
			double dummy = pnt[(loop * data -> n_channels) + channel];
			double in    = (dummy < 0) ? dummy / 32768.0 : dummy / 32767.0;
			double out;

			/* filter */
			out            = (data -> coef)[0]*in + (data -> d)[0];
			(data -> d)[0] = (data -> coef)[1]*in + (data -> coef)[5]*out + (data -> d)[1];
			(data -> d)[1] = (data -> coef)[2]*in + (data -> coef)[6]*out + (data -> d)[2];
			(data -> d)[2] = (data -> coef)[3]*in + (data -> coef)[7]*out + (data -> d)[3];
			(data -> d)[3] = (data -> coef)[4]*in + (data -> coef)[8]*out;

			/* double to short */
			temp = out < 0 ? out * 32768.0 : out * 32767.0;

			/* Check clipping */
			if (temp > 32767)
				temp = 32767;
			else if (temp < -32768)
				temp = -32768;

			/* Store new value */
			pnt[(loop * data -> n_channels) + channel] = temp;
		}
	}
}
