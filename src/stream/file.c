/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-19
 * ================================
 */

#include "zc.h"
#include <errno.h>
#ifdef _WIN64
#endif // _WIN64
#include <fcntl.h>

static const char *fp_get_type()
{
    return "_Z_FILE";
}

static int fp_close(zstream_t *fp, int release_ioctx)
{
    ztype_convert_t ct;
    ct.VOID_PTR = fp->ioctx;
    int fd = ct.INT;

    int ret = 0;
    if (release_ioctx) {
        ret = zclose(fd);
    }

    return ret;
}

static int fp_read(zstream_t *fp, void *buf, int len)
{
    ztype_convert_t ct;
    ct.VOID_PTR = fp->ioctx;
    int fd = ct.INT;

    int ret = ztimed_read(fd, buf, len, 0);
    if (ret < 0) {
        if (errno == EAGAIN) {
            zfatal("zstream_t: can not use nonblocking fd in file mode");
        }
    }

    return ret;
}

static int fp_write(zstream_t *fp, const void *buf, int len)
{
    ztype_convert_t ct;
    ct.VOID_PTR = fp->ioctx;
    int fd = ct.INT;

    return ztimed_write(fd, buf, len, 0);
}

static int fp_timed_read_wait(zstream_t *fp, int read_wait_timeout)
{
    ztype_convert_t ct;
    ct.VOID_PTR = fp->ioctx;
    int fd = ct.INT;

    return ztimed_read_wait(fd, 0);
}

static int fp_timed_write_wait(zstream_t *fp, int write_wait_timeout)
{
    ztype_convert_t ct;
    ct.VOID_PTR = fp->ioctx;
    int fd = ct.INT;

    return ztimed_write_wait(fd, 0);
}

static int fp_get_fd(zstream_t *fp)
{
    ztype_convert_t ct;
    ct.VOID_PTR = fp->ioctx;
    int fd = ct.INT;

    return fd;
}

static zstream_engine_t fp_engine = {
    fp_get_type,
    fp_close,
    fp_read,
    fp_write,
    fp_timed_read_wait,
    fp_timed_write_wait,
    fp_get_fd
};

zstream_t *zstream_open_file_engine(zstream_t *fp, const char *pathname, const char *mode)
{
    int flags = 0;
    if (*mode == 'r') {
        flags = O_RDONLY;
        if (mode[1] == '+') {
            flags = O_RDWR;
        }
    } else  if (*mode == 'w') {
        flags = O_WRONLY|O_TRUNC|O_CREAT;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT;
        }
    } else  if (*mode == 'a') {
        flags = O_WRONLY|O_TRUNC|O_CREAT|O_APPEND;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT|O_APPEND;
        }
    } else {
        flags = O_RDONLY;
    }
    int fd = open(pathname, flags, 0666);
    if (fd == -1) {
        return 0;
    }

    memset(fp, 0, sizeof(zstream_t));
    fp->read_buf_p1 = 0;
    fp->read_buf_p2 = 0;
    fp->write_buf_len = 0;
    fp->read_wait_timeout = -1;
    fp->write_wait_timeout = -1;
    fp->error = 0;
    fp->eof = 0;
    fp->engine = &fp_engine;
    ztype_convert_t ct;
    ct.INT = fd;
    fp->ioctx = ct.VOID_PTR;
    return fp;
}

zstream_t *zstream_open_file(const char *pathname, const char *mode)
{
#ifdef _WIN64
    zfatal("zstream_open_file, not supported ON win32");
#endif // _WIN64
    zstream_t *fp = (zstream_t *)zmalloc(sizeof(zstream_t));
    if (!zstream_open_file_engine(fp, pathname, mode)) {
        zfree(fp);
        fp = 0;
    }
    return fp;
}

