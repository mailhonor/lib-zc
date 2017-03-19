/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-07
 * ================================
 */

#include "libzc.h"

int zquery_line(const char *connection, const char *query, char *result, int timeout)
{
    int fd;
    zstream_t *fp;
    int len;

    if (timeout < 1) {
        timeout = 3600 * 24 * 1000;
    }
    fd = zconnect(connection, timeout);
    if (fd < 0) {
        sprintf(result, "connect error: %s, %m", connection);
        return -1;
    }
    fp = zstream_open_FD(fd);
    zstream_set_timeout(fp, timeout);

    zstream_printf_1024(fp, "%s\r\n", query);
    if (ZSTREAM_FLUSH(fp) < 0) {
        zstream_printf_1024(fp, "write error: %s", connection);
        zstream_close_FD(fp);
        close(fd);
        return -1;
    }
    len = zstream_read_line(fp, result, 1000);
    if (len < 1) {
        zstream_printf_1024(fp, "read error: %s", connection);
        zstream_close_FD(fp);
        close(fd);
        return -1;
    }
    if ((len > 0) && (result[len - 1] == '\n')) {
        len--;
    }
    if ((len > 0) && (result[len - 1] == '\r')) {
        len--;
    }
    result[len] = 0;

    zstream_close_FD(fp);
    close(fd);

    return len;
}
