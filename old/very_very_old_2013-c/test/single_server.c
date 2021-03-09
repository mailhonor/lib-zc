#include <zyc.h>
#include "test_lib.c"

static int loop_times=0;
static ZSTR *rbuf=0;
static ZSTR *wbuf=0;


static int interval_do(int rwe, int fd, char *ctx){
	loop_times++;
	zevent_request_timer(-1, interval_do, ctx, Z_CHAR_PTR_TO_INT(ctx));
	return 0;
}

static void service_init(void){
	interval_do(0, -1, Z_INT_TO_CHAR_PTR(30));
	rbuf=zstr_create(1024);
	wbuf=zstr_create(1024);

}

static void service_run(int fd, ZSTREAM *zsm){

	while(1){
		zstream_fprintf(zsm, "%d, your can input one string\n", loop_times);
		ZSTREAM_FFLUSH(zsm);
		zstream_get_delimiter(zsm, rbuf, '\n');
		zstream_fprintf(zsm, "%d, your input : %s\n", loop_times, zstr_str(rbuf));
		ZSTREAM_FFLUSH(zsm);
		if(!strncmp(zstr_str(rbuf), "exit", 4)){
			break;
		}
	}
	return;
}

int main(int argc, char **argv)
{
	zvar_program_name=argv[0];
	zmsg_set_output(test_zmsg_output_syslog);
	
	zmaster_single_server(argc, argv, service_init, service_run);
	return(0);
}
