${shell mkdir -p libexec log_dir ; }
DELS=master.pid libexec/ log.socket log_dir
SUFFIX_LIBS=-lssl -lcrypto -lpthread
coroutine_server_LIB=../../libzc_coroutine.a -static
include ../../makefiles/sample.makefile
