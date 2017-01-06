INSTALL_PREFIX=/usr/local

CFLAGS=-fPIC -Wshadow -Wwrite-strings -Wconversion
VERSION=0.2

OBJS=my_filter2.o

all: lib

lib: $(OBJS)
	ar -r my_filter2.a $(OBJS)
	ld -x -lm -shared -soname my_filter2.so.2 -o my_filter2.so.$(VERSION) --whole-archive my_filter2.a

install: all
	cp my_filter2.so.* $(INSTALL_PREFIX)/lib
	/sbin/ldconfig

clean:
	rm -f $(OBJS) my_filter2.a my_filter2.so.* core
