#include "zc.h"
#include <mysql/mysql.h>

typedef struct {
	char *host;
	int port;
	char *dbname;
	char *dbuser;
	char *dbpass;
	MYSQL *sql_db;
	char *query_format;
	ZBUF *qb;
	ZBUF *query_expand;
} ZMAP_MYSQL;

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout);
static int _query2(ZMAP * zm, char *query, char **value, int timeout);
static int _close(ZMAP * zm);

int zmap_open_mysql(ZMAP * zm, ZDICT * args)
{
	ZMAP_MYSQL *db;
	char *path, *host, *p;
	int port;
	int ret = Z_OK;

	db = zcalloc(1, sizeof(ZMAP_MYSQL));
	path = zdict_get_str(args, "main_arg", "");
	path = zstr_strdup(path);
	port = 0;
	p = strchr(path, ':');
	host = path;
	if (p) {
		*p++ = 0;
		port = atoi(p);
	} else {
		port = 3306;
	}
	db->host = zstr_strdup(host);
	db->port = port;
	db->qb = zbuf_create(1024);
	db->query_expand = zbuf_create(128);
	db->query_format = zstr_strdup(zdict_get_str(args, "query", "%s"));
	db->dbname = zstr_strdup(zdict_get_str(args, "dbname", ""));
	db->dbuser = zstr_strdup(zdict_get_str(args, "user", "root"));
	db->dbpass = zstr_strdup(zdict_get_str(args, "pass", ""));

	zm->db = db;
	zm->query = _query;
	zm->query2 = _query2;
	zm->close = _close;

	zstr_free(path);

	return ret;
}

static int ___connect(ZMAP * zm)
{
	ZMAP_MYSQL *db;

	db = (ZMAP_MYSQL *) (zm->db);

	if (db->sql_db) {
		return 0;
	}

	db->sql_db = mysql_init(0);
	if (!mysql_real_connect(db->sql_db, db->host, db->dbuser, db->dbpass, db->dbname, db->port, NULL, 0)) {
		mysql_close(db->sql_db);
		db->sql_db = 0;
		return Z_ERR;
	}

	return 0;
}

static int ___close(ZMAP * zm)
{
	ZMAP_MYSQL *db;

	db = (ZMAP_MYSQL *) (zm->db);
	if (!db->sql_db) {
		return 0;
	}
	mysql_close(db->sql_db);
	db->sql_db = 0;

	return 0;
}

static inline int ___query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	int ok, i, ret;
	ZBUF *qb;
	ZMAP_MYSQL *db;
	MYSQL_RES *res = 0;
	MYSQL_ROW row;
	int numrows;
	char *p;

	db = (ZMAP_MYSQL *) (zm->db);
	qb = db->qb;

	zmap_query_expand(db->query_expand, db->query_format, query, 0);
	query = ZBUF_DATA(db->query_expand);
	ok = 0;
	res = 0;
	for (i = 0; i < 2; i++) {
		if (i) {
			___close(zm);
		}
		if (___connect(zm) < 0) {
			return Z_ERR;
		}
		if (mysql_query(db->sql_db, query)) {
			continue;
		}
		if ((res = mysql_store_result(db->sql_db)) == 0) {
			continue;
		}
		ok = 1;
		break;
	}

	if (!ok) {
		___close(zm);
		return Z_ERR;
	}

	numrows = mysql_num_rows(res);
	if (numrows == 0) {
		mysql_free_result(res);
		return Z_NONE;
	}
	ZBUF_RESET(qb);
	for (i = 0; i < numrows; i++) {
		row = mysql_fetch_row(res);
		zbuf_put(qb, '\n');
		zbuf_strcat(qb, row[0]);

	}
	mysql_free_result(res);

	ZBUF_TERMINATE(qb);
	p = ZBUF_DATA(qb);
	ret = zstr_strncpy(value, p + 1, value_len);

	return ret;
}

static int ___release(ZTIMER * tm, void *ctx)
{
	ZMAP *zm;

	zm = (ZMAP *) ctx;
	___close(zm);

	return 0;
}

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	int ret;
	ret = ___query(zm, query, value, value_len, timeout);
	if (zm->release_sec > 0) {
		ztimer_set(&(zm->release_timer), ___release, zm, zm->release_sec * 1000);
	}

	return ret;
}

static int _query2(ZMAP * zm, char *query, char **value, int timeout)
{
	*value = 0;
	return Z_OK;
}

static int _close(ZMAP * zm)
{
	ZMAP_MYSQL *db;

	db = (ZMAP_MYSQL *) (zm->db);

	zbuf_free(db->qb);
	zbuf_free(db->query_expand);
	zstr_free(db->query_format);
	zstr_free(db->host);
	zstr_free(db->dbuser);
	zstr_free(db->dbpass);
	zstr_free(db->dbname);
	___close(zm);

	zfree(db);

	return Z_OK;
}
