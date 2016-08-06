# lib Makefile

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
ILIB = libIlib.a
SILIB = libIlib.so.${SOVERSION}
HTML = ../Ilib.html

all:
	@if [ "${STATIC}." = "yes." ]; then \
	   echo "Building for ${ILIB}"; \
	   make "MAKEFLAGS=${MAKEFLAGS}" "CC=${CC}" \
		"CFLAGS=${CFLAGS}" "RANLIB=${RANLIB}" \
		"LIBS=${LIBS}" "STATIC=${STATIC}" "DYNAMIC=${DYNAMIC}" \
		"VERSION=${VERSION}" "SOVERSION=${SOVERSION}" ${ILIB}; \
	fi
	@if [ "${DYNAMIC}." = "yes." ]; then \
	  echo "Building for ${SILIB}"; \
	  make "MAKEFLAGS=${MAKEFLAGS}" "CC=${CC}" \
		"CFLAGS=${CFLAGS}" "RANLIB=${RANLIB}" \
		"LIBS=${LIBS}" "STATIC=${STATIC}" "DYNAMIC=${DYNAMIC}" \
		"VERSION=${VERSION}" "SOVERSION=${SOVERSION}" ${SILIB}; \
	fi

.SUFFIXES: .out .o .O .po .so .s .S .c .cc .cxx .m .C .f .y .l
	

ILIBOBJS = \
	IArcProp.o IColor.o ICopyImg.o IDrawLin.o \
	IDrawPol.o IDrawArc.o IDrawCir.o IDrawEll.o \
	IDrawPt.o IDrawRec.o IDrawStr.o \
	IErrorStr.o IFileType.o IFlood.o \
	IFillPol.o IFillArc.o IFillCir.o IFillEll.o IFillRec.o \
	IFont.o IFontBDF.o \
	IGC.o IBMP.o IGIF.o IImage.o IJPEG.o IPNG.o IPGM.o IPPM.o ITextDim.o IXPM.o

ILIBSOBJS = \
	IArcProp.O IColor.O ICopyImg.O IDrawLin.O \
	IDrawPol.O IDrawArc.O IDrawCir.O IDrawEll.O \
	IDrawPt.O IDrawRec.O IDrawStr.O \
	IErrorStr.O IFileType.O IFlood.O \
	IFillPol.O IFillArc.O IFillCir.O IFillEll.O IFillRec.O \
	IFont.O IFontBDF.O \
	IGC.O IBMP.o IGIF.O IImage.O IJPEG.O IPNG.O IPGM.O IPPM.O ITextDim.O IXPM.O

$(ILIB): $(ILIBOBJS) ${HTML}
	  ar -rc $@ $(ILIBOBJS);
	  $(RANLIB) $(ILIB);

${SILIB}: ${ILIBSOBJS} ${HTML}
	  ld ${ILIBSOBJS} -Bshareable -o $@;

$(HTML): Ilib.h h2html.pl
	perl h2html.pl Ilib.h > $@

install:
	@echo "Install not done from `pwd`"

clean:
	@echo "Cleaning in `pwd`"
	rm -f $(ILIBOBJS) ${ILIBSOBJS} $(ILIB) ${SILIB} *core

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

.c.o:
	${CC} ${CFLAGS} ${INCLUDES} -c $*.c -o $@

.c.O:
	${CC} -fpic -DPIC ${CFLAGS} ${INCLUDES} -c $*.c -o $@


# START DEPENDENCIES
