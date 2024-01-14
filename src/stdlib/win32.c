/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-20
 * ================================
 */

#ifdef _WIN32

#include "zc.h"
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <Winsock2.h>
#include <windows.h>
#include <sys/stat.h>

int zWSAStartup()
{
    static int err = 1;
    static pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
    static int _init_flag = 0;
    if (_init_flag == 0)
    {
        zpthread_lock(&locker);
        if (_init_flag == 0)
        {
            WSADATA wsaData = {};
            err = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (err)
            {
                err = -1;
            }
            else
            {
                err = 1;
            }
            _init_flag = 1;
        }
        zpthread_unlock(&locker);
    }
    return err;
}

int zwin32_code_to_errno(unsigned long w32Err)
{
    size_t i;

    struct code_to_errno_map
    {
        unsigned long w32Err;
        int eerrno;
    };

    static const struct code_to_errno_map errmap[] =
        {
            /*    1 */ {ERROR_INVALID_FUNCTION, EINVAL},
            /*    2 */ {ERROR_FILE_NOT_FOUND, ENOENT},
            /*    3 */ {ERROR_PATH_NOT_FOUND, ENOENT},
            /*    4 */ {ERROR_TOO_MANY_OPEN_FILES, EMFILE},
            /*    5 */ {ERROR_ACCESS_DENIED, EACCES},
            /*    6 */ {ERROR_INVALID_HANDLE, EBADF},
            /*    7 */ {ERROR_ARENA_TRASHED, ENOMEM},
            /*    8 */ {ERROR_NOT_ENOUGH_MEMORY, ENOMEM},
            /*    9 */ {ERROR_INVALID_BLOCK, ENOMEM},
            /*   10 */ {ERROR_BAD_ENVIRONMENT, E2BIG},
            /*   11 */ {ERROR_BAD_FORMAT, ENOEXEC},
            /*   12 */ {ERROR_INVALID_ACCESS, EINVAL},
            /*   13 */ {ERROR_INVALID_DATA, EINVAL},
            /*   14 */ {ERROR_OUTOFMEMORY, ENOMEM},
            /*   15 */ {ERROR_INVALID_DRIVE, ENOENT},
            /*   17 */ {ERROR_NOT_SAME_DEVICE, EXDEV},
            /*   18 */ {ERROR_NO_MORE_FILES, ENOENT},
            /*   19 */ {ERROR_WRITE_PROTECT, EROFS},
            /*   20 */ {ERROR_BAD_UNIT, ENXIO},
            /*   21 */ {ERROR_NOT_READY, EBUSY},
            /*   22 */ {ERROR_BAD_COMMAND, EIO},
            /*   23 */ {ERROR_CRC, EIO},
            /*   24 */ {ERROR_BAD_LENGTH, EIO},
            /*   25 */ {ERROR_SEEK, EIO},
            /*   26 */ {ERROR_NOT_DOS_DISK, EIO},
            /*   27 */ {ERROR_SECTOR_NOT_FOUND, ENXIO},
            /*   28 */ {ERROR_OUT_OF_PAPER, EBUSY},
            /*   29 */ {ERROR_WRITE_FAULT, EIO},
            /*   30 */ {ERROR_READ_FAULT, EIO},
            /*   31 */ {ERROR_GEN_FAILURE, EIO},
            /*   32 */ {ERROR_SHARING_VIOLATION, EAGAIN},
            /*   33 */ {ERROR_LOCK_VIOLATION, EACCES},
            /*   34 */ {ERROR_WRONG_DISK, ENXIO},
            /*   35 */ {35, ENFILE},
            /*   36 */ {ERROR_SHARING_BUFFER_EXCEEDED, ENFILE},
            /*   37 */ {ERROR_HANDLE_EOF, EINVAL},
            /*   38 */ {ERROR_HANDLE_DISK_FULL, ENOSPC},
            /*   50 */ {ERROR_NOT_SUPPORTED, ENOSYS},
            /*   53 */ {ERROR_BAD_NETPATH, ENOENT},
            /*   65 */ {ERROR_NETWORK_ACCESS_DENIED, EACCES},
            /*   67 */ {ERROR_BAD_NET_NAME, ENOENT},
            /*   80 */ {ERROR_FILE_EXISTS, EEXIST},
            /*   82 */ {ERROR_CANNOT_MAKE, EACCES},
            /*   83 */ {ERROR_FAIL_I24, EACCES},
            /*   87 */ {ERROR_INVALID_PARAMETER, EINVAL},
            /*   89 */ {ERROR_NO_PROC_SLOTS, EAGAIN},
            /*  108 */ {ERROR_DRIVE_LOCKED, EACCES},
            /*  109 */ {ERROR_BROKEN_PIPE, EPIPE},
            /*  111 */ {ERROR_BUFFER_OVERFLOW, ENAMETOOLONG},
            /*  112 */ {ERROR_DISK_FULL, ENOSPC},
            /*  114 */ {ERROR_INVALID_TARGET_HANDLE, EBADF},
            /*  122 */ {ERROR_INSUFFICIENT_BUFFER, ERANGE},
            /*  123 */ {ERROR_INVALID_NAME, ENOENT},
            /*  124 */ {ERROR_INVALID_HANDLE, EINVAL},
            /*  126 */ {ERROR_MOD_NOT_FOUND, ENOENT},
            /*  127 */ {ERROR_PROC_NOT_FOUND, ENOENT},
            /*  128 */ {ERROR_WAIT_NO_CHILDREN, ECHILD},
            /*  129 */ {ERROR_CHILD_NOT_COMPLETE, ECHILD},
            /*  130 */ {ERROR_DIRECT_ACCESS_HANDLE, EBADF},
            /*  131 */ {ERROR_NEGATIVE_SEEK, EINVAL},
            /*  132 */ {ERROR_SEEK_ON_DEVICE, EACCES},
            /*  145 */ {ERROR_DIR_NOT_EMPTY, ENOTEMPTY},
            /*  158 */ {ERROR_NOT_LOCKED, EACCES},
            /*  161 */ {ERROR_BAD_PATHNAME, ENOENT},
            /*  164 */ {ERROR_MAX_THRDS_REACHED, EAGAIN},
            /*  167 */ {ERROR_LOCK_FAILED, EACCES},
            /*  183 */ {ERROR_ALREADY_EXISTS, EEXIST},
            /*  206 */ {ERROR_FILENAME_EXCED_RANGE, ENAMETOOLONG},
            /*  215 */ {ERROR_NESTING_NOT_ALLOWED, EAGAIN},
            /*  258 */ {WAIT_TIMEOUT, ETIME},
            /*  267 */ {ERROR_DIRECTORY, ENOTDIR},
            /*  996 */ {ERROR_IO_INCOMPLETE, EAGAIN},
            /*  997 */ {ERROR_IO_PENDING, EAGAIN},
            /* 1004 */ {ERROR_INVALID_FLAGS, EINVAL},
            /* 1113 */ {ERROR_NO_UNICODE_TRANSLATION, EINVAL},
            /* 1168 */ {ERROR_NOT_FOUND, ENOENT},
            /* 1224 */ {ERROR_USER_MAPPED_FILE, EACCES},
            /* 1314 */ {ERROR_PRIVILEGE_NOT_HELD, EACCES},
            /* 1816 */ {ERROR_NOT_ENOUGH_QUOTA, ENOMEM},
            /*      */ {ERROR_ABANDONED_WAIT_0, EIO},
            /* 4390 */ {ERROR_NOT_A_REPARSE_POINT, EINVAL},
            // WSA
            /*    4 */ {WSAEINTR, EINTR},
            /*    9 */ {WSAEBADF, EBADF},
            /*   13 */ {WSAEACCES, EACCES},
            /*   14 */ {WSAEFAULT, EFAULT},
            /*   22 */ {WSAEINVAL, EINVAL},
            /*   24 */ {WSAEMFILE, EMFILE},
            /*   35 */ {WSAEWOULDBLOCK, EWOULDBLOCK},
            /*   36 */ {WSAEINPROGRESS, EINPROGRESS},
            /*   37 */ {WSAEALREADY, EALREADY},
            /*   38 */ {WSAENOTSOCK, ENOTSOCK},
            /*   39 */ {WSAEDESTADDRREQ, EDESTADDRREQ},
            /*   40 */ {WSAEMSGSIZE, EMSGSIZE},
            /*   41 */ {WSAEPROTOTYPE, EPROTOTYPE},
            /*   42 */ {WSAENOPROTOOPT, ENOPROTOOPT},
            /*   43 */ {WSAEPROTONOSUPPORT, EPROTONOSUPPORT},
            /*   45 */ {WSAEOPNOTSUPP, EOPNOTSUPP},
            /*   47 */ {WSAEAFNOSUPPORT, EAFNOSUPPORT},
            /*   48 */ {WSAEADDRINUSE, EADDRINUSE},
            /*   49 */ {WSAEADDRNOTAVAIL, EADDRNOTAVAIL},
            /*   50 */ {WSAENETDOWN, ENETDOWN},
            /*   51 */ {WSAENETUNREACH, ENETUNREACH},
            /*   52 */ {WSAENETRESET, ENETRESET},
            /*   53 */ {WSAECONNABORTED, ECONNABORTED},
            /*   54 */ {WSAECONNRESET, ECONNRESET},
            /*   55 */ {WSAENOBUFS, ENOBUFS},
            /*   56 */ {WSAEISCONN, EISCONN},
            /*   57 */ {WSAENOTCONN, ENOTCONN},
            /*   60 */ {WSAETIMEDOUT, ETIMEDOUT},
            /*   61 */ {WSAECONNREFUSED, ECONNREFUSED},
            /*   62 */ {WSAELOOP, ELOOP},
            /*   63 */ {WSAENAMETOOLONG, ENAMETOOLONG},
            /*   65 */ {WSAEHOSTUNREACH, EHOSTUNREACH},
            /*   66 */ {WSAENOTEMPTY, ENOTEMPTY},
            {-1, -1}};

    if (w32Err == 0)
    {
        w32Err = GetLastError();
    }
    for (i = 0; i < sizeof(errmap) / sizeof(struct code_to_errno_map); ++i)
    {
        if (w32Err == errmap[i].w32Err)
        {
            return errmap[i].eerrno;
        }
    }

    return EINVAL;
}

int zUtf8ToWideChar(const char *in, int in_len, wchar_t *result_ptr, int result_size)
{
    wchar_t *result_buf = (wchar_t *)result_ptr;
    result_buf[0] = L'\0';
    int ret_len = MultiByteToWideChar(CP_UTF8, 0, in, in_len, NULL, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    ret_len = MultiByteToWideChar(CP_UTF8, 0, in, in_len, result_buf, result_size);
    if (ret_len < 1)
    {
        return -1;
    }
    if (in_len < 0)
    {
        result_buf[ret_len - 1] = L'\0';
        return ret_len - 1;
    }
    else
    {
        result_buf[ret_len] = L'\0';
        return ret_len;
    }
}


// UTF-16 (wide character)
// 返回结果是字符数
// result_size 是 结果(result_ptr)buffer 的字符数
// in_len 是 输入(in) 的字节数
// https://learn.microsoft.com/zh-cn/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
int zMultiByteToWideChar_any(const char *in, int in_len, wchar_t *result_ptr, int result_size)
{
    int codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    wchar_t *result_buf = (wchar_t *)result_ptr;
    result_buf[0] = L'\0';
    int ret_len = MultiByteToWideChar(codepage, 0, in, in_len, NULL, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    ret_len = MultiByteToWideChar(codepage, 0, in, in_len, result_buf, result_size);
    if (ret_len < 1)
    {
        return -1;
    }
    if (in_len < 0)
    {
        result_buf[ret_len - 1] = L'\0';
        return ret_len - 1;
    }
    else
    {
        result_buf[ret_len] = L'\0';
        return ret_len;
    }
}

// 返回结果是字节数
// in_size 是输入(in)的字符数
// result_size 是结果(result_ptr)buffer的字节数
// https://learn.microsoft.com/zh-cn/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
int zWideCharToUTF8(const wchar_t *in, int in_size, char *result_ptr, int result_size)
{
    char *result_buf = (char *)result_ptr;
    result_buf[0] = 0;
    int ret_len = WideCharToMultiByte(CP_UTF8, 0, in, in_size, 0, 0, 0, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    ret_len = WideCharToMultiByte(CP_UTF8, 0, in, in_size, result_buf, ret_len, 0, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    if (in_size < 0)
    {
        result_buf[ret_len - 1] = 0;
        return ret_len - 1;
    }
    else
    {
        result_buf[ret_len] = 0;
        return ret_len;
    }
}

int zMultiByteToUTF8_any(const char *in, int in_len, char *result_ptr, int result_size)
{
    wchar_t *unicode_ptr = zmalloc(sizeof(wchar_t) * result_size + 16);
    int unicode_len = zMultiByteToWideChar_any(in, in_len, unicode_ptr, result_size);
    if (unicode_len < 1) {
        zfree(unicode_ptr);
        return -1;
    }
    int ret = zWideCharToUTF8(unicode_ptr, unicode_len, result_ptr, result_size);
    zfree(unicode_ptr);
    return ret;
}

ssize_t zgetdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
    if ((lineptr == 0) || (n == 0))
    {
        zerror("zgetdelim lineptr is NULL");
        return -1;
    }
    size_t nn = *n, count = 0;
    if (*lineptr == 0)
    {
        *n = 128;
        nn = *n;
        *lineptr = (char *)zmalloc(nn + 1);
    }
    while (1)
    {
        int ch = fgetc(stream);
        if (ch == EOF)
        {
            break;
        }
        if (count + 1 > nn)
        {
            nn *= 2;
            *lineptr = (char *)zrealloc(*lineptr, nn + 1);
        }
        (*lineptr)[count] = ch;
        count++;
        if (ch == delim)
        {
            break;
        }
        continue;
    }
    (*lineptr)[count] = 0;
    if (count == 0)
    {
        return -1;
    }
    return count;
}

void *zmemmem(const void *l, size_t l_len, const void *s, size_t s_len)
{
    register char *cur, *last;
    const char *cl = (const char *)l;
    const char *cs = (const char *)s;

    /* we need something to compare */
    if (l_len == 0 || s_len == 0)
        return NULL;

    /* "s" must be smaller or equal to "l" */
    if (l_len < s_len)
        return NULL;

    /* special case where s_len == 1 */
    if (s_len == 1)
        return memchr(l, (int)*cs, l_len);

    /* the last position where its possible to find "s" in "l" */
    last = (char *)cl + l_len - s_len;

    for (cur = (char *)cl; cur <= last; cur++)
        if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
            return cur;

    return NULL;
}

ssize_t ztimegm(void *void_tm)
{
    struct tm *tm = (struct tm *)void_tm;
    ssize_t t = mktime(tm);
    TIME_ZONE_INFORMATION tzi;
    GetTimeZoneInformation(&tzi);
    t -= tzi.Bias;
    return t;
}

int zclosesocket(int sock)
{
    return closesocket(sock);
}

#endif // _WIN32
