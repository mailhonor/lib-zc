all: target

.PHONY: tags

-include ../../makefiles/defined.include

GCC ?= gcc
GPP ?= g++

FLAGS := -ggdb -Wall -Winline -I../../ -O2 $(EXTRA_CFLAGS)
GCCFLAGS := -std=gnu99 $(FLAGS)
GPPFLAGS := -std=gnu++11 $(FLAGS)

GLOBAL_LIBS :=

ALL_LIBS := 

SRCS_GCC=$(wildcard *.c)
DEST_GCC := $(SRCS_GCC:%.c=%)
$(DEST_GCC):%:%.c
	$(GCC) $*.c -o $* $(GCCFLAGS) $($@_LIB) $(PREFIX_LIBS) ../../libzc.a $(SUFFIX_LIBS) $(LIB_$@) $(GLOBAL_LIBS)

SRCS_GPP=$(wildcard *.cpp)
DEST_GPP := $(SRCS_GPP:%.cpp=%)
$(DEST_GPP):%:%.cpp
	$(GPP) $*.cpp -o $* $(GPPFLAGS) $($@_LIB) $(PREFIX_LIBS) ../../libzc.a $(SUFFIX_LIBS) $(LIB_$@) $(GLOBAL_LIBS)

$(DEST_GCC) $(DEST_GPP): ../../libzc.a ../../libzc_coroutine.a ../../zc.h 

target: libzc $(DEST_GCC) $(DEST_GPP)

clean: CLEAN_WORKER

cleanFromTop: CLEAN_WORKER

CLEAN_WORKER:
	rm -f *~
	rm -f $(DEST_GCC) $(DEST_GPP)
	rm -f tags gmon.out
	rm -rf $(DELS)
	find -type f -name "*~" -exec rm {} \;
	@echo ""

libzc:
	@echo build global lib
	@cd ../../; make libzc

tag tags:
	cd ../../; make tags

targetFromTop: $(DEST_GCC) $(DEST_GPP)

