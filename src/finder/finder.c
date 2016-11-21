/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-04
 * ================================
 */

#include "libzc.h"

int (*zfinder_create_extend_fn)(zfinder_t *finder, const char *type) = 0;

int zfinder_create_static(zfinder_t *finder);
int zfinder_create_memcache(zfinder_t *finder);
int zfinder_create_redis(zfinder_t *finder);
int zfinder_create_flat(zfinder_t *finder);
int zfinder_create_socketline(zfinder_t *finder);
int zfinder_create_socket(zfinder_t *finder);
int zfinder_create_postproxy(zfinder_t *finder);

static void ___zfinder_parse_parameters(zdict_t *parameters, const char *str);

zfinder_t *zfinder_create(const char *title)
{
    zfinder_t *finder;
    char type[56];
    char *p;
    char *uri;
    int ret;
    int len, uri_len, title_len;
    zdict_t *parameters = 0;

    if (ZEMPTY(title)) {
        zfatal("zfinder_create: title is blank");
    }
    title_len = strlen(title);

    p = strstr(title, "://");
    if (!p) {
        zfatal("zfinder_create: can not create %s", title);
    }
    len = p - title;
    if (len > 50) {
        zfatal("zfinder_create: can not create %s", title);
    }
    memcpy(type, title, len);
    type[len] = 0;
    ztolower(type);

    parameters = zdict_create();

    uri = p + 3;
    p = strchr(uri, '?');
    if (!p) {
        uri_len = strlen(uri);
    } else {
        uri_len = p - uri;
        ___zfinder_parse_parameters(parameters, p + 1);
    }

    finder = (zfinder_t *)zcalloc(1, sizeof(zfinder_t) + (title_len + 1) + (uri_len + 1));
    finder->title = (char *)finder + sizeof(zfinder_t);
    finder->uri = (char *)finder + sizeof(zfinder_t) + (title_len + 1);
    memcpy(finder->title, title, title_len);
    memcpy(finder->uri, uri, uri_len);
    if (zdict_lookup(parameters, "prefix", &p)) {
        finder->prefix = p;
    }
    if (zdict_lookup(parameters, "suffix", &p)) {
        finder->suffix = p;
    }
    finder->parameters = parameters;

    ret = -1;
    if (!strcmp(type, "static")) {
        ret = zfinder_create_static(finder);
    } else if (!strcmp(type, "memcache")) {
        ret = zfinder_create_memcache(finder);
    } else if (!strcmp(type, "redis")) {
        ret = zfinder_create_redis(finder);
    } else if (!strcmp(type, "flat")) {
        ret = zfinder_create_flat(finder);
    } else if (!strcmp(type, "socketline")) {
        ret = zfinder_create_socketline(finder);
    } else if (!strcmp(type, "socket")) {
        ret = zfinder_create_socket(finder);
    } else if (!strcmp(type, "postproxy")) {
        ret = zfinder_create_postproxy(finder);
    } else if (zfinder_create_extend_fn) {
        ret = zfinder_create_extend_fn(finder, type);
    } else {
        zfatal("zfinder_create: can not create %s", title);
    }

    if (ret < 0) {
        zfatal("finder_create: ERROR");
    }

    return finder;
}

void zfinder_close(zfinder_t * finder)
{
    if (finder == 0){
        return;
    }
    finder->close(finder);
    zdict_free(finder->parameters);
    zfree(finder);
}

int zfinder_get(zfinder_t * finder, const char *query, zbuf_t * result, int timeout)
{
    char ch, *p;
    ZSTACK_BUF(query_new, 1024);
    int ret;

    zbuf_reset(result);

    if (!finder) {
        return -1;
    }
    p = (char *)query;

    while (*p) {
        ch = *p++;
        if ((!isprint(ch)) || isblank(ch)) {
            zbuf_strcpy(result, "zfinder_get: query include illegal character");
            return -1;
        }
    }

    if (timeout < 1) {
        timeout = 3600 * 24;
    }

    zbuf_reset(query_new);
    if (finder->prefix) {
        zbuf_strcat(query_new, finder->prefix);
    }
    zbuf_strcat(query_new, query);
    if (finder->suffix) {
        zbuf_strcat(query_new, finder->suffix);
    }

    ret = finder->get(finder, ZBUF_DATA(query_new), result, timeout);
    zbuf_terminate(result);

    return ret;
}

int zfinder_get_once(const char *title, const char *key, zbuf_t * result, int timeout)
{
    zfinder_t *finder;
    int ret;

    finder = zfinder_create(title);
    if (!finder) {
        zfatal("zfinder_create: can not create %s", title);
    }
    ret = zfinder_get(finder, key, result, timeout);
    zfinder_close(finder);

    return ret;
}

int ___zfinder_read_line(zstream_t * fp, void *buf_void, int len, int *reach_end)
{
    char *buf = (char *)buf_void;

    if (reach_end) {
        *reach_end = 1;
    }

    len = zstream_read_line(fp, buf, len);
    if (len < 1) {
        return -1;
    }

    if ((len > 0) && (buf[len - 1] == '\n')) {
        len--;
        if (reach_end) {
            *reach_end = 1;
        }
    }
    if ((len > 0) && (buf[len - 1] == '\r')) {
        len--;
    }
    buf[len] = 0;

    return len;
}

static void ___zfinder_parse_parameters(zdict_t *parameters, const char *str)
{
    char *ps, *p;
    zargv_t *args;

    args = zargv_create(11);
    zargv_split_append(args, (char *)str, "&");

    ZARGV_WALK_BEGIN(args, ps) {
        p = strchr(ps, '=');
        if(p) {
            *p++ = 0;
        } else {
            p = "";
        }
        zdict_add(parameters, ps, p);
    } ZARGV_WALK_END;
    zargv_free(args);
}

