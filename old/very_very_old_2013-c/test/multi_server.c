#include <zyc.h>
#include <test.h>
#include "test_lib.c"

static ZSTR *rbuf=0;
static ZSTR *wbuf=0;


static void service_run(int fd, ZSTREAM *zsm){

	zmsg_debug("one service_run");
	zstream_fprintf(zsm, "your can input one string\n");
	ZSTREAM_FFLUSH(zsm);

	zstream_get_delimiter(zsm, rbuf, '\n');

	zstream_fprintf(zsm, "your input : %s\n", zstr_str(rbuf));
	ZSTREAM_FFLUSH(zsm);

	if(!strncmp(zstr_str(rbuf), "exit", 4)){
		zmaster_multi_server_disconnect(fd, zsm);
	}
	return;
}

int main(int argc, char **argv)
{
	zvar_program_name=argv[0];
	zmsg_set_output(test_zmsg_output_syslog);
	
	rbuf=zstr_create(1024);
	wbuf=zstr_create(1024);

	zmaster_multi_server(argc, argv, 0, service_run);
	return(0);
}
