/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
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

static void *_pwrite(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].i_int;
    const char *data = args[2].ptr_const_char;
    int len = args[3].i_int, wlen = 0, ret;
    long offset = args[4].i_long;

    if (offset > 0) {
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
            args[0].i_int = -1;
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
        args[0].i_int = -1;
    } else {
        args[0].i_int = len;
    }

    return 0;
}

int zcoroutine_block_pwrite(int fd, const void *data, int len, long offset)
{
    ztype_convert_t args[5];
    args[0].i_int = -1;
    args[1].i_int = fd;
    args[2].ptr_const_char = data;
    args[3].i_int = len;
    args[4].i_long = offset;
    zcoroutine_block_do(_pwrite, args);
    return args[0].i_int;
}

static void *_write(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].i_int;
    const char *data = args[2].ptr_const_char;
    int len = args[3].i_int, wlen = 0, ret;

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
        args[0].i_int = -1;
    } else {
        args[0].i_int = len;
    }

    return 0;
}

int zcoroutine_block_write(int fd, const void *data, int len)
{
    ztype_convert_t args[4];
    args[0].i_int = -1;
    args[1].i_int = fd;
    args[2].ptr_const_char = data;
    args[3].i_int = len;
    zcoroutine_block_do(_write, args);
    return args[0].i_int;
}

static void *_lseek(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].i_int;
    long offset = args[2].i_long;;
    int whence = args[3].i_int;
    args[0].i_long = lseek(fd, offset, whence);
    return 0;
}

long zcoroutine_block_lseek(int fd, long offset, int whence)
{
    ztype_convert_t args[4];
    args[0].i_long = -1;
    args[1].i_int = fd;
    args[2].i_long = offset;
    args[3].i_int = whence;
    zcoroutine_block_do(_lseek, args);
    return args[0].i_long;
}

static void *_open(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    const char *pathname = args[1].ptr_const_char;
    int flags = args[2].i_int;
    mode_t mode = args[3].i_int;
    do {
        args[0].i_int = zsyscall_open(pathname, flags, mode);
    } while ((args[0].i_int < 0) && (errno==EINTR));
    return 0;
}

int zcoroutine_block_open(const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    ztype_convert_t args[4];
    args[0].i_int = -1;
    args[1].ptr_const_char = pathname;
    args[2].i_int = flags;
    args[3].i_int = mode;
    zcoroutine_block_do(_open, args);
    return args[0].i_int;
}

static void *_close(void *ctx)
{
    ztype_convert_t *args = (ztype_convert_t *)ctx;
    int fd = args[1].i_int;
    do {
        args[0].i_int = zsyscall_close(fd);
    } while ((args[0].i_int < 0) && (errno==EINTR));
    return 0;
}

int zcoroutine_block_close(int fd)
{
    ztype_convert_t args[2];
    args[0].i_int = -1;
    args[1].i_int = fd;
    zcoroutine_block_do(_close, args);
    return args[0].i_int;
}
