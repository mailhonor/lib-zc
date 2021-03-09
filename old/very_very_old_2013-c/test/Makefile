all: testall

CFLAGS= -g -Os -Wall -DZYC_DEV
LIBS= -lzyc-dev -ldb

SRC=$(wildcard *.c)
OBJS := $(SRC:.c=)
$(OBJS): /usr/local/include/zyc.h /usr/local/lib/libzyc-dev.so

.c:
	gcc $*.c $(CFLAGS) $(LIBS) -o $*

testall: $(OBJS)


clean:
	rm -f  $(OBJS)
