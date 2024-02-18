/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-04
 * ================================
 */

#include "zc.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <stdarg.h>
#include <dirent.h>
#ifdef _WIN32
#include <winbase.h>
#include <wchar.h>
#else // _WIN32
#include <sys/mman.h>
#endif // _WIN32

#ifndef Z_MAX_PATH
#define Z_MAX_PATH 4096
#endif // Z_MAX_PATH

static int _zstat_utf8_or_multibyte(const char *pathname, void *statbuf, int utf8_or_multibyte)
{
#ifdef _WIN32
    if (utf8_or_multibyte)
    {
        wchar_t pathnamew[Z_MAX_PATH + 1];
        if (zUtf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
        {
            return 0;
        }
        return _wstat(pathnamew, statbuf);
    }
    else
    {
        return stat(pathname, statbuf);
    }
#else  // _WIN32
    return stat(pathname, statbuf);
#endif // _WIN32
}

int zstat(const char *pathname, void *statbuf)
{
    return _zstat_utf8_or_multibyte(pathname, statbuf, 1);
}

int zsys_stat(const char *pathname, void *statbuf)
{
    return _zstat_utf8_or_multibyte(pathname, statbuf, 0);
}

static FILE *_zfopen_utf8_or_multibyte(const char *pathname, const char *mode, int utf8_or_multibyte)
{
#ifdef _WIN32
    if (utf8_or_multibyte)
    {
        wchar_t pathnamew[Z_MAX_PATH + 1];
        wchar_t modew[64 + 1];
        int mlen = strlen(mode);
        if (mlen > 10)
        {
            return 0;
        }
        if (zUtf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
        {
            return 0;
        }
        if (zUtf8ToWideChar(mode, mlen, modew, 64) < 1)
        {
            return 0;
        }
        return _wfopen(pathnamew, modew);
    }
    else
    {
        return fopen(pathname, mode);
    }
#else  // _Win32
    return fopen(pathname, mode);
#endif // _Win32
}

FILE *zfopen(const char *pathname, const char *mode)
{
    return _zfopen_utf8_or_multibyte(pathname, mode, 1);
}

FILE *zsys_fopen(const char *pathname, const char *mode)
{
    return _zfopen_utf8_or_multibyte(pathname, mode, 0);
}

ssize_t zfile_get_size(const char *pathname)
{
    struct stat st;
    if (zstat(pathname, &st) == -1)
    {
        int ec = zget_errno();
        if (ec == ENOENT)
        {
            return 0;
        }
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
    }
    return st.st_size;
}

ssize_t zsys_file_get_size(const char *pathname)
{
    struct stat st;
    if (zsys_stat(pathname, &st) == -1)
    {
        int ec = zget_errno();
        if (ec == ENOENT)
        {
            return 0;
        }
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
    }
    return st.st_size;
}

int zfile_exists(const char *pathname)
{
    struct stat st;
    if (zstat(pathname, &st) == -1)
    {
        int ec = zget_errno();
        if (ec == ENOENT)
        {
            return 0;
        }
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
    }
    return 1;
}

int zsys_file_exists(const char *pathname)
{
    struct stat st;
    if (zsys_stat(pathname, &st) == -1)
    {
        int ec = zget_errno();
        if (ec == ENOENT)
        {
            return 0;
        }
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
    }
    return 1;
}

/* ################################################################## */
/* file get/put contents */
static int _zfile_put_contents_utf8_or_multibyte(const char *pathname, const void *data, int len, int utf8_or_multibyte)
{
#ifdef _WIN32
    int ret;
    FILE *fp;
    if (utf8_or_multibyte)
    {
        fp = zfopen(pathname, "wb+");
    }
    else
    {
        fp = fopen(pathname, "wb+");
    }
    if (!fp)
    {
        errno = zget_errno();
        return -1;
    }
    fwrite(data, 1, len, fp);
    ret = fflush(fp);
    fclose(fp);
    if (ret == EOF)
    {
        return -1;
    }
#else  // _Win32
    // 之所以不用FILE,而直接用open/write, 是因为open/read可以用在协程框架
    int ret;
    int fd;
    int wlen = 0;
    int errno2;

    while (((fd = open(pathname, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1) && (errno == EINTR))
    {
        continue;
    }

    if (fd == -1)
    {
        return -1;
    }

    while (len > wlen)
    {
        ret = write(fd, (const char *)data + wlen, len - wlen);
        if (ret > -1)
        {
            wlen += ret;
            continue;
        }
        errno2 = errno;
        if (errno == EINTR)
        {
            continue;
        }
        close(fd);
        errno = errno2;
        return -1;
    }

    close(fd);
#endif // _WIN32

    return 1;
}

int zfile_put_contents(const char *pathname, const void *data, int len)
{
    return _zfile_put_contents_utf8_or_multibyte(pathname, data, len, 1);
}

int zsys_file_put_contents(const char *pathname, const void *data, int len)
{
    return _zfile_put_contents_utf8_or_multibyte(pathname, data, len, 0);
}

static ssize_t _zfile_get_contents_utf8_or_multibyte(const char *pathname, zbuf_t *bf, int utf8_or_multibyte)
{
#ifdef _WIN32
    int ret, ch;
    ssize_t rlen = 0;
    FILE *fp = 0;
    if (utf8_or_multibyte)
    {
        fp = zfopen(pathname, "rb");
    }
    else
    {
        fp = fopen(pathname, "rb");
    }
    if (!fp)
    {
        errno = zget_errno();
        return -1;
    }
    int k = 0;
    while (1)
    {
        k++;
        ch = fgetc(fp);
        if (ch == EOF)
        {
            break;
        }
        ZBUF_PUT(bf, ch);
    }
    fclose(fp);
    zbuf_terminate(bf);
    return zbuf_len(bf);
#else  // _WIN32
    int fd;
    int errno2;
    int ret;
    char buf[4096 + 1];
    ssize_t rlen = 0;

    while (((fd = open(pathname, O_RDONLY)) == -1) && (errno == EINTR))
    {
        continue;
    }
    if (fd == -1)
    {
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        close(fd);
        return -1;
    }
    zbuf_reset(bf);
    zbuf_need_space(bf, st.st_size);

    while (1)
    {
        ret = read(fd, buf, 4096);
        if (ret < 0)
        {
            errno2 = errno;
            if (errno == EINTR)
            {
                continue;
            }
            close(fd);
            errno = errno2;
            return -1;
        }
        if (ret == 0)
        {
            break;
        }
        zbuf_memcat(bf, buf, ret);
        rlen += ret;
    }
    close(fd);

    return rlen;
#endif // _WIN32
}

ssize_t zfile_get_contents(const char *pathname, zbuf_t *bf)
{
    return _zfile_get_contents_utf8_or_multibyte(pathname, bf, 1);
}

ssize_t zsys_file_get_contents(const char *pathname, zbuf_t *bf)
{
    return _zfile_get_contents_utf8_or_multibyte(pathname, bf, 0);
}

int zfile_get_contents_sample(const char *pathname, zbuf_t *bf)
{
    int ret = zfile_get_contents(pathname, bf);
    if (ret < 0)
    {
        zinfo("ERROR load from %s", pathname);
        exit(1);
    }
    return ret;
}

int zsys_file_get_contents_sample(const char *pathname, zbuf_t *bf)
{
    int ret = zsys_file_get_contents(pathname, bf);
    if (ret < 0)
    {
        zinfo("ERROR load from %s", pathname);
        exit(1);
    }
    return ret;
}

int zstdin_get_contents(zbuf_t *bf)
{
#if (defined _WIN32) || (defined __APPLE__)
    zbuf_reset(bf);
    while (1)
    {
        int ch = fgetc(stdin);
        if (ch == EOF)
        {
            break;
        }
        ZBUF_PUT(bf, ch);
    }
    zbuf_terminate(bf);
    return zbuf_len(bf);
#else  // _WIN32
    int fd = 0;
    int errno2;
    int ret;
    char buf[4096 + 1];
    int rlen = 0;
    zbuf_reset(bf);

    while (1)
    {
        ret = read(fd, buf, 4096);
        if (ret < 0)
        {
            errno2 = errno;
            if (errno == EINTR)
            {
                continue;
            }
            close(fd);
            errno = errno2;
            return -1;
        }
        if (ret == 0)
        {
            break;
        }
        zbuf_memcat(bf, buf, ret);
        rlen += ret;
    }

    return rlen;
#endif // _WIN32
}

/* ################################################################## */
static int _zmmap_reader_init_utf8_or_multibyte(zmmap_reader_t *reader, const char *pathname, int utf8_or_multibyte)
{
#ifdef _WIN32
    HANDLE fd, fm;
    ssize_t size;
    void *data;
    struct stat st;
    int errno2;

    reader->fd = INVALID_HANDLE_VALUE;
    reader->data = 0;
    reader->len = 0;

    if (utf8_or_multibyte)
    {
        wchar_t pathnamew[Z_MAX_PATH + 1];
        if (zUtf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
        {
            return -1;
        }
        fd = CreateFileW(pathnamew,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    }
    else
    {
        fd = CreateFile(pathname,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    }
    if (fd == INVALID_HANDLE_VALUE)
    {
        errno = zget_errno();
        return -1;
    }

    LARGE_INTEGER info;
    memset(&info, 0, sizeof(info));
    GetFileSizeEx(fd, &info);
    size = info.QuadPart;
    if (size < 0)
    {
        errno = zget_errno();
        CloseHandle(fd);
        return -1;
    }

    fm = CreateFileMapping(
        fd,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL);

    if (NULL == INVALID_HANDLE_VALUE)
    {
        errno = zget_errno();
        CloseHandle(fd);
        return -1;
    }

    data = MapViewOfFile(
        fm,
        FILE_MAP_READ,
        0,
        0,
        0);

    if (NULL == data)
    {
        CloseHandle(fm);
        CloseHandle(fd);
        return -1;
    }
    reader->fd = fd;
    reader->fm = fm;
    reader->data = data;
    reader->len = size;
    return 1;
#else  // _WIN32
    int fd;
    ssize_t size;
    void *data;
    struct stat st;
    int errno2;

    reader->fd = -1;
    reader->data = 0;
    reader->len = 0;

    while (((fd = open(pathname, O_RDONLY)) == -1) && (errno == EINTR))
    {
        continue;
    }
    if (fd == -1)
    {
        return -1;
    }
    if (fstat(fd, &st) == -1)
    {
        errno2 = errno;
        close(fd);
        errno = errno2;
        return -1;
    }
    size = st.st_size;
    data = mmap(NULL, size + 1, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        errno2 = errno;
        close(fd);
        errno = errno2;
        return -1;
    }

    reader->fd = fd;
    reader->len = size;
    reader->data = (char *)data;

    return 1;
#endif // _WIN32
    return 1;
}

int zmmap_reader_init(zmmap_reader_t *reader, const char *pathname)
{
    return _zmmap_reader_init_utf8_or_multibyte(reader, pathname, 1);
}

int zsys_mmap_reader_init(zmmap_reader_t *reader, const char *pathname)
{
    return _zmmap_reader_init_utf8_or_multibyte(reader, pathname, 0);
}

int zmmap_reader_fini(zmmap_reader_t *reader)
{
#ifdef _WIN32
    UnmapViewOfFile(reader->data);
    CloseHandle(reader->fm);
    CloseHandle(reader->fd);
    return 1;
#else  // _WIN32
    munmap(reader->data, reader->len + 1);
    close(reader->fd);
    return 1;
#endif // _WIN32
}

static int _ztouch_utf8_or_multibyte(const char *pathname, int utf8_or_multibyte)
{
#ifdef _WIN32
    if (utime(pathname, 0) == 0)
    {
        return 1;
    }
    int fd = 0;
    if (utf8_or_multibyte)
    {
        fd = zopen(pathname, O_RDWR | O_CREAT, 0666);
    }
    else
    {
        fd = open(pathname, O_RDWR | O_CREAT, 0666);
    }
    if (fd < 0)
    {
        return -1;
    }
    close(fd);
    return 1;
#else  // _WIN32
    int fd = open(pathname, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
    {
        return -1;
    }
    if (futimens(fd, 0) < 0)
    {
        close(fd);
        return -1;
    }
    close(fd);
    return 1;
#endif // _WIN32
}

int ztouch(const char *pathname)
{
    return _ztouch_utf8_or_multibyte(pathname, 1);
}

int zsys_touch(const char *pathname)
{
    return _ztouch_utf8_or_multibyte(pathname, 0);
}

zargv_t *zfind_file_sample(zargv_t *file_argv, const char **pathnames, int pathnames_count, const char *pathname_match)
{
#ifdef _WIN32
    return file_argv;
#else  // _WIN32
    char buf[4096 + 1];
    if (file_argv == 0)
    {
        file_argv = zargv_create(-1);
    }
    for (int i = 0; i < pathnames_count; i++)
    {
        const char *pathname = pathnames[i];
        struct stat st;
        if (stat(pathname, &st) == -1)
        {
            if (errno == ENOENT)
            {
                continue;
            }
            zdebug_show("ERROR open %s(%m)", pathname);
            exit(1);
        }
        if (S_ISREG(st.st_mode))
        {
            zargv_add(file_argv, pathname);
            continue;
        }
        else if (!S_ISDIR(st.st_mode))
        {
            zdebug_show("WARNING file must be regular file or directory %s", pathname);
            continue;
        }
        if (zempty(pathname_match))
        {
            zsprintf(buf, "find \"%s\" -type f", pathname);
        }
        else
        {
            zsprintf(buf, "find \"%s\" -type f -name \"%s\"", pathname, pathname_match);
        }
        FILE *fp = popen(buf, "r");
        if (!fp)
        {
            zdebug_show("ERROR popen: find \"%s\" -type f", pathname);
        }
        while (fgets(buf, 4096, fp))
        {
            char *p = strchr(buf, '\n');
            if (p)
            {
                *p = 0;
            }
            p = strchr(buf, '\r');
            if (p)
            {
                *p = 0;
            }
            zargv_add(file_argv, buf);
        }
        fclose(fp);
    }
    return file_argv;
#endif // _WIN32
}

#ifdef _WIN32
int _zmkdir(const char *pathname)
{
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (zUtf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    return _wmkdir(pathnamew);
}
#endif // _WIN32

static int _v_zmkdirs_utf8_or_multibyte(int utf8_or_multibyte, int mode, const char *path1, va_list ap)
{
    int r = -1, ret;
    struct stat st;
    zbuf_t *tmppath = 0;
    char *path;
    unsigned char *ps, *p;
    int saved_ch;

    tmppath = zbuf_create(1024);
    zbuf_strcpy(tmppath, path1);
    while ((path = (char *)va_arg(ap, char *)))
    {
        if (zbuf_data(tmppath)[zbuf_len(tmppath) - 1] != zvar_path_splitor)
        {
            zbuf_put(tmppath, zvar_path_splitor);
        }
        if (zempty(path))
        {
            break;
        }
        if (path[0] == '/')
        {
            path++;
        }
#ifdef _WIN32
        if (path[0] == '\\')
        {
            path++;
        }
#endif // _WIN32
        zbuf_strcat(tmppath, path);
    }

#ifdef _WIN32
    for (unsigned char *p = (unsigned char *)zbuf_data(tmppath); *p; p++)
    {
        if (*p == '/')
        {
            *p = '\\';
        }
    }
#endif // _WIN32

    path = (char *)zbuf_data(tmppath);
    ps = (unsigned char *)path;
    saved_ch = -1;
    for (; ps;)
    {
        if (saved_ch > -1)
        {
            ps[0] = saved_ch;
        }
        p = (unsigned char *)strchr((char *)ps, zvar_path_splitor);
        if (p)
        {
            ps = p + 1;
            saved_ch = *ps;
            *ps = 0;
        }
        else
        {
            ps = 0;
        }
        if (utf8_or_multibyte)
        {
            ret = zstat(path, &st);
        }
        else
        {
            ret = zsys_stat(path, &st);
        }
        if (ret < 0)
        {
            if (errno == ENOTDIR)
            {
                goto over;
            }
        }
        else
        {
            if (!S_ISDIR(st.st_mode))
            {
                errno = ENOTDIR;
                goto over;
            }
            continue;
        }

        ret = -1;
#ifdef _WIN32
        if (utf8_or_multibyte)
        {
            ret = _zmkdir(path);
        }
        else
        {
            ret = mkdir(path);
        }
#else  // _WIN32
        ret = mkdir(path, mode);
#endif // _WIN32
        if (ret < 0)
        {
            if (errno != EEXIST)
            {
                goto over;
            }
            continue;
        }
    }
    r = 1;
over:
    zbuf_free(tmppath);
    return r;
}

int zmkdirs(int mode, const char *path1, ...)
{
    int ret;
    va_list ap;
    va_start(ap, path1);
    ret = _v_zmkdirs_utf8_or_multibyte(1, mode, path1, ap);
    va_end(ap);
    return ret;
}

int zmkdir(const char *path, int mode)
{
    return zmkdirs(mode, path, 0);
}

int zsys_mkdirs(int mode, const char *path1, ...)
{
    int ret;
    va_list ap;
    va_start(ap, path1);
    ret = _v_zmkdirs_utf8_or_multibyte(0, mode, path1, ap);
    va_end(ap);
    return ret;
}

int zsys_mkdir(const char *path, int mode)
{
    return zsys_mkdirs(mode, path, 0);
}

int zrename(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zUtf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (zUtf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        errno = zget_errno();
        return -1;
    }
    if (!MoveFileExW(oldpathw, newpathw, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        errno = zget_errno();
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(rename(oldpath, newpath));
#endif // _WIN32
}

int zsys_rename(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zMultiByteToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (zMultiByteToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        errno = zget_errno();
        return -1;
    }
    if (!MoveFileExW(oldpathw, newpathw, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        errno = zget_errno();
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(rename(oldpath, newpath));
#endif // _WIN32
}

int zunlink(const char *pathname)
{
#ifdef _WIN32
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (zUtf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return -1;
    }
    if (!DeleteFileW(pathnamew))
    {
        int ec = zget_errno();
        if (ec == ENOENT)
        {
            return 0;
        }
        errno = ec;
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(unlink(pathname));
#endif // _WIN32
}

int zsys_unlink(const char *pathname)
{
#ifdef _WIN32
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (zMultiByteToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return -1;
    }
    if (!DeleteFileW(pathnamew))
    {
        int ec = zget_errno();
        if (ec == ENOENT)
        {
            return 0;
        }
        errno = ec;
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(unlink(pathname));
#endif // _WIN32
}

int zlink(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zUtf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (zUtf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        errno = zget_errno();
        return -1;
    }
    if (!CreateHardLinkW(newpathw, oldpathw, NULL))
    {
        errno = zget_errno();
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(link(oldpath, newpath));
#endif // _WIN32
}

int zsys_link(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zMultiByteToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (zMultiByteToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        errno = zget_errno();
        return -1;
    }
    if (!CreateHardLinkW(newpathw, oldpathw, NULL))
    {
        errno = zget_errno();
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(link(oldpath, newpath));
#endif // _WIN32
}

int zlink_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    int ret = zlink(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    if (errno != EEXIST)
    {
        return -1;
    }

    char tmppath[Z_MAX_PATH + 1];
    char unique_id[zvar_unique_id_size + 1];
    zbuild_unique_id(unique_id);
    zsnprintf(tmppath, Z_MAX_PATH, "%s/%s", tmpdir, unique_id);
    ret = zlink(oldpath, tmppath);
    if (ret < 0)
    {
        return -1;
    }
    ret = zrename(tmppath, newpath);
    zunlink(tmppath);
    if (ret < 0)
    {
        return -1;
    }
    return 0;
}

int zsys_link_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    int ret = zsys_link(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    if (errno != EEXIST)
    {
        return -1;
    }

    char tmppath[Z_MAX_PATH + 1];
    char unique_id[zvar_unique_id_size + 1];
    zbuild_unique_id(unique_id);
    zsnprintf(tmppath, Z_MAX_PATH, "%s/%s", tmpdir, unique_id);
    ret = zsys_link(oldpath, tmppath);
    if (ret < 0)
    {
        return -1;
    }
    ret = zsys_rename(tmppath, newpath);
    zsys_unlink(tmppath);
    if (ret < 0)
    {
        return -1;
    }
    return 0;
}

int zsymlink(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
    DWORD attr;
    BOOLEAN res;
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zUtf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (zUtf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    if ((attr = GetFileAttributesW(oldpathw)) == INVALID_FILE_ATTRIBUTES)
    {
        errno = zget_errno();
        return -1;
    }
    WINBASEAPI BOOLEAN APIENTRY CreateSymbolicLinkW(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags);
    res = CreateSymbolicLinkW(oldpathw, oldpathw, (attr & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0));
    if (!res)
    {
        errno = zget_errno();
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(symlink(oldpath, newpath));
#endif // _WIN32
}

int zsys_symlink(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
    DWORD attr;
    BOOLEAN res;
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zMultiByteToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (zMultiByteToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    if ((attr = GetFileAttributesW(oldpathw)) == INVALID_FILE_ATTRIBUTES)
    {
        errno = zget_errno();
        return -1;
    }
    WINBASEAPI BOOLEAN APIENTRY CreateSymbolicLinkW(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags);
    res = CreateSymbolicLinkW(oldpathw, oldpathw, (attr & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0));
    if (!res)
    {
        errno = zget_errno();
        return -1;
    }
    return 0;
#else  // _WIN32
    ZC_ROBUST_DO(symlink(oldpath, newpath));
#endif // _WIN32
}

int zsymlink_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    int ret = zsymlink(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    if (errno != EEXIST)
    {
        return -1;
    }

    char tmppath[Z_MAX_PATH + 1];
    char unique_id[zvar_unique_id_size + 1];
    zbuild_unique_id(unique_id);
    zsnprintf(tmppath, Z_MAX_PATH, "%s/%s", tmpdir, unique_id);
    ret = zsymlink(oldpath, tmppath);
    if (ret < 0)
    {
        return -1;
    }
    ret = zrename(tmppath, newpath);
    if (ret < 0)
    {
        zunlink(tmppath);
        return -1;
    }
    return 0;
}

int zsys_symlink_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    int ret = zsys_symlink(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    if (errno != EEXIST)
    {
        return -1;
    }

    char tmppath[Z_MAX_PATH + 1];
    char unique_id[zvar_unique_id_size + 1];
    zbuild_unique_id(unique_id);
    zsnprintf(tmppath, Z_MAX_PATH, "%s/%s", tmpdir, unique_id);
    ret = zsys_symlink(oldpath, tmppath);
    if (ret < 0)
    {
        return -1;
    }
    ret = zsys_rename(tmppath, newpath);
    if (ret < 0)
    {
        zsys_unlink(tmppath);
        return -1;
    }
    return 0;
}

static int _zget_filenames_in_dir_default(const char *dirname, zargv_t *filenames)
{
    DIR *dir;
    struct dirent *ent_list;

    if (!(dir = opendir(dirname)))
    {
        if (errno == ENOENT)
        {
            return 0;
        }
        zerror("访问文件夹失败:%s(%m)", dirname);
        return -1;
    }

    // modern linux, readdir is thread-safe
    while ((ent_list = readdir(dir)))
    {
        const char *fn = ent_list->d_name;
        if ((!strcmp(fn, ".")) || (!strcmp(fn, "..")))
        {
            continue;
        }
        zargv_add(filenames, fn);
    }
    closedir(dir);
    return 1;
}

static int _zget_filenames_in_dir_utf8_or_multibyte(const char *dirname, zargv_t *filenames, int utf8_or_multibyte)
{
#ifdef _WIN32
    if (utf8_or_multibyte)
    {
        _WDIR *dir;
        struct _wdirent *ent_list;

        wchar_t pathnamew[4096 + 1];
        if (zUtf8ToWideChar(dirname, -1, pathnamew, 4096) < 1)
        {
            return -1;
        }

        if (!(dir = _wopendir(pathnamew)))
        {
            if (errno == ENOENT)
            {
                return 0;
            }
            zerror("访问文件夹失败:%s(%m)", dirname);
            return -1;
        }

        while ((ent_list = _wreaddir(dir)))
        {
            char fn[4096 + 1];
            int ret = zWideCharToUTF8(ent_list->d_name, -1, fn, 4096);
            if (ret < 1)
            {
                continue;
            }
            if ((!strcmp(fn, ".")) || (!strcmp(fn, "..")))
            {
                continue;
            }
            zargv_add(filenames, fn);
        }
        _wclosedir(dir);
        return 1;
    }
    else
    {
        return _zget_filenames_in_dir_default(dirname, filenames);
    }
#else  // _WIN32
    return _zget_filenames_in_dir_default(dirname, filenames);
#endif // _WIN32
}

int zget_filenames_in_dir(const char *dirname, zargv_t *filenames)
{
    return _zget_filenames_in_dir_utf8_or_multibyte(dirname, filenames, 1);
}

int zsys_get_filenames_in_dir(const char *dirname, zargv_t *filenames)
{
    return _zget_filenames_in_dir_utf8_or_multibyte(dirname, filenames, 0);
}

#ifdef _WIN32
static int _zopen(const char *pathname, int flags, mode_t mode)
{
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (zUtf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    return _wopen(pathnamew, flags, mode);
}
#endif // _WIN32

int zopen(const char *pathname, int flags, mode_t mode)
{
#ifdef _WIN32
    ZC_ROBUST_DO_WIN32(_zopen(pathname, flags, mode));
#else  // _WIN32
    ZC_ROBUST_DO(open(pathname, flags, mode));
#endif // _WIN32
}

int zsys_open(const char *pathname, int flags, mode_t mode)
{
#ifdef _WIN32
    ZC_ROBUST_DO_WIN32(open(pathname, flags, mode));
#else  // _WIN32
    ZC_ROBUST_DO(open(pathname, flags, mode));
#endif // _WIN32
}

ssize_t zread(int fd, void *buf, size_t count)
{
    ssize_t ret;
    int ec;
    for (;;)
    {
        if ((ret = read(fd, buf, count)) < 0)
        {
            ec = zget_errno();
            if (ec == EINTR)
            {
                continue;
            }
            errno = ec;
            return -1;
        }
        return ret;
    }
    return -1;
}

ssize_t zwrite(int fd, const void *buf, size_t count)
{
    ssize_t ret;
    int ec, is_closed = 0;
    const char *ptr = (const char *)buf;
    ssize_t left = count;
    for (; left > 0;)
    {
        ret = write(fd, ptr, left);
        if (ret < 0)
        {
            ec = zget_errno();
            if (ec == EINTR)
            {
                continue;
            }
            errno = ec;
            if (ec == EPIPE)
            {
                is_closed = 1;
                break;
            }
            break;
        }
        else if (ret == 0)
        {
            continue;
        }
        else
        {
            left -= ret;
            ptr += ret;
        }
    }
    if (count > left)
    {
        return count - left;
    }
    if (is_closed)
    {
        return 0;
    }
    return -1;
}

int zclose(int fd)
{
#ifdef _WIN32
    ZC_ROBUST_DO_WIN32(close(fd));
#else  // _WIN32
    ZC_ROBUST_DO(close(fd));
#endif // _WIN32
}

#ifdef _WIN32
// https://learn.microsoft.com/zh-cn/windows/win32/api/fileapi/nf-fileapi-lockfile
#else  // _WIN32
int zflock(int fd, int operation)
{
    ZC_ROBUST_DO(flock(fd, operation));
}

int zflock_share(int fd)
{
    ZC_ROBUST_DO(flock(fd, LOCK_SH));
}

int zflock_exclusive(int fd)
{
    ZC_ROBUST_DO(flock(fd, LOCK_EX));
}

int zfunlock(int fd)
{
    ZC_ROBUST_DO(flock(fd, LOCK_UN));
}
#endif // _WIN32
