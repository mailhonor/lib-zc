#include "zyc.h"

int zvar_dict_bdb_cache=1;
typedef struct {
	DB *bdb;
	int cache;

}ZDICT_BDB;
static int zdict_bdb_lookup(ZDICT *dict, char *name);
static int zdict_bdb_create(char *describe, char *path, ZDICT *dict, DBTYPE dt);

int zdict_hash_create(char *describe, char *path, ZDICT *dict){
	return zdict_bdb_create(describe, path, dict, DB_HASH);
}
int zdict_btree_create(char *describe, char *path, ZDICT *dict){
	return zdict_bdb_create(describe, path, dict, DB_BTREE);
}
static int zdict_bdb_create(char *describe, char *path, ZDICT *dict, DBTYPE dt){
	ZDICT_BDB *info;
	DB *bdb;
	ZSTR *zbuf;

	dict->lookup=zdict_bdb_lookup;
	dict->info=z_malloc(sizeof(ZDICT_BDB));

	info=(ZDICT_BDB *)(dict->info);

	if(z_global_config){
		zbuf=zstr_concatenate(describe, ":cache", 0);
		info->cache=zgconfig_get_int(zstr_str(zbuf), zvar_dict_bdb_cache);
	}
	db_create(&bdb, NULL, 0);
	bdb->set_cachesize(bdb, 0, (info->cache)*1024*1024, 0);
	if(bdb->open(bdb, NULL, path, NULL, dt, 0, 0)){
		zmsg_error("error: open bdb: %s :%m", path);
		return -1;
	}
	info->bdb=bdb;
	dict->info=info;
	return 0;
}

static int zdict_bdb_lookup(ZDICT *dict, char *name){
	DBT key, data;
	int ret;
	DB *bdb;

	bdb=((ZDICT_BDB *)(dict->info))->bdb;
	zstr_reset(zvar_maps_query);

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));
	key.data = name;
	key.size = strlen(name);

	ret = bdb->get(bdb, NULL, &key, &data, 0);
	if(ret){
		return 0;
	}
	if(data.size){
		zstr_strncat(zvar_maps_result, data.data, data.size);
	}
	return 1;
}
