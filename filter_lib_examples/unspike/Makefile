INSTALL_PREFIX=/usr/local

CFLAGS=-fPIC -Wshadow -Wwrite-strings -Wconversion -Wall -pedantic
VERSION=0.1

OBJS=unspike.o

all: lib

lib: $(OBJS)
	ar -r unspike.a $(OBJS)
	ld -x -lm -shared -soname unspike.so.2 -o unspike.so.$(VERSION) --whole-archive unspike.a

install: all
	cp unspike.so.* $(INSTALL_PREFIX)/lib
	/sbin/ldconfig

uninstall: clean
	rm -f $(INSTALL_PREFIX)/lib/unspike.so.* 

clean:
	rm -f $(OBJS) unspike.a unspike.so.* core
