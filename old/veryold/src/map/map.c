#include "zc.h"

static int _set_info(ZMAP * zm, char *fmt, ...);
static int _parse_map_url(ZDICT * args, char *new_url);

int zmap_open_static(ZMAP * zm, ZDICT * args);
int zmap_open_socket(ZMAP * zm, ZDICT * args);
int zmap_open_memcache(ZMAP * zm, ZDICT * args);
int zmap_open_mysql(ZMAP * zm, ZDICT * args);
int zmap_open_skvlist(ZMAP * zm, ZDICT * args);

ZMAP *zmap_create(int flags)
{
	ZMAP *zm;

	zm = (ZMAP *) zcalloc(1, sizeof(ZMAP));
	zm->info_size = 256;
	zm->info = (char *)zmalloc(zm->info_size);
	zm->info[0] = 0;
	zm->set_info = _set_info;

	return zm;
}

int zmap_open(ZMAP * zm, char *url)
{
	ZDICT *args;
	char *new_url;
	char *type;
	int ret;

	new_url = zstr_strdup(url);
	args = zdict_create();

	if (_parse_map_url(args, new_url) < 0) {
		type = "://config";
	} else {
		type = zdict_get_str(args, "ztype", "");
	}

	if (!strcmp(type, "static")) {
		ret = zmap_open_static(zm, args);
	} else if (!strcmp(type, "socket")) {
		ret = zmap_open_socket(zm, args);
	} else if (!strcmp(type, "memcache")) {
		ret = zmap_open_memcache(zm, args);
	} else if (!strcmp(type, "mysql")) {
		ret = zmap_open_mysql(zm, args);
	} else if (!strcmp(type, "skvlist")) {
		ret = zmap_open_skvlist(zm, args);
#if 0
	} else if (!strcmp(type, "url")) {
		ret = zmap_open_url(zm, args);
	} else if (!strcmp(type, "mysql")) {
		ret = zmap_open_url(zm, args);
#endif
	} else if (!strcmp(type, "://config")) {
		zm->set_info(zm, "zmap_open: url error: %s", url);
		ret = Z_CONFIG;
	} else {
		zm->set_info(zm, "zmap_open: unkown map type: (%s)", type);
		ret = Z_NONE_TYPE;
	}
	zdict_free_STR(args);
	zstr_free(new_url);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

void zmap_set_release_event(ZMAP * zm, ZEVENT_BASE * eb, int sec)
{
	ztimer_init(&(zm->release_timer), eb);
	zm->release_sec = sec;
}

void zmap_free(ZMAP * zm)
{
	if (zm->close) {
		zm->close(zm);
	}
	zfree(zm->info);
	zfree(zm);
}

int zmap_query(ZMAP * zm, char *query, char *value, int value_len, int timeout)
{
	zm->info[0] = 0;
	return zm->query(zm, query, value, value_len, timeout);
}

int zmap_query2(ZMAP * zm, char *query, char **value, int timeout)
{
	zm->info[0] = 0;
	return zm->query2(zm, query, value, timeout);
}

char *zmap_get_info(ZMAP * zm)
{
	return zm->info;
}

static int _set_info(ZMAP * zm, char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vsnprintf(zm->info, zm->info_size - 1, fmt, ap);
	va_end(ap);

	return ret;
}

static int _parse_map_url(ZDICT * args, char *url)
{
	char *p;
	ZBUF *zb;

	p = strstr(url, "://");
	if (!p) {
		return -1;
	}
	*p = 0;
	zdict_enter_STR(args, "ztype", url);
	url = p + 3;
	zb = zbuf_create(1024);
	zbuf_strcpy(zb, "main_arg=");
	zbuf_strcat(zb, url);

	zdict_parse_line(args, ZBUF_DATA(zb), ZBUF_LEN(zb));

	zbuf_free(zb);

	return 0;
}

int zmap_query_expand(ZBUF * zb, char *fmt, char *query, int flags)
{
	int last_ch = -1;
	char ch;

	ZBUF_RESET(zb);
	while ((ch = *fmt++)) {
		if (last_ch == '%') {
			last_ch = -1;
			if (ch == '%') {
				zbuf_put(zb, ch);
				continue;
			}
			if (ch == 's') {
				zbuf_strcat(zb, query);
				continue;
			}
			zbuf_put(zb, '%');
			zbuf_put(zb, ch);
			continue;
		}
		if (ch == '%') {
			last_ch = ch;
			continue;
		}
		zbuf_put(zb, ch);
	}
	if (last_ch != -1) {
		zbuf_put(zb, last_ch);
	}

	ZBUF_TERMINATE(zb);

	return 0;
}
