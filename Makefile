# configuration part of the Makefile

# if you change one of these flags, do 'make clean; make depend'

# For cdialog version 0.9a, libgpm can't be used because it doesn't
# implement the wtimeout() function of ncurses.

# gpm mouse support?
HAVE_GPM_SUPPORT=false
# ncurses or just normal curses?
HAVE_NCURSES=true
# support of a configuration file
HAVE_RC_FILE=false
# can be disabled to make cdialog a bit smaller (could be done for more things)
HAVE_GUAGE=true
HAVE_TAILBOX=true

BINDIR = $(prefix)/bin
MANDIR = $(prefix)/man/man1

CC = gcc
CPP = gcc -E
OPTIM = -O2 -Wall -Wstrict-prototypes -fomit-frame-pointer -pipe
#OPTIM = -O -Wall -Wstrict-prototypes -g -pipe

# end of the configuration part
#----------------------------------------------------------------------------
# do not edit below this line

CFLAGS = $(OPTIM) -DLOCALE -DVERSION=\"0.9a\"
LDFLAGS = -L .
LDLIBS = -ldialog -lm

OBJS = checklist.o inputbox.o menubox.o msgbox.o \
	 textbox.o util.o yesno.o mousewget.o

ifeq ($(HAVE_GPM_SUPPORT), true)
LDLIBS+=-lgpm
CFLAGS+=-DHAVE_LIBGPM
OBJS+=mouse.o
endif
ifeq ($(HAVE_NCURSES), true)
CFLAGS+=-DHAVE_NCURSES -I/usr/include/ncurses
LDLIBS+=-lncurses
endif
ifeq ($(HAVE_RC_FILE), true)
CFLAGS+=-DHAVE_RC_FILE
OBJS+=rc.o
endif
ifeq ($(HAVE_GUAGE), true)
OBJS+=guage.o
CFLAGS+=-DHAVE_GUAGE
endif
ifeq ($(HAVE_TAILBOX), true)
OBJS+=tailbox.o
CFLAGS+=-DHAVE_TAILBOX
endif

SRCS = $(OBJS:.o=.c)

all: libdialog.a dialog

libdialog.a: $(OBJS)
	ar rcs libdialog.a $(OBJS)

dialog: $(OBJS) dialog.o

clean:
	rm -f core *.o *~ dialog libdialog.a
	:> .depend

include .depend

install: dialog libdialog.a
	install -s dialog $(BINDIR)
	install -m 644 dialog.1 $(MANDIR)
#	install -m 644 libdialog.a /usr/lib/

depend:
	$(CPP) -M $(CFLAGS) $(SRCS) > .depend

