all: libzc.a libzc_coroutine.a

include OBJS_DEST/depend
-include makefiles/defined.include

CC ?= gcc
CPP ?= g++

FLAGS := -fPIC -shared -Wall -Winline -I./ -O2 -g -ggdb -D___ZC_DEV_MODE___ $(EXTRA_CFLAGS)
CFLAGS := -std=gnu99 $(FLAGS)
CPPFLAGS := -std=gnu++11 $(FLAGS) -D___ZC_ZCC_MODE___

SRCS_C := ${shell find src -type f -name "*.c"}
OBJS_C := $(patsubst %.c, OBJS_DEST/%.o, $(SRCS_C))

SRCS_CPP := ${shell find src -type f -name "*.cpp"|grep -v "^src/coroutine/"}
OBJS_CPP := $(patsubst %.cpp, OBJS_DEST/%.o, $(SRCS_CPP))

SRCS_COROUTINE := ${shell find src/coroutine -type f -name "*.c"}
OBJS_COROUTINE := $(patsubst %.c, OBJS_DEST/%.o, $(SRCS_COROUTINE))

SRCS_ZC := ${shell find src -type f -name "*.c"|grep -v "^src/coroutine/"}
OBJS_ZC := $(patsubst %.c, OBJS_DEST/%.o, $(SRCS_ZC))

SRCS_ZCC := ${shell find src -type f -name "*.cpp"|grep -v "^src/coroutine/"}
OBJS_ZCC := $(patsubst %.cpp, OBJS_DEST/%.o, $(SRCS_ZCC))

$(OBJS_C):OBJS_DEST/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS_CPP):OBJS_DEST/%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

libzc.a: $(OBJS_ZC) $(OBJS_ZCC)
	ar r libzc.a $(OBJS_ZC) $(OBJS_ZCC)
	ranlib libzc.a

libzc_coroutine.a: $(OBJS_COROUTINE)
	ar r libzc_coroutine.a $(OBJS_COROUTINE)
	ranlib libzc_coroutine.a

