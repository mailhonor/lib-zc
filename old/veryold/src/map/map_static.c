#include "zc.h"

typedef struct {
	char *result;
} ZMAP_STATIC;

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout);
static int _query2(ZMAP * zm, char *query, char **value, int timeout);
static int _close(ZMAP * zm);

int zmap_open_static(ZMAP * zm, ZDICT * args)
{
	ZMAP_STATIC *db;
	char *def;

	db = zcalloc(1, sizeof(ZMAP_STATIC));
	def = zdict_get_str(args, "main_arg", "");

	db->result = zstr_strdup(def);

	zm->db = db;
	zm->query = _query;
	zm->query2 = _query2;
	zm->close = _close;

	return Z_OK;
}

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	int len;
	ZMAP_STATIC *db;

	db = (ZMAP_STATIC *) (zm->db);
	len = zstr_strncpy(value, db->result, value_len);

	return len;
}

static int _query2(ZMAP * zm, char *query, char **value, int timeout)
{
	ZMAP_STATIC *db;

	db = (ZMAP_STATIC *) (zm->db);
	*value = (db->result);

	return Z_OK;
}

static int _close(ZMAP * zm)
{
	ZMAP_STATIC *db;

	db = (ZMAP_STATIC *) (zm->db);
	zstr_free(db->result);
	zfree(db);

	return Z_OK;
}
