/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-06-26
 * ================================
 */

#include "zc.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

typeof(write) zsyscall_write;
typeof(open) zsyscall_open;
typeof(close) zsyscall_close;
typeof(rename) zsyscall_rename;
typeof(unlink) zsyscall_unlink;

static void *_pwrite(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].INT;
    const char *data = args[2].CONST_CHAR_PTR;
    int len = args[3].INT, wlen = 0, ret;
    long offset = args[4].LONG;

    if (offset > 0) {
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
            args[0].INT = -1;
            return 0;
        }
    }
    while (len > wlen) {
        ret = zsyscall_write(fd, data + wlen, len - wlen);
        if (ret > -1) {
            wlen += ret;
            continue;
        }
        if (errno == EINTR) {
            if (zvar_proc_stop) {
                break;
            }
            continue;
        }
        break;
    }
    if (len != wlen) {
        args[0].INT = -1;
    } else {
        args[0].INT = len;
    }

    return 0;
}

int zcoroutine_block_pwrite(int fd, const void *data, int len, long offset)
{
    ztype_convert_t args[5];
    args[0].INT = -1;
    args[1].INT = fd;
    args[2].CONST_CHAR_PTR = data;
    args[3].INT = len;
    args[4].LONG = offset;
    zcoroutine_block_do(_pwrite, args);
    return args[0].INT;
}

static void *_write(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].INT;
    const char *data = args[2].CONST_CHAR_PTR;
    int len = args[3].INT, wlen = 0, ret;

    while (len > wlen) {
        ret = zsyscall_write(fd, data + wlen, len - wlen);
        if (ret > -1) {
            wlen += ret;
            continue;
        }
        if (errno == EINTR) {
            if (zvar_proc_stop) {
                break;
            }
            continue;
        }
        break;
    }
    if (len != wlen) {
        args[0].INT = -1;
    } else {
        args[0].INT = len;
    }

    return 0;
}

int zcoroutine_block_write(int fd, const void *data, int len)
{
    ztype_convert_t args[4];
    args[0].INT = -1;
    args[1].INT = fd;
    args[2].CONST_CHAR_PTR = data;
    args[3].INT = len;
    zcoroutine_block_do(_write, args);
    return args[0].INT;
}

static void *_lseek(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].INT;
    long offset = args[2].LONG;;
    int whence = args[3].INT;
    args[0].LONG = lseek(fd, offset, whence);
    return 0;
}

long zcoroutine_block_lseek(int fd, long offset, int whence)
{
    ztype_convert_t args[4];
    args[0].LONG = -1;
    args[1].INT = fd;
    args[2].LONG = offset;
    args[3].INT = whence;
    zcoroutine_block_do(_lseek, args);
    return args[0].LONG;
}

static void *_open(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    const char *pathname = args[1].CONST_CHAR_PTR;
    int flags = args[2].INT;
    mode_t mode = args[3].INT;
    do {
        args[0].INT = zsyscall_open(pathname, flags, mode);
    } while ((args[0].INT < 0) && (errno==EINTR) && (zvar_proc_stop==0));
    return 0;
}

int zcoroutine_block_open(const char *pathname, int flags, mode_t mode)
{
    ztype_convert_t args[4];
    args[0].INT = -1;
    args[1].CONST_CHAR_PTR = pathname;
    args[2].INT = flags;
    args[3].INT = mode;
    zcoroutine_block_do(_open, args);
    return args[0].INT;
}

static void *_close(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].INT;
    do {
        args[0].INT = zsyscall_close(fd);
    } while ((args[0].INT < 0) && (errno==EINTR) && (zvar_proc_stop==0));
    return 0;
}

int zcoroutine_block_close(int fd)
{
    ztype_convert_t args[2];
    args[0].INT = -1;
    args[1].INT = fd;
    zcoroutine_block_do(_close, args);
    return args[0].INT;
}

static void *_rename(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    const char *oldpath = args[1].CONST_CHAR_PTR;
    const char *newpath = args[2].CONST_CHAR_PTR;
    do {
        args[0].INT = zsyscall_rename(oldpath, newpath);
    } while ((args[0].INT < 0) && (errno==EINTR) && (zvar_proc_stop==0));
    return 0;
}

int zcoroutine_block_rename(const char *oldpath, const char *newpath)
{
    ztype_convert_t args[3];
    args[0].INT = -1;
    args[1].CONST_CHAR_PTR = oldpath;
    args[2].CONST_CHAR_PTR = newpath;
    zcoroutine_block_do(_rename, args);
    return args[0].INT;
}

static void *_unlink(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    const char *pathname = args[1].CONST_CHAR_PTR;
    do {
        args[0].INT = zsyscall_unlink(pathname);
    } while ((args[0].INT < 0) && (errno==EINTR) && (zvar_proc_stop==0));
    return 0;
}

int zcoroutine_block_unlink(const char *pathname)
{
    ztype_convert_t args[2];
    args[0].INT = -1;
    args[1].CONST_CHAR_PTR = pathname;
    zcoroutine_block_do(_unlink, args);
    return args[0].INT;
}

