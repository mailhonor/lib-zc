all: libzc.a libzc_coroutine.a

include OBJS_DEST/depend
include makefiles/defined.include

CC=gcc

CFLAGS= -std=gnu11 -Wall -Winline -I./ -O3 -g -ggdb $(EXTRA_MODULES)

SRCS=${shell find src -type f -name "*.c"}

OBJS_DEST = $(patsubst %.c, OBJS_DEST/%.o, $(SRCS))

SRCS_COROUTINE=${shell find src/coroutine -type f -name "*.c"}
OBJS_COROUTINE = $(patsubst %.c, OBJS_DEST/%.o, $(SRCS_COROUTINE))

SRCS_ZC=${shell find src -type f -name "*.c"|grep -v "^src/coroutine/"}
OBJS_ZC = $(patsubst %.c, OBJS_DEST/%.o, $(SRCS_ZC))

OBJS_DEST/%.o: %.c
	@echo build $<
	$(CC) $(CFLAGS) -c $< -o $@

libzc.a: $(OBJS_ZC)
	@echo build libzc.a
	@ar r libzc.a $(OBJS_ZC)
	@ranlib libzc.a

libzc_coroutine.a: $(OBJS_COROUTINE)
	@echo build libzc_coroutine.a
	@ar r libzc_coroutine.a $(OBJS_COROUTINE)
	@ranlib libzc_coroutine.a

