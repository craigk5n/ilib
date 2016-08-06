
SUBDIRS = isample iconvert

all:	build

build:
	@echo "Building all in `pwd`"; \
	for i in $(SUBDIRS); \
	do \
	  echo "======> $$i"; \
	  ( cd $$i; make ) \
	done

install:
	@echo "Example do not get installed"
	
clean:
	@echo "Cleaning in `pwd`"; \
	for i in $(SUBDIRS); \
	do \
	  ( cd $$i; make clean ) \
	done


makefiles:
	@echo "Updating makefiles in `pwd`..."; \
	for i in $(SUBDIRS); \
	do \
	  ( cd $$i; make "SITE_DEF_FILE=$(SITE_DEF_FILE)" makefiles ) \
	done

makefile:

