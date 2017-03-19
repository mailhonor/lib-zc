#include "zc.h"

typedef struct {
	char *path;
	ZSKVLIST *skv;
	unsigned long stamp;
	char *query_format;
	ZBUF *query_expand;
} ZMAP_SKVLIST;

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout);
static int _query2(ZMAP * zm, char *query, char **value, int timeout);
static int _close(ZMAP * zm);

int zmap_open_skvlist(ZMAP * zm, ZDICT * args)
{
	ZMAP_SKVLIST *db;
	char *path;
	int ret = Z_OK;

	db = zcalloc(1, sizeof(ZMAP_SKVLIST));
	path = zdict_get_str(args, "main_arg", "");
	db->path = zstr_strdup(path);
	db->query_expand = zbuf_create(128);
	db->query_format = zstr_strdup(zdict_get_str(args, "query", "%s"));
	db->skv = zskvlist_create(path);

	zm->db = db;
	zm->query = _query;
	zm->query2 = _query2;
	zm->close = _close;

	return ret;
}

static inline int ___query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	int ret;
	ZMAP_SKVLIST *db;
	char *v;

	db = (ZMAP_SKVLIST *) (zm->db);

	zmap_query_expand(db->query_expand, db->query_format, query, 0);
	query = ZBUF_DATA(db->query_expand);

	if (time(0) - db->stamp > 1) {
		if (zskvlist_changed(db->skv)) {
			zskvlist_load(db->skv);
		}
		db->stamp = time(0);
	}
	if (!zskvlist_lookup(db->skv, query, &v)) {
		return Z_NONE;
	}

	ret = zstr_strncpy(value, v, value_len);

	return ret;
}

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	int ret;
	ret = ___query(zm, query, value, value_len, timeout);

	return ret;
}

static int _query2(ZMAP * zm, char *query, char **value, int timeout)
{
	*value = 0;
	return Z_OK;
}

static int _close(ZMAP * zm)
{
	ZMAP_SKVLIST *db;

	db = (ZMAP_SKVLIST *) (zm->db);

	zstr_free(db->path);
	zstr_free(db->query_expand);
	zbuf_free(db->query_expand);
	zskvlist_free(db->skv);

	zfree(db);

	return Z_OK;
}
