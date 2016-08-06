
#CC = gcc
#CFLAGS = -g -Wall
#RANLIB = ranlib

SUBDIRS = ifont2h iwebreprt ifraggraph idisplayfont iindex

all:	build

build:
	@echo "Building all in `pwd`"; \
	for i in $(SUBDIRS); \
	do \
	  echo "======> $$i"; \
	  ( cd $$i; make ) \
	done

install:
	@echo "Installing from `pwd`"; \
	for i in $(SUBDIRS); \
	do \
	  echo "======> $$i"; \
	  ( cd $$i; make install ) \
	done
	
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
	  ( cd $$i; make makefiles ) \
	done

makefile:

