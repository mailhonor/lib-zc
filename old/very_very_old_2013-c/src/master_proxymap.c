#include "zyc.h"

static ZSTR *dict_action=0;
static ZSTR *dict_describe=0;
static ZSTR *dict_query=0;
static int zsm_error_times=0;
static int zsm_error_max=100;

static void proxymap_service_error(int fd, ZSTREAM *zsm){
	zmsg_error("PROXY map service read EOF");
	zmaster_multi_server_disconnect(fd, zsm);
	if(++zsm_error_times > zsm_error_max){
		zmsg_fatal("PROXY map service too ERROR(%d)", zsm_error_times);
	}
}
static void proxymap_service_init(void){
	dict_action=zstr_create(64);
	dict_describe=zstr_create(64);
	dict_query=zstr_create(128);
}

static void proxymap_service(int fd, ZSTREAM *zsm){
	ZDICT *zdict;
	char *describe, *query;
	int  sret;

	zattr_fscan(zsm, ZATTR_STR, dict_action, ZATTR_STR, dict_describe, ZATTR_STR, dict_query, ZATTR_END);
	describe=ZSTR_STR(dict_describe);
	query=ZSTR_STR(dict_query);
	if(zstream_ferror(zsm)){
		proxymap_service_error(fd, zsm);
		return;
	}
	zmsg_debug("proxymap_service, describe: %s, query: %s", describe, query);

	zdict=zdict_locate_dict(describe);
	while(1){
		if(zdict==0){
			if(zdict_register(describe)<0){
				zstr_strcpy(zvar_maps_status, "ERR");
				zstr_strcpys(zvar_maps_result, "can not create dict: ", describe, 0);
				break;
			}
			zdict=zdict_locate_dict(describe);
		}
		zstr_reset(zvar_maps_result);
		sret=zdict_lookup(zdict, query);
		if(sret<0){
			zstr_strcpy(zvar_maps_status, "ERR");
		}else if(sret==0){
			zstr_strcpy(zvar_maps_status, "NO");
		}else{
			zstr_strcpy(zvar_maps_status, "YES");
		}
		break;
	}
	zattr_fprint(zsm, ZATTR_STR, ZSTR_STR(zvar_maps_status), ZATTR_STR, ZSTR_STR(zvar_maps_result), ZATTR_END);
	ZSTREAM_FFLUSH(zsm);
	if(zstream_ferror(zsm)){
		proxymap_service_error(fd, zsm);
		return;
	}
	return;
}

int zmaster_proxymap(int argc, char **argv)
{
	zvar_program_name=argv[0];
	
	zmaster_multi_server(argc, argv, proxymap_service_init, proxymap_service);
	return(0);
}
