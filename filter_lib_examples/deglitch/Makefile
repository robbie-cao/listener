INSTALL_PREFIX=/usr/local

CFLAGS=-fPIC -Wshadow -Wwrite-strings -Wconversion -Wall -pedantic
VERSION=0.1

OBJS=deglitch.o

all: lib

lib: $(OBJS)
	ar -r deglitch.a $(OBJS)
	ld -x -lm -shared -soname deglitch.so.1 -o deglitch.so.$(VERSION) --whole-archive deglitch.a

install: all
	cp deglitch.so.* $(INSTALL_PREFIX)/lib
	/sbin/ldconfig

uninstall: clean
	rm -f $(INSTALL_PREFIX)/lib/deglitch.so.* 

clean:
	rm -f $(OBJS) deglitch.a deglitch.so.* core
