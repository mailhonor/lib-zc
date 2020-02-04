${shell mkdir -p uploaded_files; }
DELS=uploaded_files

LIB_coroutine_httpd = ../../libzc_coroutine.a -lssl -lcrypto -lpthread
LIB_no_ssl_coroutine_httpd = ../../libzc_coroutine.a -lpthread
LIB_general_coroutine_server_httpd = ../../libzc_coroutine.a -lssl -lcrypto -lpthread

LIB_simple_httpd = -lssl -lcrypto
LIB_no_sslsimple_httpd =

include ../../makefiles/sample.makefile

$(DEST): ./lib_httpd.h
no_ssl_coroutine_httpd: coroutine_httpd.c
no_ssl_simple_httpd: simple_httpd.c
