#include "zyc.h"

int zvar_dict_inet_try_limit=5;
int zvar_dict_inet_timeout=10;

typedef struct {
	char *host;
	int port;
	ZSTREAM *fp;
	int try_limit;
	int timeout;
}ZDICT_INET;

static int zdict_inet_lookup(ZDICT *dict, char *name);

int zdict_inet_create(char *describe, char *path, ZDICT *dict){
	ZDICT_INET *info;
	char *p,*pbuf;
	ZSTR *zbuf;

	dict->lookup=zdict_inet_lookup;
	dict->info=z_malloc(sizeof(ZDICT_INET));
	info=(ZDICT_INET *)(dict->info);

	info->fp=0;

	pbuf=z_strdup(path);
	p=strchr(pbuf, ':');
	if(p==NULL){
		zmsg_error("create maps error: %s", describe);
		return -1;
	}
	*p=0;
	p++;
	info->host=pbuf;
	info->port=atoi(p);
	if(info->port<0 || info->port > 65535){
		zmsg_error("create maps error: %s : port error", describe);
		return -1;
	}

	zbuf=zstr_concatenate(describe, ":try_limit", 0);
	info->try_limit=zgconfig_get_int(zstr_str(zbuf), zvar_dict_inet_try_limit);

	zbuf=zstr_concatenate(describe, ":timeout", 0);
	info->timeout=zgconfig_get_int(zstr_str(zbuf), zvar_dict_inet_timeout);
	return 0;
}

static int zdict_inet_connect(ZDICT_INET *info){
	int sock;

	sock=zsocket_inet_connect_host(info->host, info->port, info->timeout);
	if(sock<1){
		return sock;
	}
	info->fp=zstream_fdopen(sock, info->timeout);
	return 0;
}

static int zdict_inet_lookup(ZDICT *dict, char *name){
	ZDICT_INET *info;
	int tries;
	char *p;

	info=(ZDICT_INET *)(dict->info);

	tries=0;
	while(tries < info->try_limit){
		tries++;
		if(info->fp !=0 || zdict_inet_connect(info) == 0){
			zattr_fprint(info->fp, ZATTR_STR, "get", ZATTR_STR, name, ZATTR_END);
			ZSTREAM_FFLUSH(info->fp);
			zattr_fscan(info->fp, ZATTR_STR, zvar_maps_status, ZATTR_STR, zvar_maps_result, ZATTR_END);
			if(zstream_ferror(info->fp)!=ZSTREAM_EOF){
				break;
			}
			zmsg_error("read TCP map reply from %s:%d: unexpected EOF (%m)", info->host,info->port);
			zstream_fclose(info->fp);
			info->fp=0;
		}
		if(tries == info->try_limit){
			zstr_sprintf(zvar_maps_result, "INET map %s:%d: too many try", info->host,info->port);
			return ZMAPS_LOOKUP_ERROR;
		}
		if(tries!=1){
			sleep(1);
		}
	}
	if(tries > 1){
	}
	if(zstr_len(zvar_maps_status)==0){
		return ZMAPS_LOOKUP_ERROR;
	}
	p=ZSTR_STR(zvar_maps_status);
	if(*p=='N'){
		return 0;
	}
	if(*p=='E'){
		return ZMAPS_LOOKUP_ERROR;
	}
	if(*p!='Y'){
		return ZMAPS_LOOKUP_ERROR;
	}
	return 1;
}
