# listener

This program listens for sound. If it detects any, it starts recording automatically and also automatically stops when things become silent again.

## Downloads

- [listener-2.2.tgz](https://www.vanheusden.com/listener/listener-2.2.tgz)
- [listener-2.0.1.tgz](https://www.vanheusden.com/listener/listener-2.0.1.tgz)    (portaudio version)
- [listener-2.0.0.tgz](https://www.vanheusden.com/listener/listener-2.0.0.tgz)    (ALSA version)
- [listener-1.7.2.tgz](https://www.vanheusden.com/listener/listener-1.7.2.tgz)    (old OSS version)

This program needs [`libsndfile`](http://www.mega-nerd.com/libsndfile/) as well as [portaudio](http://www.portaudio.com/).

```
$ sudo apt-get install libsndfile-dev portaudio19-dev
```

Please note that since version 2.0.1 the command line parameters for selecting e.g. the samplerate have changed. Run with '-h' to see a list.

## Changelog

- **2.2**:   maintenance release, fixed all kinds of small bugs
- **2.0.1**: portaudio-versio which is more usable than the alsa version. also cross-platform so listener might now be usable on e.g. windows as well. the portaudio changes and the new filters were developed by Felipe Emmanuel Ferreira de Castro!
- **2.0.0**: first version which is using ALSA for sound I/O
- **1.7.2**: listener can now write its pid to a file when running in daemon-mode. also added one-shot recording
- **1.7.1**: 'on_event_start' did not work
- **1.7.0**: 'on_event_start' can now specify what script to start as soon as the recording starts, one can now use fixed amplification factor, one can now let listener store the audio recorded before the sound started

## Links

https://www.vanheusden.com/listener/
