all: target

.PHONY: tags

CC=gcc
CFLAGS= -std=gnu11 -ggdb -Wall -Winline -I../../ -O3
GLOBAL_LIBS=

ALL_LIBS = 
SRCS=${shell find -type f -name "*.c"}
DEST := $(SRCS:.c=)
.c:
	$(CC) $*.c -o $* $(CFLAGS) $($*_LIB) $(PREFIX_LIBS) ../../libzc.a $(SUFFIX_LIBS) $(LIB_$*) $(GLOBAL_LIBS)


$(DEST): ../../libzc.a ../../libzc_coroutine.a ../../zc.h

target: libzc $(DEST)

clean: CLEAN
	@echo clean

CLEAN:
	@rm -f *~; rm -f $(DEST); rm -f tags gmon.out;rm -rf $(DELS);
	@find -type f -name "*~" -exec rm  {} \;

libzc:
	@echo build global lib
	@cd ../../; make libzc

tag tags:
	cd ../../; make tags

targetFromTop: $(DEST)
