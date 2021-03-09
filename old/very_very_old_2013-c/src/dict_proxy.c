#include "zyc.h"

char *zvar_dict_proxy_path="zsockets/";
int zvar_dict_proxy_try_limit=5;
int zvar_dict_proxy_timeout=10;

static ZHASH *z_dict_proxy_stream_list=0;
typedef struct {
	ZSTREAM *fp;
	int used_count;
}ZDICT_PROXY_STREAM;

typedef struct {
	char *proxy_name;
	char *proxy_path;
	char *path;
	int try_limit;
	int timeout;
	ZDICT_PROXY_STREAM *pfp;
}ZDICT_PROXY;

static int zdict_proxy_lookup(ZDICT *dict, char *name);

int zdict_proxy_create(char *describe, char *path, ZDICT *dict){
	ZDICT_PROXY *info;
	char *p,*pbuf;
	ZSTR *zbuf;
	
	if(z_dict_proxy_stream_list==0){
		z_dict_proxy_stream_list=zhash_create(13);
	}

	dict->lookup=zdict_proxy_lookup;
	dict->info=z_malloc(sizeof(ZDICT_PROXY));
	info=(ZDICT_PROXY *)(dict->info);

	pbuf=z_strdup(path);
	p=strchr(pbuf, ':');
	if(p==NULL || p==path){
		zmsg_error("create maps error: %s", describe);
		return -1;
	}

	*p=0;
	info->proxy_name=z_strdup(pbuf);

	p++;
	info->path=z_strdup(p);

	if(zhash_find(z_dict_proxy_stream_list, info->proxy_name, &p)){
		info->pfp=(ZDICT_PROXY_STREAM *)p;
	}else{
		info->pfp=(ZDICT_PROXY_STREAM *)(z_malloc(sizeof(ZDICT_PROXY_STREAM)));
		info->pfp->fp=0;
		info->pfp->used_count=0; /* no use*/
		zhash_enter(z_dict_proxy_stream_list, info->proxy_name, (char *)(info->pfp));
	}

	zbuf=zstr_concatenate(describe, ":try_limit", 0);
	info->try_limit=zgconfig_get_int(zstr_str(zbuf), zvar_dict_proxy_try_limit);

	zbuf=zstr_concatenate(describe, ":timeout", 0);
	info->timeout=zgconfig_get_int(zstr_str(zbuf), zvar_dict_proxy_timeout);

	info->proxy_path=z_strdup(zstr_str(zstr_concatenate(zvar_dict_proxy_path, info->proxy_name, 0)));
	return 0;
}

static int zdict_proxy_connect(ZDICT_PROXY *info){
	int sock;

	sock=zsocket_unix_connect(info->proxy_path, info->timeout);
	if(sock<1){
		return sock;
	}
	info->pfp->fp=zstream_fdopen(sock, info->timeout);
	return 0;
}

static int zdict_proxy_lookup(ZDICT *dict, char *name){
	ZDICT_PROXY *info;
	int tries;
	char *p;

	info=(ZDICT_PROXY *)(dict->info);

	tries=0;
	while(tries < info->try_limit){
		tries++;
		if(info->pfp->fp !=0 || zdict_proxy_connect(info) == 0){
			zattr_fprint(info->pfp->fp, ZATTR_STR, "get", ZATTR_STR, info->path, ZATTR_STR, name, ZATTR_END);
			ZSTREAM_FFLUSH(info->pfp->fp);
			zattr_fscan(info->pfp->fp, ZATTR_STR, zvar_maps_status, ZATTR_STR, zvar_maps_result, ZATTR_END);
			if(!zstream_ferror(info->pfp->fp)){
				break;
			}
			zmsg_error("read PROXY map %s: unexpected EOF (%m)", info->proxy_name);
			zstream_fclose(info->pfp->fp);
			info->pfp->fp=0;
		}
		if(tries == info->try_limit){
			zmsg_error("PROXY map %s: too many try", info->proxy_name);
			zstr_sprintf(zvar_maps_result, "PROXY map %s: too many try", info->proxy_name);
			return ZMAPS_LOOKUP_ERROR;
		}
		if(tries!=1){
			sleep(1);
		}
	}
	if(tries > 1){
	}
	if(ZSTR_LEN(zvar_maps_status)==0){
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
