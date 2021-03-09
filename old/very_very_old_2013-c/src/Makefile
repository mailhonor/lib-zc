all:libzyc-dev.so

CFLAGS= -g -Os -Wall -fPIC -shared -DZYC_INUSE -DZYC_DEV

SRC=$(wildcard *.c)
OBJS := $(SRC:.c=.o)
$(OBJS): zyc.h

.c.o:
	gcc -c $*.c $(CFLAGS)  -o $*.o


libzyc-dev.so: zyc.h $(OBJS)
	gcc $(CFLAGS) -o libzyc-dev.so $(OBJS)
	cp -a ./libzyc-dev.so /usr/local/lib/
	cp -a ./zyc.h /usr/local/include/


clean:
	rm -f libzyc-dev.so
	rm -f *.o
