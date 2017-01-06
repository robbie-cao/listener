INSTALL_PREFIX=/usr/local

CFLAGS=-fPIC -Wshadow -Wwrite-strings -Wconversion -Wall -pedantic
VERSION=0.1

OBJS=highpass.o

all: lib

lib: $(OBJS)
	ar -r highpass.a $(OBJS)
	ld -x -lm -shared -soname highpass.so.2 -o highpass.so.$(VERSION) --whole-archive highpass.a

install: all
	cp highpass.so.* $(INSTALL_PREFIX)/lib
	/sbin/ldconfig

uninstall: clean
	rm -f $(INSTALL_PREFIX)/lib/highpass.so.* 

clean:
	rm -f $(OBJS) highpass.a highpass.so.* core
