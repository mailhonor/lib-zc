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
#ifdef __linux__
#include <sys/mman.h>
#endif // __linux__
#ifdef _WIN32
#include <winsock2.h>
#include <winbase.h>
#endif // _WIN32

#ifndef Z_MAX_PATH
#define Z_MAX_PATH 1024
#endif // Z_MAX_PATH

ssize_t zfile_get_size(const char *pathname)
{
    struct stat st;
    if (stat(pathname, &st) == -1)
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
    if (stat(pathname, &st) == -1)
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
int zfile_put_contents(const char *pathname, const void *data, int len)
{
#ifdef __linux__
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
#endif // __linux__

#ifdef _WIN32
    int ret;
    FILE *fp;
    if (!(fp = fopen(pathname, "wb+")))
    {
#ifdef _WIN32
        errno = zget_errno();
#endif // _WIN32
        return -1;
    }
    fwrite(data, 1, len, fp);
    ret = fflush(fp);
    fclose(fp);
    if (ret == EOF)
    {
        return -1;
    }
#endif // _WIN32

    return 1;
}

ssize_t zfile_get_contents(const char *pathname, zbuf_t *bf)
{
#ifdef __linux__
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
#endif // __linux__
#ifdef _WIN32
    int ret, ch;
    ssize_t rlen = 0;
    ssize_t fsize = zfile_get_size(pathname);
    if (fsize < 0)
    {
        return -1;
    }
    if (fsize > 256LL * 256 * 256 * 126)
    {
        zfatal("zfile_get_contents size too big: :%zd", fsize);
    }
    zbuf_reset(bf);
    zbuf_need_space(bf, (int)fsize);
    FILE *fp = fopen(pathname, "rb");
    if (!fp)
    {
#ifdef _WIN32
        errno = zget_errno();
#endif // _WIN32
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
#endif // _WIN32
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

int zstdin_get_contents(zbuf_t *bf)
{
#ifdef __linux__
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
#endif // __linux__

#ifdef _WIN32
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
#endif // _WIN32
}

/* ################################################################## */
int zmmap_reader_init(zmmap_reader_t *reader, const char *pathname)
{
#ifdef __linux__
    int fd;
    int size;
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
#endif // __linux__

#ifdef _WIN32
    reader->file_buf = 0;
    reader->data = 0;
    reader->len = 0;

    zbuf_t *buf = zbuf_create(-1);
    if (zfile_get_contents(pathname, buf) < 0)
    {
        zbuf_free(buf);
        return -1;
    }

    reader->file_buf = buf;
    reader->len = zbuf_len(buf);
    reader->data = zbuf_data(buf);

    return 1;
#endif // _WIN32
}

int zmmap_reader_fini(zmmap_reader_t *reader)
{
#ifdef __linux__
    munmap(reader->data, reader->len + 1);
    close(reader->fd);
    return 1;
#endif // __linux__

#ifdef _WIN32
    if (reader->file_buf)
    {
        zbuf_free(reader->file_buf);
    }
    reader->file_buf = 0;
    reader->data = 0;
    reader->len = 0;
    return 1;
#endif // _WIN32
}

int ztouch(const char *pathname)
{
#ifdef __linux__
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
#endif // __linux__
#ifdef _WIN32
    if (utime(pathname, 0) == 0)
    {
        return 1;
    }
    int fd = open(pathname, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
    {
        return -1;
    }
    close(fd);
    return 1;
#endif // _WIN32
}

zargv_t *zfind_file_sample(zargv_t *file_argv, const char **pathnames, int pathnames_count, const char *pathname_match)
{
#ifdef __linux__
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
#endif // __linux__

#ifdef _WIN32
    return file_argv;
#endif // _WIN32
}

int zmkdirs(int perms, const char *path1, ...)
{
    int r = -1, ret;
    struct stat st;
    va_list ap;
    zbuf_t *tmppath = 0;
    char *path, *ps, *p, saved_ch;

    if (zempty(path1))
    {
        r = 0;
        goto over;
    }
    tmppath = zbuf_create(1024);
    zbuf_strcpy(tmppath, path1);
    va_start(ap, path1);
    while ((path = (char *)va_arg(ap, char *)))
    {
        if (zbuf_data(tmppath)[zbuf_len(tmppath) - 1] != '/')
        {
            zbuf_strcat(tmppath, "/");
        }
        if (zempty(path))
        {
            break;
        }
        if (path[0] == '/')
        {
            path++;
        }
        zbuf_strcat(tmppath, path);
    }
    va_end(ap);

    path = zbuf_data(tmppath);
    ps = path;
    saved_ch = -1;
    for (; ps;)
    {
        if (saved_ch > -1)
        {
            ps[0] = saved_ch;
        }
        p = strchr(ps, '/');
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

        if ((ret = stat(path, &st)) < 0)
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
#ifdef __linux__
        ret = mkdir(path, perms);
#endif // __linux__
#ifdef _WIN32
        ret = mkdir(path);
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

int zmkdir(const char *path, int perms)
{
    return zmkdirs(perms, path, 0);
}

int zrename(const char *oldpath, const char *newpath)
{
#ifdef __linux__
    ZC_ROBUST_DO(rename(oldpath, newpath));
#endif // __linux__
#ifdef _WIN32
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zMultiByteToWideChar_any(oldpath, -1, oldpathw) < 1) || (zMultiByteToWideChar_any(newpath, -1, newpathw) < 1))
    {
        return -1;
    }
    if (!MoveFileExW(oldpathw, newpathw, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        return -1;
    }
    return 0;
#endif // _WIN32
}

int zunlink(const char *pathname)
{
#ifdef __linux__
    ZC_ROBUST_DO(unlink(pathname));
#endif // __linux__
#ifdef _WIN32
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (zMultiByteToWideChar_any(pathname, -1, pathnamew) < 1)
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
#endif // _WIN32
}

int zlink(const char *oldpath, const char *newpath)
{
#ifdef __linux__
    ZC_ROBUST_DO(link(oldpath, newpath));
#endif // __linux__

#ifdef _WIN32
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zMultiByteToWideChar_any(oldpath, -1, oldpathw) < 1) || (zMultiByteToWideChar_any(newpath, -1, newpathw) < 1))
    {
        return -1;
    }
    if (!CreateHardLinkW(newpathw, oldpathw, NULL))
    {
        return -1;
    }
    return 0;
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
    if (ret < 0)
    {
        zunlink(tmppath);
        return -1;
    }
    return 0;
}

int zsymlink(const char *oldpath, const char *newpath)
{
#ifdef __linux__
    ZC_ROBUST_DO(symlink(oldpath, newpath));
#endif // __linux__

#ifdef _WIN32
    DWORD attr;
    BOOLEAN res;
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((zMultiByteToWideChar_any(oldpath, -1, oldpathw) < 1) || (zMultiByteToWideChar_any(newpath, -1, newpathw) < 1))
    {
        return -1;
    }
    if ((attr = GetFileAttributesW(oldpathw)) == INVALID_FILE_ATTRIBUTES)
    {
        return -1;
    }
    WINBASEAPI BOOLEAN APIENTRY CreateSymbolicLinkW (LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags);
    res = CreateSymbolicLinkW(oldpathw, oldpathw, (attr & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0));
    if (!res)
    {
        return -1;
    }
    return 0;
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

int zopen(const char *pathname, int flags, mode_t mode)
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
