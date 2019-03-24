all: target

.PHONY: tags

CC=gcc
CFLAGS= -ggdb -Wall -I../../ -O3
GLOBAL_LIBS=

SRCS=${shell find -type f -name "*.c"}
DEST := $(SRCS:.c=)
.c:
	$(CC) $*.c -o $* $(CFLAGS) ../../libzc.a $(GLOBAL_LIBS) $(LIBS) $($*_LIB)


$(DEST): ../../libzc.a ../../zc.h

target: libzc $(DEST)

clean: CLEAN
	@echo clean

CLEAN:
	@rm -f *~; rm -f $(DEST); rm -f tags gmon.out;rm -rf $(DELS);

libzc:
	@echo build global lib
	@cd ../../; make libzc

tag tags:
	cd ../../; make tags

targetFromTop: $(DEST)
