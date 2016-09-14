/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-07
 * ================================
 */

#include "libzc.h"


int zquery_line(char *connection, char *query, char *result, int timeout)
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
    fp = zfopen_FD(fd);
    zfset_timeout(fp, timeout);

    zfprintf(fp, "%s\r\n", query);
    if (ZFFLUSH(fp) < 0) {
        zfprintf(fp, "write error: %s", connection);
        zfclose_FD(fp);
        close(fd);
        return -1;
    }
    len = zfread_line(fp, result, 1000);
    if (len < 1) {
        zfprintf(fp, "read error: %s", connection);
        zfclose_FD(fp);
        close(fd);
        return -1;
    }
    if ((len>0) && (result[len-1] == '\n')) {
        len --;
    }
    if ((len>0) && (result[len-1] == '\r')) {
        len --;
    }
    result[len] = 0;

    zfclose_FD(fp);
    close(fd);

    return len;
}
