typedef struct {
	void *library;
	char *par;
	void *lib_data;

	void * (*init_library)(int bits_per_sample, int n_channels, int sample_rate, char *par);
	void (*do_filter)(void *lib_data, void *sample_data, int n_samples);

} filter_lib;
