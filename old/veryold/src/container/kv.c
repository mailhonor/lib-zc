#include "zc.h"

static int _lookup(ZKV * zkv, char *key, char **value)
{
	*value = 0;
	return 0;
}

static int _delete(ZKV * zkv, char *key)
{
	return 0;
}

static int _update(ZKV * zkv, char *key, char *value)
{
	return 0;
}

static int _query(ZKV * zkv, char *query, char **result)
{
	*result = 0;
	return 0;
}

static void _free(ZKV * zkv)
{
}

ZKV *zkv_create(void)
{
	ZKV *zkv;

	zkv = (ZKV *) zcalloc(1, sizeof(ZKV));
	zkv->lookup_fn = _lookup;
	zkv->delete_fn = _delete;
	zkv->update_fn = _update;
	zkv->query_fn = _query;
	zkv->free_fn = _free;

	return zkv;
}

void zkv_free(ZKV * zkv)
{
	zkv->free_fn(zkv);
	zfree(zkv);
}
