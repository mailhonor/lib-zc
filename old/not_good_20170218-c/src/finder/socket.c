/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-06
 * ================================
 */

#include "libzc.h"

int ___zfinder_read_line(zstream_t * fp, void *buf_void, int len, int *reach_end);

typedef struct zfinder_socket_t zfinder_socket_t;
struct zfinder_socket_t {
    char *module;
    int fd;
    zstream_t *fp;
};

static int _close(zfinder_t * finder)
{
    zfinder_socket_t *my_db;

    my_db = (zfinder_socket_t *) (finder->db);

    if (my_db->fp) {
        zstream_close_FD(my_db->fp);
    }
    if (my_db->fd != -1) {
        close(my_db->fd);
    }

    zfree(my_db);

    return 0;
}

static int ___close(zfinder_t * finder)
{
    zfinder_socket_t *my_db;

    my_db = (zfinder_socket_t *) (finder->db);
    if (my_db->fp) {
        zstream_close_FD(my_db->fp);
        my_db->fp = 0;
    }
    if (my_db->fd != -1) {
        close(my_db->fd);
        my_db->fd = -1;
    }

    return 0;
}

static int ___connect(zfinder_t * finder, int timeout)
{
    zfinder_socket_t *my_db;

    my_db = (zfinder_socket_t *) (finder->db);
    if (my_db->fp) {
        return 0;
    }

    if (my_db->fd < 0) {
        my_db->fd = zconnect(finder->uri, timeout);
    }
    if (my_db->fd < 0) {
        return -1;
    }

    my_db->fp = zstream_open_FD(my_db->fd);

    return 0;
}

static int ___get(zfinder_t * finder, const char *query, zbuf_t * result, int timeout)
{
    int i, ret, rend, len;
    long dtime = ztimeout_set(timeout);
    char buf[102400 + 10];
    zfinder_socket_t *mfinder;

    mfinder = (zfinder_socket_t *) (finder->db);

    for (i = 0; i < 2; i++) {
        zbuf_reset(result);
        if (i) {
            ___close(finder);
        }
        ret = ___connect(finder, ztimeout_left(dtime));
        if (ret < 0) {
            zbuf_printf_1024(result, "zfinder_get: %s : connection error", finder->title);
            return ret;
        }
        zstream_set_timeout(mfinder->fp, ztimeout_left(dtime));
        sprintf(buf, "%s %s\r\n", mfinder->module, query);
        if ((zstream_write_n(mfinder->fp, buf, strlen(buf)) < 0) || (ZSTREAM_FLUSH(mfinder->fp) < 0)) {
            zbuf_printf_1024(result, "zfinder_get: %s : write error", finder->title);
            continue;
        }

        ret = ___zfinder_read_line(mfinder->fp, buf, 102400, &rend);
        if ((ret < 3) || ((buf[0] != '5') && (ret < 4))) {
            zbuf_printf_1024(result, "zfinder_get: %s : read error", finder->title);
            continue;
        }
        if (buf[0] == '5') {
            return 0;
        }

        if (buf[0] == '4') {
            zbuf_printf_1024(result, "zfinder_get: %s : read error, from server: %s", finder->title, buf + 4);
            continue;
        }
        if (buf[0] != '2') {
            zbuf_printf_1024(result, "zfinder_get: %s : read error: protocal: %s", finder->title, buf);
            continue;
        }

        if (!strncmp(buf, "200", 3)) {
            zbuf_memcpy(result, buf + 4, ret - 4);
            zbuf_terminate(result);
            return 1;
        }
        if (!strncmp(buf, "210", 3)) {
            len = atoi(buf + 4);
            if (len > 102400) {
                zbuf_printf_1024(result, "zfinder_get: %s : read error, line too long: %d", finder->title, len);
                continue;
            }
            if (len > 0) {
                ret = zstream_read_n(mfinder->fp, buf, len);
                if (ret < 1) {
                    zbuf_printf_1024(result, "zfinder_get: %s : read error", finder->title);
                    continue;
                }
                zbuf_memcpy(result, buf, ret);
                zbuf_terminate(result);
            }
            ret = ___zfinder_read_line(mfinder->fp, buf, 102400, &rend);
            if (ret != 0) {
                zbuf_printf_1024(result, "zfinder_get: %s : read error", finder->title);
                continue;
            }
            return 1;
        }

        zbuf_printf_1024(result, "zfinder_get: %s : read error: protocal: %s", finder->title, buf);
        continue;
    }

    return -1;
}

static int _get(zfinder_t * finder, const char *query, zbuf_t * result, int timeout)
{
    int ret;

    ret = ___get(finder, query, result, timeout);

    return ret;
}

int zfinder_create_socket(zfinder_t * finder)
{
    zfinder_socket_t *my_db;

    my_db = (zfinder_socket_t *)zcalloc(1, sizeof(zfinder_socket_t));
    finder->db = my_db;
    finder->close = _close;
    finder->get = _get;

    my_db->fd = -1;
    my_db->fp = 0;
    if (!zdict_lookup(finder->parameters, "module", &(my_db->module))){
        zfree(my_db);
        finder->db = 0;
        return -1;
    } 

    return 0;
}
