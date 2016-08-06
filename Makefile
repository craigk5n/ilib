# Ilib toplevel Makefile
#
# See README for build/install information.
#
# See COPYING for license information.
# Ilib Home Page:
#	http://ilib.sourceforge.net
#
# Set the following and then run "make all"

# You can get GIFLIB at:
#	http://prtr-13.ucsc.edu/~badger/software/libungif.shtml
# You can get libpng (as well as zlib)  at:
#	ftp://ftp.uu.net/graphics/png/src/
# Get libjpeg at:
#	ftp://ftp.uu.net/graphics/jpeg/
#
# ###############
#   BUILD CONFIG
# ###############

#   Build .a static library if yes
STATIC = yes


#   Build .so shared library if yes
DYNAMIC = no


#   Lib path may need correction depending upon platform
LIBS = -L/usr/local/lib -lgif -lpng -ljpeg 


#   -DHAVE_GIFLIB  if using -lgif above
#   -DHAVE_PNGLIB  if using -lpng above
#   -DHAVE_JPEGLIB if using -ljpeg above
#   -DHAVE_TCL     if using -ltcl{vv}
#   -DHAVE_TK      if usint -ltk{xx}
DEFINES = -DHAVE_GIFLIB -DHAVE_PNGLIB -DHAVE_JPEGLIB


#   Include path may need modification depending upon platform
#   (Use only absolute paths here)
INCLUDES = -I/usr/local/include -I/usr/local/include/giflib

#  INSTALL CONFIG

#   Compress Manpages if defined, use commnd after install
COMPRESS = gzip -9v

#   Install Locations
PREFIX = /usr/local
BINDIR = ${PREFIX}/bin
LIBDIR = ${PREFIX}/lib
MANDIR = ${PREFIX}/man
DOCDIR = ${PREFIX}/share/Ilib
DATDIR = ${PREFIX}/share/Ilib
FNTDIR = ${DATDIR}/bdf
#   Install Commands (e.g. "install" for BSD install)
INSTALL         = install
INSTALL_PROGRAM = ${INSTALL} -s -o bin  -g bin -m 00751
INSTALL_DOC     = ${INSTALL} -o bin -g bin -m 00644
INSTALL_DATA    = ${INSTALL} -o bin -g bin -m 00644
INSTALL_FONT    = ${INSTALL} -o bin -g bin -m 00644
INSTALL_SCRIPT  = ${INSTALL} -o bin -g bin -m 00755

#   If we've installed with DYNAMIC above, we want
#   to merge in the newly installed shared library
#   with ldconfig
#   If we want X to also use the installed fonts, we
#   may wish to:       xset fn+ ${FNTDIR}
#   We may wish to gzip them first and do a mkfontdir
POST_INSTALL = echo
#POST_INSTALL = ldconfig -m ${LIBDIR}

# Definitions for your compiler and make (do you have system defined CFLAGS?)
CC = cc
CFLAGS = -g -Wall $(DEFINES) $(INCLUDES)
RANLIB = ranlib

######################################################################
MAJVERSION = 1
MINVERSION = 1
PATCHLEVEL = 9
VERSION = ${MAJVERSION}.${MINVERSION}.${PATCHLEVEL}
SOVERSION = ${MAJVERSION}.${MINVERSION}
######################################################################
# No need to edit anything beyond here
DIST = Ilib-${VERSION}

SUBDIRS = include src clients examples fonts

all:	build

build:
	@echo "Building all"; \
	for i in ${SUBDIRS}; \
	do \
	  echo "===> Entering $$i for build"; \
	  ( cd $$i; make "MAKEFLAGS=${MAKEFLAGS}" "CC=${CC}" \
		"CFLAGS=${CFLAGS}" "RANLIB=${RANLIB}" \
		"LIBS=${LIBS}" "STATIC=${STATIC}" "DYNAMIC=${DYNAMIC}" \
		"VERSION=${VERSION}" "SOVERSION=${SOVERSION}" ) \
	done;

install:
	@echo "Installing all"; \
	for i in ${SUBDIRS}; \
	do \
	  echo "===> Entering $$i for install"; \
          ( cd $$i; make "MAKEFLAGS=${MAKEFLAGS}" "DEFINES=${DEFINES}" \
		"BINDIR=${BINDIR}" "LIBDIR=${LIBDIR}" "MANDIR=${MANDIR}" \
		"DOCDIR=${DOCDIR}" "DATDIR=${DATDIR}" "FNTDIR=${FONTDIR}" \
		"VERSION=${VERSION}" "INSTALL_PROGRAM=${INSTALL_PROGRAM}" \
		"INSTALL_DOC=${INSTALL_DOC}" "INSTALL_DATA=${INSTALL_DATA}" \
		"SOVERSION=${SOVERSION}" "INSTALL_FONT=${INSTALL_FONT}" \
		"INSTALL_SCRIPT=${INSTALL_SCRIPT}" install ) \
	done;
	@if [ ".${POST_INSTALL}" != "." ]; then \
	   echo "Post Install Processing"; \
	   ${POST_INSTALL}; \
	fi
	
clean:
	@echo "Cleaning..."; \
	rm -f *core *.tar *.zip *.tar.gz *.Z filelist.txt filelist2.txt
	for i in $(SUBDIRS); \
	do \
	  echo "==> Cleaning in $$i"; \
	  ( cd $$i; make "SOVERSION=${SOVERSION}" clean ) \
	done


makefiles:
	@echo "Updating makefiles in `pwd`"; \
	temp="/tmp/ilib-options.dat"; \
	echo "Saving options in $$temp"; \
	echo "# -- Do not edit these parameters here." > $$temp; \
	echo "# -- Edit the toplevel makefile and then 'make makefiles'" >> $$temp; \
	echo "CC              = $(CC)" >> $$temp; \
	echo "CFLAGS          = $(CFLAGS)" >> $$temp; \
	echo "RANLIB          = $(RANLIB)" >> $$temp; \
	echo "STATIC          = $(STATIC)" >> $$temp; \
	echo "DYNAMIC         = $(DYNAMIC)" >> $$temp; \
	echo "LIBS            = $(LIBS)" >> $$temp; \
	echo "MAJVERSION      = $(MAJVERSION)" >> $$temp; \
	echo "MINVERSION      = $(MINVERSION)" >> $$temp; \
	echo "PATCHLEVEL      = $(PATCHLEVEL)" >> $$temp; \
	echo "VERSION         = $(VERSION)" >> $$temp; \
	echo "SOVERSION       = $(SOVERSION)" >> $$temp; \
	echo "# Install locations" >> $$temp; \
	echo "PREFIX          = $(PREFIX)" >> $$temp; \
	echo "BINDIR          = $(BINDIR)" >> $$temp; \
	echo "LIBDIR          = $(LIBDIR)" >> $$temp; \
	echo "MANDIR          = $(MANDIR)" >> $$temp; \
	echo "DOCDIR          = $(DOCDIR)" >> $$temp; \
	echo "DATDIR          = $(DATDIR)" >> $$temp; \
	echo "FNTDIR          = $(FNTDIR)" >> $$temp; \
	echo "# Install commands" >> $$temp; \
	echo "INSTALL         = $(INSTALL)" >> $$temp; \
	echo "INSTALL_PROGRAM = $(INSTALL_PROGRAM)" >> $$temp; \
	echo "INSTALL_DOC     = $(INSTALL_DOC)" >> $$temp; \
	echo "INSTALL_DATA    = $(INSTALL_DATA)" >> $$temp; \
	echo "INSTALL_FONT    = $(INSTALL_FONT)" >> $$temp; \
	echo "INSTALL_SCRIPT  = $(INSTALL_SCRIPT)" >> $$temp; \
	for i in $(SUBDIRS); \
	do \
	  ( cd $$i; make "SITE_DEF_FILE=$$temp" makefiles ) \
	done

makefile:

mindist:
	@make clean; \
	rm -f filelist.txt filelist2.txt $(DIST).tar.gz $(DIST).zip $(DIST).Z; \
	find $(DIST)/* -type f \! -name "*,v" -print | grep -v bak | grep -v 'perl/' | grep -v filelist | grep -v CVS | grep -v '\/\.' > filelist.txt; \
	grep -v fonts filelist.txt > filelist2.txt; \
	grep "fonts/Makefile" filelist.txt >> filelist2.txt; \
	grep "fonts/helvR24.bdf" filelist.txt >> filelist2.txt; \
	grep "fonts/helvB18.bdf" filelist.txt >> filelist2.txt; \
	grep "fonts/helvR08.bdf" filelist.txt >> filelist2.txt; \
	grep "fonts/courR10.bdf" filelist.txt >> filelist2.txt; \
	echo "$(DIST)/perl/README" >> filelist2.txt; \
	echo "$(DIST)/perl/Ilib.xs" >> filelist2.txt; \
	echo "$(DIST)/perl/Ilib.pm" >> filelist2.txt; \
	echo "$(DIST)/perl/Makefile.PL" >> filelist2.txt; \
	echo "$(DIST)/perl/test.pl" >> filelist2.txt; \
	echo "$(DIST)/perl/Changes" >> filelist2.txt; \
	echo "$(DIST)/perl/MANIFEST" >> filelist2.txt; \
	echo "$(DIST)/perl/typemap" >> filelist2.txt; \
	echo "$(DIST)/perl/xstmp.c" >> filelist2.txt; \
	echo "Building minimum tar file"; \
	tar cvf $(DIST).tar `cat filelist2.txt`; \
	echo "Compressing (gzip)"; \
	gzip -c < $(DIST).tar > $(DIST)-min.tar.gz ; \
	echo "Zipping"; \
	zip $(DIST)-min.zip `cat filelist2.txt`; \
	rm -f $(DIST).tar filelist.txt filelist2.txt

dist:
	@make clean; \
	rm -f filelist.txt filelist2.txt $(DIST).tar.gz $(DIST).zip $(DIST).Z; \
	echo "Build full distribution"; \
	find $(DIST)/* -type f \! -name "*,v" -print | grep -v bak | grep -v 'perl/' | grep -v CVS | grep -v '\/\.' > filelist.txt; \
	echo "Building full tar file"; \
	tar cvf $(DIST).tar `cat filelist.txt`; \
	grep -v fonts filelist.txt > filelist2.txt; \
	grep "fonts/Makefile" filelist.txt >> filelist2.txt; \
	grep "fonts/helvR24.bdf" filelist.txt >> filelist2.txt; \
	grep "fonts/helvB18.bdf" filelist.txt >> filelist2.txt; \
	grep "fonts/helvR08.bdf" filelist.txt >> filelist2.txt; \
	grep "fonts/courR10.bdf" filelist.txt >> filelist2.txt; \
	echo "$(DIST)/perl/README" >> filelist2.txt; \
	echo "$(DIST)/perl/Ilib.xs" >> filelist2.txt; \
	echo "$(DIST)/perl/Ilib.pm" >> filelist2.txt; \
	echo "$(DIST)/perl/Makefile.PL" >> filelist2.txt; \
	echo "$(DIST)/perl/test.pl" >> filelist2.txt; \
	echo "$(DIST)/perl/Changes" >> filelist2.txt; \
	echo "$(DIST)/perl/MANIFEST" >> filelist2.txt; \
	echo "$(DIST)/perl/typemap" >> filelist2.txt; \
	echo "$(DIST)/perl/xstmp.c" >> filelist2.txt; \
	echo "Compressing (gzip)"; \
	gzip -c < $(DIST).tar > $(DIST)-full.tar.gz ; \
	echo "Zipping"; \
	zip $(DIST)-full.zip `cat filelist.txt`; \
	rm -f $(DIST).tar; \
	echo "Building minimum tar file"; \
	tar cvf $(DIST).tar `cat filelist2.txt`; \
	echo "Compressing (gzip)"; \
	gzip -c < $(DIST).tar > $(DIST)-min.tar.gz ; \
	echo "Zipping"; \
	zip $(DIST)-min.zip `cat filelist2.txt`; \
	rm -f $(DIST).tar; \
	grep fonts filelist.txt > filelist2.txt; \
	echo "Building fonts tar file"; \
	tar cvf $(DIST).tar `cat filelist2.txt`; \
	echo "Compressing (gzip)"; \
	gzip -c < $(DIST).tar > $(DIST)-fonts.tar.gz ; \
	echo "Zipping"; \
	zip $(DIST)-fonts.zip `cat filelist2.txt`; \
	rm -f $(DIST).tar filelist.txt filelist2.txt

