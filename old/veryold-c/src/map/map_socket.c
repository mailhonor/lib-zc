#include "zc.h"

typedef struct {
	char *host;
	int port;
	int fd;
	ZSIO *sio;
	char *query_format;
	ZBUF *qb;
	ZBUF *query_expand;
} ZMAP_SOCKET;

static int _query(ZMAP * zm, char *query, char *value, int value_len, int timeout);
static int _query2(ZMAP * zm, char *query, char **value, int timeout);
static int _close(ZMAP * zm);

int zmap_open_socket(ZMAP * zm, ZDICT * args)
{
	ZMAP_SOCKET *db;
	char *path, *host;
	int type, port;
	int ret = Z_OK;

	db = zcalloc(1, sizeof(ZMAP_SOCKET));
	db->fd = 0;
	path = zdict_get_str(args, "main_arg", "");
	path = zstr_strdup(path);
	port = 0;
	if (zsocket_parse_path(path, &type, &host, &port) < 0) {
		zmap_set_info(zm, "%s", "socket's path error");
		ret = Z_ERR;
		goto err;
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

      err:
	zstr_free(path);

	return ret;
}

static int ___connect(ZMAP * zm)
{
	int fd;
	ZMAP_SOCKET *db;

	db = (ZMAP_SOCKET *) (zm->db);

	if (db->sio) {
		return 0;
	}

	if (db->port) {
		fd = zsocket_inet_connect_host(db->host, db->port, 10000);
	} else {
		fd = zsocket_unix_connect(db->host, 1000);
	}
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
	ZMAP_SOCKET *db;

	db = (ZMAP_SOCKET *) (zm->db);
	if (!db->sio) {
		return 0;
	}
	zsio_free(db->sio);
	db->sio = 0;
	close(db->fd);
	db->fd = -1;

	return 0;
}

static inline int ___query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	int ok, i, len, ret;
	ZBUF *qb;
	ZMAP_SOCKET *db;
	char *p;

	db = (ZMAP_SOCKET *) (zm->db);
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
		zsio_fprintf(db->sio, "%s\r\n", query);
		ZSIO_FFLUSH(db->sio);

		len = zsio_get_delimiter(db->sio, qb, '\n');
		if (len < 2) {
			continue;
		}
		p = ZBUF_DATA(qb);
		if (p[len - 1] != '\n') {
			continue;
		}
		len--;
		if (p[len - 1] == '\r') {
			len--;
		}
		p[len] = 0;
		*value = 0;
		ret = 0;
		if (len) {
			ret = zstr_strncpy(value, p, value_len);
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
	ZMAP_SOCKET *db;

	db = (ZMAP_SOCKET *) (zm->db);

	zbuf_free(db->qb);
	zbuf_free(db->query_expand);
	zstr_free(db->query_format);
	zstr_free(db->host);
	___close(zm);

	zfree(db);

	return Z_OK;
}
