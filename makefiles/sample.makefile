all: target

.PHONY: tags

-include ../../makefiles/defined.include

CC ?= gcc
CPP ?= g++

FLAGS := -std=gnu99 -ggdb -Wall -Winline -I../../ -O2 $(EXTRA_CFLAGS)
CFLAGS := -std=gnu99 $(FLAGS)
CPPFLAGS := -std=gnu++11 $(FLAGS)

GLOBAL_LIBS :=

ALL_LIBS := 

SRCS_C=$(wildcard *.c)
DEST_C := $(SRCS_C:%.c=%)
$(DEST_C):%:%.c
	$(CC) $*.c -o $* $(CFLAGS) $($*_LIB) $(PREFIX_LIBS) ../../libzc.a $(SUFFIX_LIBS) $(LIB_$*) $(GLOBAL_LIBS)

SRCS_CPP=$(wildcard *.cpp)
DEST_CPP := $(SRCS_CPP:%.cpp=%)
$(DEST_CPP):%:%.cpp
	$(CPP) $*.cpp -o $* $(CPPFLAGS) $($*_LIB) $(PREFIX_LIBS) ../../libzc.a $(SUFFIX_LIBS) $(LIB_$*) $(GLOBAL_LIBS)

$(DEST_C) $(DEST_CPP): ../../libzc.a ../../libzc_coroutine.a ../../zc.h 

target: libzc $(DEST_C) $(DEST_CPP)

clean: CLEAN
	@echo clean

CLEAN:
	rm -f *~; rm -f $(DEST_C) $(DEST_CPP); rm -f tags gmon.out;rm -rf $(DELS);
	find -type f -name "*~" -exec rm  {} \;

libzc:
	@echo build global lib
	@cd ../../; make libzc

tag tags:
	cd ../../; make tags

targetFromTop: $(DEST_C) $(DEST_CPP)
