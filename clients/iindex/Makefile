
# __ILIB_SITE_SETTINGS__
# -- Do not edit these parameters here.
# -- Edit the toplevel makefile and then 'make makefiles'
CC              = cc
CFLAGS          = -g -Wall -DHAVE_GIFLIB -DHAVE_PNGLIB -DHAVE_JPEGLIB -I/usr/local/include -I/usr/local/include/giflib
RANLIB          = ranlib
STATIC          = yes
DYNAMIC         = yes
LIBS            = -L/usr/local/lib -lgif -lpng -ljpeg 
MAJVERSION      = 1
MINVERSION      = 1
PATCHLEVEL      = 8
VERSION         = 1.1.8
SOVERSION       = 1.1
# Install locations
PREFIX          = /usr/local
BINDIR          = /usr/local/bin
LIBDIR          = /usr/local/lib
MANDIR          = /usr/local/man
DOCDIR          = /usr/local/share/Ilib
DATDIR          = /usr/local/share/Ilib
FNTDIR          = /usr/local/share/Ilib/bdf
# Install commands
INSTALL         = install
INSTALL_PROGRAM = install -s -o bin  -g bin -m 00751
INSTALL_DOC     = install -o bin -g bin -m 00644
INSTALL_DATA    = install -o bin -g bin -m 00644
INSTALL_FONT    = install -o bin -g bin -m 00644
INSTALL_SCRIPT  = install -o bin -g bin -m 00755
# __END_ILIB_SITE_SETTINGS__


INCLIB		= -L../../src -lIlib
ILIB		= ../../src/libIlib.a
SILIB		= ../../src/libIlib.so.${SOVERSION}
INCLUDES	= -I../../include
FONT2H		= ../ifont2h/ifont2h

all: helvR08.h iindex

static:	${ILIB} helvR08.h
	$(CC) iindex.o $(LIBS) -lm $(ILIB) -o iindex

dynamic: ${SILIB} helvR08.h
	$(CC) iindex.o $(LIBS) -lm $(SILIB) -o iindex

iindex: helvR08.h iindex.o

helvR08.h: $(FONT2H) ../../fonts/helvR08.bdf
	$(FONT2H) ../../fonts/helvR08.bdf > $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c -o $@

iindex: iindex.o helvR08.h
	@if [ "${DYNAMIC}." = "yes." ]; then \
	  make "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
		"STATIC=${STATIC}" "DYNAMIC=${DYNAMIC}" \
		"LIBS=${LIBS}" "SOVERSION=${SOVERSION}" dynamic; \
	else \
	  make "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
		"STATIC=${STATIC}" "DYNAMIC=${DYNAMIC}" \
		"LIBS=${LIBS}" "SOVERSION=${SOVERSION}" static; \
	fi

install: iindex
	@echo "Installing 'iindex' in $(BINDIR)"; \
	$(INSTALL_PROGRAM) iindex $(BINDIR)

clean:
	@echo "Cleaning in `pwd`"
	rm -f *.o iindex *core helvR08.h out.* *.html *.jpg

makefiles: makefile

makefile:
	@echo "Updating `pwd`/Makefile"; \
	if [ "$(SITE_DEF_FILE)." = "." ]; then \
		echo "You must run 'make makefiles' from the toplevel Makefile."; \
		exit 1; \
	fi; \
	sed -n '1,/^# __ILIB_SITE_SETTINGS__/ p' Makefile > Makefile.new; \
	cat $(SITE_DEF_FILE) >> Makefile.new; \
	sed -n '/^# __END_ILIB_SITE_SETTINGS__/,/^# START DEPENDENCIES/ p' Makefile >> Makefile.new; \
	$(CC) $(INCLUDES) -M *.c >> Makefile.new; \
	mv Makefile Makefile.bak; \
	mv Makefile.new Makefile


# START DEPENDENCIES
