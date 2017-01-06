INSTALL_PREFIX=/usr/local

CFLAGS=-fPIC -Wshadow -Wwrite-strings -Wconversion -Wall -pedantic
VERSION=0.2

OBJS=my_filter1.o

all: lib

lib: $(OBJS)
	ar -r my_filter1.a $(OBJS)
	ld -x -lm -shared -soname my_filter1.so.2 -o my_filter1.so.$(VERSION) --whole-archive my_filter1.a

install: all
	cp my_filter1.so.* $(INSTALL_PREFIX)/lib
	/sbin/ldconfig

clean:
	rm -f $(OBJS) my_filter1.a my_filter1.so.* core
