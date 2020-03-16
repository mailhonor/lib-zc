all: libzc.a libzc_coroutine.a

include OBJS_DEST/depend
include makefiles/defined.include

CC ?= gcc.sh
CFLAGS := -std=gnu99 -Wall -Winline -I./ -O2 -g -ggdb $(EXTRA_CFLAGS)

SRCS := ${shell find src -type f -name "*.c"}
OBJS_DEST := $(patsubst %.c, OBJS_DEST/%.o, $(SRCS))

SRCS_COROUTINE := ${shell find src/coroutine -type f -name "*.c"}
OBJS_COROUTINE := $(patsubst %.c, OBJS_DEST/%.o, $(SRCS_COROUTINE))

SRCS_ZC := ${shell find src -type f -name "*.c"|grep -v "^src/coroutine/"}
OBJS_ZC := $(patsubst %.c, OBJS_DEST/%.o, $(SRCS_ZC))

OBJS_DEST/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libzc.a: $(OBJS_ZC)
	ar r libzc.a $(OBJS_ZC)
	ranlib libzc.a

libzc_coroutine.a: $(OBJS_COROUTINE)
	ar r libzc_coroutine.a $(OBJS_COROUTINE)
	ranlib libzc_coroutine.a

include makefiles/special_src.include
SPECIAL_OBJS_DEST = $(patsubst %.c, OBJS_DEST/%.o, $(SPECIAL_SRC))
$(SPECIAL_OBJS_DEST):OBJS_DEST/%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

special_src: $(SPECIAL_OBJS_DEST)
