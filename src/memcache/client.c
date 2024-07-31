/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-11-25
 * ================================
 */

#include "zc.h"
#include <ctype.h>

#define ___check_status() {if(mc->fd == -1){zmemcache_client_connect_inner(mc);}if(mc->fd ==-1) {return -1;}} 
#define ___check_key() {if(!is_valid_key(key)){return -1;}}

struct zmemcache_client_t {
    char *destination;
    int fd:31;
    unsigned int auto_reconnect:1;
    int connect_timeout;
    int read_wait_timeout;
    int write_wait_timeout;
};

static zbool_t is_valid_key(const char *key)
{
    if (zempty(key)) {
        return 0;
    }
    while (*key) {
        if (iscntrl(*key) || isblank(*key)) {
            return 0;
        }
        key ++;
    }
    return 1;
}

static void zmemcache_client_close_inner(zmemcache_client_t *mc)
{
    if(!mc) {
        return;
    }
    if (mc->fd != -1) {
        zclosesocket(mc->fd);
        mc->fd = -1;
    }
}

static void zmemcache_client_connect_inner(zmemcache_client_t *mc)
{
    if (mc->fd != -1) {
        return;
    }
    mc->fd = zconnect(mc->destination, mc->connect_timeout);
    znonblocking(mc->fd, 1);
}

zmemcache_client_t *zmemcache_client_connect(const char *destination, int connect_timeout)
{
    int fd = zconnect(destination, connect_timeout);
    if (fd < 0) {
        return 0;
    }
    znonblocking(fd, 1);
    zmemcache_client_t *mc = (zmemcache_client_t *)zcalloc(1, sizeof(zmemcache_client_t));
    mc->destination = zstrdup(destination);
    mc->fd = fd;
    mc->connect_timeout = connect_timeout;
    mc->read_wait_timeout = -1;
    mc->write_wait_timeout = -1;
    mc->auto_reconnect = 0;
    return mc;
}

void zmemcache_client_set_connect_timeout(zmemcache_client_t *mc, int connect_timeout)
{
    mc->connect_timeout = connect_timeout;
}

void zmemcache_client_set_read_wait_timeout(zmemcache_client_t *mc, int read_wait_timeout)
{
    mc->read_wait_timeout = read_wait_timeout;
}

void zmemcache_client_set_write_wait_timeout(zmemcache_client_t *mc, int write_wait_timeout)
{
    mc->write_wait_timeout = write_wait_timeout;
}

void zmemcache_client_set_auto_reconnect(zmemcache_client_t *mc, zbool_t auto_reconnect)
{
    mc->auto_reconnect = auto_reconnect;
}

void zmemcache_client_disconnect(zmemcache_client_t *mc)
{
    if (mc) {
        zmemcache_client_close_inner(mc);
        zfree(mc->destination);
        zfree(mc);
    }
}

int zmemcache_client_get(zmemcache_client_t *mc, const char *key, int *flag, zbuf_t *value)
{
    if (value) {
        zbuf_reset(value);
    }
    ___check_status();
    ___check_key();
    zstream_t *fp = zstream_open_fd(mc->fd);
    zstream_set_read_wait_timeout(fp, mc->read_wait_timeout);
    zstream_set_write_wait_timeout(fp, mc->write_wait_timeout);
    zstream_printf_1024(fp, "get %s\r\n", key);
    int have_val = 0;
    int protocol_error = 0;
    zbuf_t *str = zbuf_create(-1);
    while(1) {
        zbuf_reset(str);
        if (zstream_gets(fp, str, 1024) < 1) {
            zmemcache_client_close_inner(mc);
            protocol_error = 1;
            break;
        }
        char *p, *ps = zbuf_data(str);
        if (!strncmp(ps, "VALUE ", 6)) {
            p = strchr(ps + 6, ' ');
            if (!p) {
                protocol_error = 1;
                break;
            }
            ps = p + 1;
            p = strchr(ps, ' ');
            if (!p) {
                protocol_error = 1;
                break;
            }
            if (flag) {
                *p = 0;
                *flag = atoi(ps);
                *p = ' ';
            }
            ps = p + 1;
            int len = atoi(ps);
            if (len < 0) {
                protocol_error = 1;
                break;
            }
            if (value) {
                zbuf_reset(value);
            }
            if ((zstream_readn(fp, value, len) < len) || (zstream_readn(fp, 0, 2) < 2)) {
                protocol_error = 1;
                break;
            }
            have_val = 1;
            continue;
        }
        if (!strncmp(ps, "END", 3)) {
            break;
        }
        protocol_error = 1;
        break;
    }
    zbuf_free(str);
    zstream_close(fp, 0);
    if (protocol_error) {
        zmemcache_client_close_inner(mc);
        return -1;
    }
    return (have_val?1:0);
}

static int zmemcache_client_asrpa(zmemcache_client_t *mc, const char *op, const char *key, int flag, long timeout, const void *data, int len)
{
    int ret = -1;
    ___check_status();
    ___check_key();
    ZSTACK_BUF(str, 1024);
    zstream_t *fp = zstream_open_fd(mc->fd);
    zstream_set_read_wait_timeout(fp, mc->read_wait_timeout);
    zstream_set_write_wait_timeout(fp, mc->write_wait_timeout);
    zstream_printf_1024(fp, "%s %s %d %ld %d\r\n", op, key, flag, timeout, len);
    zstream_write(fp, data, len);
    zstream_write(fp, "\r\n", 2);
    if (zstream_gets(fp, str, 1024) < 1) {
        ret = -1;
        goto over;
    }
    char *ps = zbuf_data(str);
    if (!strncmp(ps, "STORED", 6)) {
        ret = 1;
        goto over;
    }
    if (!strncmp(ps, "NOT_STORED", 10)) {
        ret = 0;
        goto over;
    }

over:
    zstream_close(fp, 0);
    if (ret < 0) {
        zmemcache_client_close_inner(mc);
    }
    return ret;
}

int zmemcache_client_add(zmemcache_client_t *mc, const char *key, int flag, ssize_t timeout, const void *data, int len)
{
    return zmemcache_client_asrpa(mc, "add", key, flag, timeout, data, len);
}

int zmemcache_client_set(zmemcache_client_t *mc, const char *key, int flag, ssize_t timeout, const void *data, int len)
{
    return zmemcache_client_asrpa(mc, "set", key, flag, timeout, data, len);
}


int zmemcache_client_replace(zmemcache_client_t *mc, const char *key, int flag, ssize_t timeout, const void *data, int len)
{
    return zmemcache_client_asrpa(mc, "replace", key, flag, timeout, data, len);
}


int zmemcache_client_append(zmemcache_client_t *mc, const char *key, int flag, ssize_t timeout, const void *data, int len)
{
    return zmemcache_client_asrpa(mc, "append", key, flag, timeout, data, len);
}

int zmemcache_client_prepend(zmemcache_client_t *mc, const char *key, int flag, ssize_t timeout, const void *data, int len)
{
    return zmemcache_client_asrpa(mc, "prepend", key, flag, timeout, data, len);
}


static ssize_t zmemcache_client_incr_decr(zmemcache_client_t *mc, const char *op, const char *key, size_t n)
{
    ___check_status();
    ___check_key();
    ZSTACK_BUF(str, 1024);
    zstream_t *fp = zstream_open_fd(mc->fd);
    zstream_set_read_wait_timeout(fp, mc->read_wait_timeout);
    zstream_set_write_wait_timeout(fp, mc->write_wait_timeout);
    zstream_printf_1024(fp, "%s %s %zd\r\n", op, key, n);
    int ret = zstream_gets(fp, str, 1024);
    zstream_close(fp, 0);
    if (ret < 1) {
        zmemcache_client_close_inner(mc);
        return -1;
    }
    char *ps = zbuf_data(str);
    if (isdigit(ps[0])) {
        return atoll(ps);
    }
    zmemcache_client_close_inner(mc);
    return -1;
}

ssize_t zmemcache_client_incr(zmemcache_client_t *mc, const char *key, size_t n)
{
    return zmemcache_client_incr_decr(mc, "incr", key, n);
}

ssize_t zmemcache_client_decr(zmemcache_client_t *mc, const char *key, size_t n)
{
    return zmemcache_client_incr_decr(mc, "decr", key, n);
}


int zmemcache_client_del(zmemcache_client_t *mc, const char *key)
{
    int ret = -1;
    ___check_status();
    ___check_key();
    ZSTACK_BUF(str, 1024);
    zstream_t *fp = zstream_open_fd(mc->fd);
    zstream_set_read_wait_timeout(fp, mc->read_wait_timeout);
    zstream_set_write_wait_timeout(fp, mc->write_wait_timeout);
    zstream_printf_1024(fp, "delete %s\r\n", key);
    if (zstream_gets(fp, str, 1024) < 1) {
        ret = -1;
        goto over;
    }
    char *ps = zbuf_data(str);
    if (!strncmp(ps, "DELETED", 7)) {
        ret = 1;
        goto over;
    } else if (!strncmp(ps, "NOT_FOUND", 9)) {
        ret = 0;
        goto over;
    }
over:
    zstream_close(fp, 0);
    if (ret < 0) {
        zmemcache_client_close_inner(mc);
    }
    return ret;
}


int zmemcache_client_flush_all(zmemcache_client_t *mc, ssize_t after_second)
{
    int ret = -1;
    ___check_status();
    ZSTACK_BUF(str, 1024);
    zstream_t *fp = zstream_open_fd(mc->fd);
    zstream_set_read_wait_timeout(fp, mc->read_wait_timeout);
    zstream_set_write_wait_timeout(fp, mc->write_wait_timeout);
    if (after_second > 0) {
        zstream_printf_1024(fp, "flush_all %zd\r\n", after_second);
    } else {
        zstream_puts(fp, "flush_all\r\n");
    }
    if (zstream_gets(fp, str, 1024) < 1) {
        ret = -1;
        goto over;
    }
    char *ps = zbuf_data(str);
    if (!strncmp(ps, "OK", 2)) {
        ret = 1;
        goto over;
    } else {
        ret = 0;
    }
over:
    zstream_close(fp, 0);
    if (ret < 0) {
        zmemcache_client_close_inner(mc);
    }
    return ret;
}

int zmemcache_client_version(zmemcache_client_t *mc, zbuf_t *version)
{
    int ret = -1;
    zbuf_reset(version);
    ___check_status();
    ZSTACK_BUF(str, 1024);
    zstream_t *fp = zstream_open_fd(mc->fd);
    zstream_set_read_wait_timeout(fp, mc->read_wait_timeout);
    zstream_set_write_wait_timeout(fp, mc->write_wait_timeout);
    zstream_puts(fp, "version\r\n");
    if (zstream_gets(fp, str, 1024) < 1) {
        ret = -1;
        goto over;
    }
    char *ps = zbuf_data(str);
    if (strncmp(ps, "VERSION ", 8)) {
        ret = -1;
        goto over;
    } else {
        zbuf_puts(version, ps+8);
        ret = 1;
        goto over;
    }
over:
    zstream_close(fp, 0);
    if (ret < 0) {
        zmemcache_client_close_inner(mc);
    }
    return ret;
}
