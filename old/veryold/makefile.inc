
ifeq ($(LIBZC_DEV), YES)
	include makefile.srclist
else
	SRC=${shell find src -type f -name "*.c"}
endif

