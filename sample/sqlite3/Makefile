include ../../makefiles/sample.makefile

my_extra_cflags :=
ifneq ($(findstring $EXTRA_CFLAGS, -D_LIB_ZC_SQLITE3_), "")
	my_extra_cflags := -lsqlite3
endif
SUFFIX_LIBS := -lpthread $(my_extra_cflags)
