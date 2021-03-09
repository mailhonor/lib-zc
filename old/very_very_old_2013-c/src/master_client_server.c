#include "zyc.h"

#define _SINGLE_CONNECTION_READABLE_TIMEOUT	0

int zvar_ss_use_limit=0;
int zvar_ss_idle_timeout=0;
int zvar_ss_runtime_limit=0;
int zvar_ss_daemon_timeout=0;
int zvar_ss_connection_timeout=0;

static int z_server_type=0;
static long z_start_time=0;
static int z_use_count=0;
static int z_client_count=0;
static int z_service_type=0;
static void(*z_service)(int fd, ZSTREAM *stream);

static int zmaster_child_status_notify(int status){
	struct {
		int pid;
		int magic:24;
		int status:8;
	} _status;

	status=(status?1:0);
	_status.pid=getpid();
	_status.magic=0xbeefac;
	_status.status=(0xFF & status);

	if(write(ZMASTER_STATUS_FD, &_status, sizeof(_status))!=sizeof(_status)){
		return -1;
	}
	return 0;
}
static void zmaster_server_exit(void){
	exit(0);
}

static int zmaster_server_abort(int rwe, int fd, char *ctx){

	zmsg_debug("master disconnect -- exiting");
	zmaster_server_exit();
	return 0;
}

static int zmaster_server_timeout(int rwe, int fd, char *ctx){

	zmsg_debug("idle timeout -- exiting");
	zmaster_server_exit();
	return 0;
}

void zmaster_multi_server_disconnect(int fd, ZSTREAM *zsm){
	z_client_count--;
	z_use_count++;
	zevent_disable_readwrite(fd);
	zstream_fdclose(zsm);
#if _SINGLE_CONNECTION_READABLE_TIMEOUT
	zevent_cancel_timer(fd, zmaster_multi_server_excute_timeout, (char *)zsm);
#endif
	close(fd);

	if(z_client_count == 0 && zvar_ss_idle_timeout > 0){
		zevent_request_timer(-1, zmaster_server_timeout, 0, zvar_ss_idle_timeout);
	}
	zmsg_debug("zmaster_multi_server_disconnect");
}

#if _SINGLE_CONNECTION_READABLE_TIMEOUT
static int zmaster_multi_server_excute_timeout(int rwe, int fd, char *ctx){
	zmaster_multi_server_disconnect(fd, (ZSTREAM *)ctx);
}
#endif 

static int zmaster_multi_server_excute(int rwe, int fd, char *ctx){
	ZSTREAM *zsm;

	zmsg_debug("zmaster_multi_server_excute: rwe: %x", rwe);
	zsm=(ZSTREAM *)ctx;

	if(ZEVENT_IS_EXCEPTION(rwe)){
		zmaster_multi_server_disconnect(fd, zsm);
		return 0;
	}

	/* daemon timeout, none*/
	if(zmaster_child_status_notify(1)<0){
		zmaster_server_abort(0,0,0);
	}
	z_service(fd, zsm);
	if(zmaster_child_status_notify(0)<0){
		zmaster_server_abort(0,0,0);
	}
#if _SINGLE_CONNECTION_READABLE_TIMEOUT
	zevent_request_timer(fd, zmaster_multi_server_excute_timeout, (char *)zsm);
#endif

	return 0;
}

static int zmaster_server_accpet_accept(int listen_fd){
	int fd;

	if(z_server_type=='i'){
		fd=zsocket_inet_accept(listen_fd);
	}else if(z_server_type=='u'){
		fd=zsocket_unix_accept(listen_fd);
	}else if(z_server_type=='f'){
		fd=listen_fd;
	}else{
		fd=-1;
		zmsg_fatal("master service: wrong server type: %s", z_server_type);
	}
	if(fd < 0){
		if(errno !=EAGAIN){
			zmsg_fatal("accept connect: %m");
		}
		return -1;
	}
	return fd;
}
static int zmaster_multi_server_accpet(int rwe, int listen_fd, char *ctx){
	int fd;
	ZSTREAM *zsm;

	if(z_client_count == 0 && zvar_ss_idle_timeout > 0){
		zevent_cancel_timer(-1, zmaster_server_timeout, 0);
	}

	fd=zmaster_server_accpet_accept(listen_fd);
	if(fd<0){
		if(errno !=EAGAIN){
			zmsg_error("accpet connection: %m");
		}
		if(z_client_count == 0 && zvar_ss_idle_timeout > 0){
			zevent_request_timer(-1, zmaster_server_timeout, 0, zvar_ss_idle_timeout);
		}
		return fd;
	}
	
	z_client_count++;

	zio_set_nonblock(fd);
	zsm=zstream_fdopen(fd, zvar_ipc_timeout);

	zevent_enable_read(fd, zmaster_multi_server_excute, (char *)zsm);

#if _SINGLE_CONNECTION_READABLE_TIMEOUT
	zevent_request_timer(fd, zmaster_multi_server_excute_timeout, (char *)zsm);
#endif
	zmsg_debug("zmaster_multi_server_accpet");

	return 0;
}

static int zmaster_single_server_accpet(int rwe, int listen_fd, char *ctx){
	int fd;
	ZSTREAM *zsm;

	if((fd=zmaster_server_accpet_accept(listen_fd))<0){
		return fd;
	}

	z_use_count ++;
	if(zvar_ss_idle_timeout > 0)
		zevent_cancel_timer(-1, zmaster_server_timeout, 0);

	zio_set_block(fd);
	zsm=zstream_fdopen(fd, zvar_ipc_timeout);

	if(zmaster_child_status_notify(1)<0){
		zmaster_server_abort(0,0,0);
	}
	/* here, should add daemon timeout code */
	for(;;){
		z_service(fd, zsm);
		if(z_server_type!='f'){
			break;
		}
		if(!zstream_rbuf_left(zsm)){
			break;
		}
	}
	if(zmaster_child_status_notify(0)<0){
		zmaster_server_abort(0,0,0);
	}

	zstream_fdclose(zsm);

	if(z_server_type != 'f') {
		close(fd);
	}
	if(zvar_ss_idle_timeout > 0)
		zevent_request_timer(-1, zmaster_server_timeout, 0, zvar_ss_idle_timeout);
	return 0;
}

int zmaster_client_server(int argc, char **argv, int service_type, 
		void (*service_pre)(void), void(*service)(int fd, ZSTREAM *stream)){
	int i;

	z_service_type=service_type;
	z_service = service;

	z_start_time=time(0);
	zgconfig_init();
	zevent_init();


	for(i=1;i<argc;i++){
		char *name, *value;
		name=z_strdup(argv[i]);
		value=strchr(name, '=');
		if(value==0 || value==name){
			zmsg_warning("cmd %s, No.%d arg error: %s", argv[0], i, argv[i]);
			continue;
		}
		*value=0;value++;
		if(!strcmp(name, "-t")){
			z_server_type=value[0];
		}else if(!strcmp(name, "-c")){
			zgconfig_load(value, 1);
		}else {
			zgconfig_update(name, value);
		}
	}

	/* init default bool value */
	ZCONFIG_BOOL config_bool_list[]={
		/* */
		{"z_debug", 0, &zvar_debug},
		{"z_verbose", 0, &zvar_verbose},
		{0}
	};
	zgconfig_bool_table(config_bool_list);

	/* init default int value */
	ZCONFIG_INT config_int_list[]={
		/* */
		{"zss_use_limit", 		0,	&zvar_ss_use_limit},
		/* */
		{"zdict_bdb_cache", 		1,	&zvar_dict_bdb_cache},
		/* */
		{0}
	};
	zgconfig_int_table(config_int_list);

	/* init default time value */
	ZCONFIG_TIME config_time_list[]={
		/* */
		{"z_ipc_timeout",		"60s", 	&zvar_ss_use_limit},
		/* */
		{"zss_connection_timeout",	"90s",	&zvar_ss_connection_timeout},
		{"zss_daemon_timeout",		"0s", 	&zvar_ss_daemon_timeout},
		{"zss_idle_timeout",		"0s", 	&zvar_ss_idle_timeout},
		{"zss_runtime_timeout",		"0s", 	&zvar_ss_runtime_limit},
		/* */
		{0}
	};
	zgconfig_time_table(config_time_list);

	/* service_pre could do some useful actions, example:
	 * private vars, event loop, etc ...
	 */
	if(service_pre){
		service_pre();
	}

	zevent_enable_read(ZMASTER_STATUS_FD, zmaster_server_abort, 0);
	zio_close_on_exec(ZMASTER_STATUS_FD, 1);

	zevent_enable_read(ZMASTER_LISTEN_FD,
			((z_server_type=='f' || z_service_type=='S')?zmaster_single_server_accpet:zmaster_multi_server_accpet), 0);
	zio_close_on_exec(ZMASTER_LISTEN_FD, 1);

	while(1){
		int loop_delay;
		loop_delay=-1;

		if(zvar_ss_use_limit>0 && z_use_count > zvar_ss_use_limit){
			zmsg_info("master client server exit: use limit exceed %d", zvar_ss_use_limit);
			break;
		}
		if(zvar_ss_runtime_limit > 0 && time(0)>z_start_time+zvar_ss_runtime_limit){
			zmsg_info("master client server exit: time limit exceed %d", zvar_ss_runtime_limit);
			break;
		}

		if(zvar_ss_idle_timeout>0){
			loop_delay = zvar_ss_idle_timeout;
		}
		zevent_loop(loop_delay);
	}
	return 0;
}

int zmaster_single_server(int argc, char **argv,
		void (*service_pre)(void), void(*service)(int fd, ZSTREAM *stream)){
	return zmaster_client_server(argc, argv, 'S', service_pre, service);
}

int zmaster_multi_server(int argc, char **argv,
		void (*service_pre)(void), void(*service)(int fd, ZSTREAM *stream)){
	return zmaster_client_server(argc, argv, 'M', service_pre, service);
}

