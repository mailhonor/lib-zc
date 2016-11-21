all: libzc.a

include OBJS_DEST/depend
include makefiles/depend.static

CC=gcc

CFLAGS= -Wall -I./ -O3 -ggdb

SRCS=${shell find src -type f -name "*.c"}

OBJS_DEST = $(patsubst %.c, OBJS_DEST/%.o, $(SRCS))

OBJS_DEST/%.o: %.c libzc.h
	@echo build $<
	@$(CC) $(CFLAGS) -c $< -o $@

libzc.a: $(OBJS_DEST)
	@echo build libzc.a
	@ar r libzc.a $(OBJS_DEST)
	@ranlib libzc.a

