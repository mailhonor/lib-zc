#include "zc.h"

typedef struct {
	char *host;
	int port;
	int fd;
	ZSIO *sio;
	char *query_format;
	ZBUF *qb;
	ZBUF *query_expand;
} ZMAP_MEMCACHE;

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout);
static int _query2(ZMAP * zm, char *query, char **value, int timeout);
static int _close(ZMAP * zm);

int zmap_open_memcache(ZMAP * zm, ZDICT * args)
{
	ZMAP_MEMCACHE *db;
	char *path, *host, *p;
	int port;
	int ret = Z_OK;

	db = zcalloc(1, sizeof(ZMAP_MEMCACHE));
	db->fd = 0;
	path = zdict_get_str(args, "main_arg", "");
	path = zstr_strdup(path);
	port = 0;
	p = strchr(path, ':');
	host = path;
	if (p) {
		*p++ = 0;
		port = atoi(p);
	} else {
		port = 11211;
	}
	db->host = zstr_strdup(host);
	db->port = port;
	db->qb = zbuf_create(1024);
	db->query_expand = zbuf_create(128);
	db->query_format = zstr_strdup(zdict_get_str(args, "query", "%s"));

	zm->db = db;
	zm->query = _query;
	zm->query2 = _query2;
	zm->close = _close;

	zstr_free(path);

	return ret;
}

static int ___connect(ZMAP * zm)
{
	int fd;
	ZMAP_MEMCACHE *db;

	db = (ZMAP_MEMCACHE *) (zm->db);

	if (db->sio) {
		return 0;
	}

	fd = zsocket_inet_connect_host(db->host, db->port, 10000);

	if (fd < 0) {
		return Z_ERR;
	}

	zio_nonblocking(fd, 1);
	db->sio = zsio_create(0);
	zsio_set_FD(db->sio, fd);
	db->fd = fd;

	return 0;
}

static int ___close(ZMAP * zm)
{
	ZMAP_MEMCACHE *db;

	db = (ZMAP_MEMCACHE *) (zm->db);
	if (!db->sio) {
		return 0;
	}
	zsio_free(db->sio);
	db->sio = 0;
	close(db->fd);
	db->fd = -1;

	return 0;
}

static int ___get_line(ZMAP * zm)
{
	ZBUF *qb;
	ZMAP_MEMCACHE *db;
	char *p;
	int len;

	db = (ZMAP_MEMCACHE *) (zm->db);
	qb = db->qb;

	ZBUF_RESET(qb);
	len = zsio_get_delimiter(db->sio, qb, '\n');
	if (len < 2) {
		___close(zm);
		return Z_ERR;
	}
	p = ZBUF_DATA(qb);
	if (p[len - 1] != '\n' || p[len - 2] != '\r') {
		___close(zm);
		return Z_ERR;
	}
	p[len - 2] = 0;

	return len - 2;
}

static inline int ___query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	int ok, i, len, ret;
	ZBUF *qb;
	ZMAP_MEMCACHE *db;

	db = (ZMAP_MEMCACHE *) (zm->db);
	qb = db->qb;

	zmap_query_expand(db->query_expand, db->query_format, query, 0);
	query = ZBUF_DATA(db->query_expand);
	ok = 0;
	for (i = 0; i < 2; i++) {
		if (i) {
			___close(zm);
		}
		if (___connect(zm) < 0) {
			return Z_ERR;
		}
		zsio_set_timeout(db->sio, timeout);
		zsio_fprintf(db->sio, "get %s\r\n", query);
		ZSIO_FFLUSH(db->sio);

		len = ___get_line(zm);
		if (len < 1) {
			continue;
		}
		if (!strcmp(ZBUF_DATA(qb), "END")) {
			return Z_NONE;
		}

		if (sscanf(ZBUF_DATA(qb), "VALUE %*s %*s %d", &len) < 1) {
			___close(zm);
			continue;
		}
		ZBUF_RESET(qb);
		if (zsio_get_n(db->sio, qb, len) < len) {
			___connect(zm);
			continue;
		}
		ZBUF_TERMINATE(qb);
		ret = zstr_strncpy(value, (char *)ZBUF_DATA(qb), value_len);

		len = ___get_line(zm);
		if (len < 0) {
			continue;
		}
		len = ___get_line(zm);
		if (len < 0) {
			continue;
		}
		if (strcmp(ZBUF_DATA(qb), "END")) {
			continue;
		}
		ok = 1;
		break;
	}

	if (ok) {
		return ret;
	}
	___close(zm);

	return Z_ERR;
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
	ZMAP_MEMCACHE *db;

	db = (ZMAP_MEMCACHE *) (zm->db);

	zbuf_free(db->qb);
	zbuf_free(db->query_expand);
	zstr_free(db->query_format);
	zstr_free(db->host);
	___close(zm);

	zfree(db);

	return Z_OK;
}
