#include "zyc.h"

int zvar_dict_unix_try_limit=5;
int zvar_dict_unix_timeout=10;

typedef struct {
	char *path;
	ZSTREAM *fp;
	int try_limit;
	int timeout;
}ZDICT_UNIX;

static int zdict_unix_lookup(ZDICT *dict, char *name);

int zdict_unix_create(char *describe, char *path, ZDICT *dict){
	ZDICT_UNIX *info;
	ZSTR *zbuf;

	dict->lookup=zdict_unix_lookup;
	dict->info=z_malloc(sizeof(ZDICT_UNIX));
	info=(ZDICT_UNIX *)(dict->info);

	info->path=z_strdup(path);

	zbuf=zstr_concatenate(describe, ":try_limit", 0);
	info->try_limit=zgconfig_get_int(zstr_str(zbuf), zvar_dict_inet_try_limit);

	zbuf=zstr_concatenate(describe, ":timeout", 0);
	info->timeout=zgconfig_get_int(zstr_str(zbuf), zvar_dict_inet_timeout);
	return 0;
}

static int zdict_unix_connect(ZDICT_UNIX *info){
	int sock;

	sock=zsocket_unix_connect(info->path, info->timeout);
	if(sock<1){
		return sock;
	}
	info->fp=zstream_fdopen(sock, info->timeout);
	return 0;
}

static int zdict_unix_lookup(ZDICT *dict, char *name){
	ZDICT_UNIX *info;
	int tries;
	char *p;

	info=(ZDICT_UNIX *)(dict->info);

	tries=0;
	while(tries < info->try_limit){
		tries++;
		if(info->fp !=0 || zdict_unix_connect(info) == 0){
			zattr_fprint(info->fp, ZATTR_STR, "get", ZATTR_STR, name, ZATTR_END);
			ZSTREAM_FFLUSH(info->fp);
			zattr_fscan(info->fp, ZATTR_STR, zvar_maps_status, ZATTR_STR, zvar_maps_result, ZATTR_END);
			if(zstream_ferror(info->fp)!=ZSTREAM_EOF){
				break;
			}
			zmsg_error("read UNIX map reply from %s: unexpected EOF (%m)", info->path);
			zstream_fclose(info->fp);
			info->fp=0;
		}
		if(tries == info->try_limit){
			zstr_sprintf(zvar_maps_result, "INET map %s: too many try", info->path);
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
