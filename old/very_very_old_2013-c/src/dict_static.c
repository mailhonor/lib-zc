#include "zyc.h"

typedef struct {
	char *result;
	int len;
}ZDICT_STATIC;
static int zdict_static_lookup(ZDICT *dict, char *name);

int zdict_static_create(char *describe, char *path, ZDICT *dict){
	ZDICT_STATIC *info;

	dict->lookup=zdict_static_lookup;
	dict->info=z_malloc(sizeof(ZDICT_STATIC));

	info=(ZDICT_STATIC *)(dict->info);

	info->result=path;
	info->len=strlen(path);
	dict->info=info;
	return 0;
}

static int zdict_static_lookup(ZDICT *dict, char *name){
	ZDICT_STATIC *info;

	info=(ZDICT_STATIC *)(dict->info);
	zstr_memcpy(zvar_maps_result, info->result, info->len);
	return 1;
}
