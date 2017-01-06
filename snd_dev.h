int set_soundcard_sample_format(int fd);
int set_soundcard_number_of_channels(int fd, int n_channels_req, int *got_n_channels);
int set_soundcard_sample_rate(int fd, int hz);
