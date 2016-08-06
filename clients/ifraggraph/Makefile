
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

all: helvB18.h courR10.h ifraggraph

ifraggraph: helvB18.h courR10.h ifraggraph.o

helvB18.h: $(FONT2H) ../../fonts/helvB18.bdf
	$(FONT2H) ../../fonts/helvB18.bdf > $@

courR10.h: $(FONT2H) ../../fonts/courR10.bdf
	$(FONT2H) ../../fonts/courR10.bdf > $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c -o $@

static: ${ILIB} ifraggraph.o
	${CC} ifraggraph.o ${LIBS} ${INCLIB} -lm -o ifraggraph

dynamic: ${SILIB} ifraggraph.o
	${CC} ifraggraph.o ${LIBS} ${INCLIB} -lm -o ifraggraph

ifraggraph: ifraggraph.o
	@if [ "${DYNAMIC}." = "yes." ]; then \
	  make "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
		"STATIC=${STATIC}" "DYNAMIC=${DYNAMIC}" \
		"LIBS=${LIBS}" "SOVERSION=${SOVERSION}" dynamic; \
	else \
	  make "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
		"STATIC=${STATIC}" "DYNAMIC=${DYNAMIC}" \
		"LIBS=${LIBS}" "SOVERSION=${SOVERSION}" static; \
	fi

install: ifraggraph
	@echo "Installing 'ifraggraph' in $(BINDIR)"; \
	$(INSTALL_PROGRAM) ifraggraph $(BINDIR)

clean:
	@echo "Cleaning in `pwd`"
	rm -f *.o ifraggraph *core helvB18.h courR10.h *.gif

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
