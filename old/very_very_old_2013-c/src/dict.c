#include "zyc.h"

static ZHASH *z_all_dict_list=0;
ZSTR *zvar_maps_query=0;
ZSTR *zvar_maps_result=0;
ZSTR *zvar_maps_status=0;

int zdict_register(char *describe){
	char type[12], *path, *p;
	int type_len, dict_ret;
	ZDICT *dict;

	if(z_all_dict_list == 0){
		z_all_dict_list=zhash_create(100);
		zvar_maps_query=zstr_create(128);
		zvar_maps_result=zstr_create(128);
		zvar_maps_status=zstr_create(9);
	}

	if(zhash_find(z_all_dict_list, describe, 0)){
		return 0;
	}
	
	p=strchr(describe, ':');
	if(!p || ((type_len=p-describe)>10)){
		return -1;
	}

	if(type_len>0)
		strncpy(type, describe, type_len);
	type[type_len]=0;
	p++;
	path=p;

	dict=(ZDICT *)z_malloc(sizeof(ZDICT));
	dict_ret=0;
	if(type[0]==0){
		dict_ret=zdict_proxy_create(describe, path, dict);
	}else if(!strcmp("hash", type)){
		dict_ret=zdict_hash_create(describe, path, dict);
	}else if(!strcmp("btree", type)){
		dict_ret=zdict_btree_create(describe, path, dict);
	}else if(!strcmp("static", type)){
		dict_ret=zdict_static_create(describe, path, dict);
	}else if(!strcmp("inet", type)){
		dict_ret=zdict_inet_create(describe, path, dict);
	}else if(!strcmp("unix", type)){
		dict_ret=zdict_unix_create(describe, path, dict);
	}else{
		zmsg_error("zdict_register: unkown dict type: %s", type);
		return -2;
	}

	if(dict_ret<0){
		z_free(dict);
		return -3;
	}

	zhash_enter(z_all_dict_list, describe, (char *)dict);
	return 0;
}

int zmaps_lookup(ZMAPS *zm, char *name, int flag){
	int i, ret;
	char *dict_name, *name_case;
	ZDICT *dict;

	name_case=name;
	if(flag & ZMAPS_LOOKUP_LOWERCASE){
		z_lowercase(name_case);
	}else if(flag & ZMAPS_LOOKUP_UPPERCASE){
		z_uppercase(name_case);
	}
	for(i=0;i<zm->argc;i++){
		dict_name = zm->argv[i];
		zhash_find(z_all_dict_list, dict_name, (char **)&dict);
		ret=zdict_lookup(dict, name_case);
		if(ret == ZMAPS_LOOKUP_ERROR){
			if(flag & ZMAPS_LOOKUP_RETURN_AFTER_ERROR){
				return ZMAPS_LOOKUP_ERROR;
			}else if(flag & ZMAPS_LOOKUP_SKIP_AFTER_ERROR){
				continue;
			}else{
				zmsg_fatal("%s", ZSTR_STR(zvar_maps_result));
			}
		}
		if(ret>0){
			return ret;
		}
	}
	return 0;
}

ZMAPS * zmaps_create(char *str){
	int i, ret;
	ZMAPS *zm;

	zm=zargv_split(str, " ,;\t\r\n");
	for(i=0;i<zm->argc;i++){
		ret=zdict_register(zm->argv[i]);
		if(ret < 0 )return 0;
	}
	return zm;
}

ZDICT * zdict_locate_dict(char *describe){
	char *dict;

	if(z_all_dict_list == 0){
		return 0;
	}
	if(zhash_find(z_all_dict_list, describe, &dict)){
		return (ZDICT *)dict;
	}
	return 0;
}
