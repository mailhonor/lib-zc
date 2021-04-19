all: libzc.a libzc_coroutine.a

include .depend
-include makefiles/defined.include

GCC := gcc
GPP := g++

FLAGS := -fPIC -shared -Wall -Winline -I./ -O2 -g -ggdb -D___ZC_DEV_MODE___ $(EXTRA_CFLAGS)
GCCFLAGS := -std=gnu99 $(FLAGS)
GPPFLAGS := -std=gnu++11 $(FLAGS) -D___ZC_ZCC_MODE___

SRCS_GCC := ${shell find src cpp_src -type f -name "*.c"}
OBJS_GCC := $(patsubst %,%.o, $(SRCS_GCC))

SRCS_GPP := ${shell find src cpp_src -type f -name "*.cpp"}
OBJS_GPP := $(patsubst %,%.o, $(SRCS_GPP))

SRCS_COROUTINE := ${shell find src/coroutine -type f -name "*.c"}
OBJS_COROUTINE := $(patsubst %,%.o, $(SRCS_COROUTINE))

OBJS_ZC := $(filter-out src/coroutine/%o, $(OBJS_GCC))
OBJS_ZCC := $(filter-out src/coroutine/%.o, $(OBJS_GPP))

$(OBJS_GCC):%.o: %
	$(GCC) $(GCCFLAGS) -c $< -o $@

$(OBJS_GPP):%.o: %
	$(GPP) $(GPPFLAGS) -c $< -o $@

libzc.a: $(OBJS_ZC) $(OBJS_ZCC)
	ar r libzc.a $(OBJS_ZC) $(OBJS_ZCC)
	ranlib libzc.a

libzc_coroutine.a: $(OBJS_COROUTINE)
	ar r libzc_coroutine.a $(OBJS_COROUTINE)
	ranlib libzc_coroutine.a

