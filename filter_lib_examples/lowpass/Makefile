INSTALL_PREFIX=/usr/local

CFLAGS=-fPIC -Wshadow -Wwrite-strings -Wconversion -Wall -pedantic
VERSION=0.1

OBJS=lowpass.o

all: lib

lib: $(OBJS)
	ar -r lowpass.a $(OBJS)
	ld -x -lm -shared -soname lowpass.so.2 -o lowpass.so.$(VERSION) --whole-archive lowpass.a

install: all
	cp lowpass.so.* $(INSTALL_PREFIX)/lib
	/sbin/ldconfig

uninstall: clean
	rm -f $(INSTALL_PREFIX)/lib/lowpass.so.* 

clean:
	rm -f $(OBJS) lowpass.a lowpass.so.* core
