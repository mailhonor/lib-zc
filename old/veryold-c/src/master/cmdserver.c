#include "zc.h"

int zcmder_quit(ZCMDER * cmder)
{
	int fd;
	ZAIO *aio = cmder->aio;

	fd = zaio_get_fd(aio);

	zaio_fini(aio);
	close(fd);

	return 0;
}

static int zcmder_noop(ZCMDER * cmder)
{
	return zcmder_ok_dict(cmder, 1000, 0);
}

static int zcmder_none(ZCMDER * cmder)
{
	ZBUF *zb;

	zb = cmder->outbuf;

	zbuf_reset(zb);
	zbuf_sizedata_escape(zb, "B", 1);
	zcmder_append_zbuf(cmder, zb);
	zcmder_flush(cmder, 1000);

	return 0;
}

int zcmder_fatal(ZCMDER * cmder)
{
	int ret;
	ZAIO *aio = cmder->aio;

	ret = zaio_get_ret(aio);

	if (ret == ZAIO_ERROR) {
	} else {
	}

	zcmder_quit(cmder);

	return 0;
}

int zcmder_error(ZCMDER * cmder, char *code, char *msg)
{
	ZBUF *zb = cmder->outbuf;

	zbuf_reset(zb);
	zbuf_sizedata_escape(zb, "B", 1);
	zbuf_sizedata_escape(zb, "code", 4);
	if (!code) {
		code = "";
	}
	zbuf_sizedata_escape(zb, code, strlen(code));
	zbuf_sizedata_escape(zb, "msg", 4);
	if (!msg) {
		msg = "";
	}
	zbuf_sizedata_escape(zb, msg, strlen(msg));
	zcmder_append_zbuf(cmder, zb);
	zcmder_flush(cmder, 1000);

	return 0;
}

int zcmder_bad(ZCMDER * cmder)
{
	return zcmder_error(cmder, "argument", 0);
}

int zcmder_ok_dict(ZCMDER * cmder, int timeout, ZDICT * dict)
{
	ZBUF *zb = cmder->outbuf;

	zbuf_reset(zb);
	zbuf_sizedata_escape(zb, "o", 1);
	zcmder_append_zbuf(cmder, zb);
	zcmder_flush(cmder, timeout);

	return 0;
}

int zcmder_flush(ZCMDER * cmder, int timeout)
{
	zaio_write_cache_flush(cmder->aio, zcmder_loop, 0, timeout);

	return 0;
}

int zcmder_append_zbuf(ZCMDER * cmder, ZBUF * zb)
{
	zaio_write_cache_append_sizedata(cmder->aio, ZBUF_LEN(zb), ZBUF_DATA(zb));
	return 0;
}

int zcmder_append_buf(ZCMDER * cmder, int size, void *buf)
{
	zaio_write_cache_append_sizedata(cmder->aio, size, buf);
	return 0;
}

ZCMDER *zcmder_create(void)
{
	ZCMDER *cmder;

	cmder = zcalloc(1, sizeof(ZCMDER));

	cmder->eb = zevent_base_create();
	cmder->cmd_func_list = zdict_create();
	cmder->sdlist = zsdlist_create(256);
	cmder->outbuf = zbuf_create(102400);
	zevent_base_set_context(cmder->eb, cmder);

	return cmder;
}

void zcmder_register(ZCMDER * cmder, char *cmd_name, ZCMDER_FN cmd_func)
{
	ZDICT_NODE *n;
	ZDICT *list = cmder->cmd_func_list;

	if (ZDICT_LEN(list) == 0) {
		zdict_enter(list, "quit", zcmder_quit, 0);
		zdict_enter(list, "exit", zcmder_quit, 0);
		zdict_enter(list, "noop", zcmder_noop, 0);
	}

	n = zdict_lookup(list, cmd_name, 0);
	if (n) {
		ZDICT_UPDATE_VALUE(n, cmd_func);
	} else {
		zdict_enter(list, cmd_name, cmd_func, 0);
	}
}

int zcmder_arguments_lookup(ZCMDER * cmder, char *key, char **value)
{
	int i;
	ZSDATA *sdk, *sdv;
	ZSDLIST *sdlist = cmder->sdlist;

	for (i = 1; i < sdlist->len; i++) {
		sdk = sdlist->list + i++;
		if (i == sdlist->len) {
			return -1;
		}
		if (strcmp((char *)(sdk->data), key)) {
			continue;
		}
		sdv = sdlist->list + i;
		if (value) {
			*value = (char *)(sdv->data);
		}
		return sdv->size;
	}

	return -1;
}

static void *cmder_worker_run(void *arg)
{
	ZCMDER *cmder;
	ZEVENT_BASE *eb;

	cmder = (ZCMDER *) arg;
	eb = cmder->eb;
	if (!cmder->background) {
		pthread_detach(pthread_self());
	}

	while (1) {
		zevent_base_dispatch(eb, 0);
	}

	return arg;
}

void zcmder_run(ZCMDER * cmder, int flag)
{				/* 0 means running on background */
	cmder->background = (flag ? 0 : 1);
	if (cmder->background) {
		pthread_create(&(cmder->pth_t), 0, cmder_worker_run, cmder);
	} else {
		cmder_worker_run(cmder);
	}
}

static int _parse_cmder_line(ZAIO * aio, void *context, char *rbuf)
{
	int ret;
	char *cmder_name;
	ZCMDER_FN cmder_func;
	ZSDLIST *sdlist;
	ZCMDER *cmder;

	cmder = (ZCMDER *) zevent_base_get_context(zaio_get_base(aio));
	cmder->aio = aio;
	sdlist = cmder->sdlist;
	zsdlist_reset(sdlist);

	cmder_func = 0;

	ret = zaio_get_ret(aio);
	if (ret < 1) {
		return zcmder_fatal(cmder);
	}

	zsdlist_parse_sizedata(sdlist, rbuf, ret);
	if (sdlist->len < 1) {
		return zcmder_error(cmder, "nocmder", 0);
	}
	zsdlist_terminate(sdlist);

	cmder_name = (char *)(sdlist->list[0].data);
	zstr_tolower(cmder_name);

	if (!zdict_lookup(cmder->cmd_func_list, cmder_name, (char **)&cmder_func)) {
		cmder_func = zcmder_none;
	}

	zbuf_reset(cmder->outbuf);
	cmder_func(cmder);

	return 0;
}

int zcmder_loop(ZAIO * aio, void *context, char *unused)
{
	zaio_read_sizedata(aio, _parse_cmder_line, context, 60000);

	return 0;
}
