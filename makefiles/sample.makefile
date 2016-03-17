all: target

.PHONY: tags depend

${shell touch depend}
CC=gcc
CFLAGS= -ggdb -Wall -I../../ -O3
GLOBAL_LIBS= -lpthread -lrt -lssl -lcrypto
SRCS=${shell find -type f -name "*.c"}
DEST := $(SRCS:.c=)

$(DEST): ../../libzc.a $(MY_LIBS)
.c:
	$(CC) $*.c -o $* $(CFLAGS) ../../libzc.a $(MY_LIBS) $(GLOBAL_LIBS) $(LIBS)

dest: $(DEST)

target: depend libwow
	make dest MY_LIBS="$(MY_LIBS)" LIBS="$(LIBS)"

clean: CLEAN
	@echo clean

CLEAN:
	@rm -f *~; rm -f $(DEST); rm -f tags gmon.out depend;rm -rf $(DELS);

libwow:
	@echo build global lib
	@cd ../../; make libwow

tag tags:
	cd ../../; make tags

depend: *.c
	@$(CC) -E -MM *.c -I../../ | sed s/.o:/:/ > depend

targetFromTop: depend
	make dest MY_LIBS="$(MY_LIBS)" LIBS="$(LIBS)"

