CFLAGS=-O2 -pipe -Wall -Wstrict-prototypes -s

LDLIBS=-L ../.. -ldialog /usr/lib/libncurses_g.a

all: setup
	strip setup

setup: setup.o

install:
	install -m 644 setup.help /usr/share/setup.help

clean:
	rm -f *.o setup

test: setup
	./setup

