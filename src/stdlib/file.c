/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-04
 * ================================
 */

#include "zc.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


int zfile_get_size(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == -1) {
        return -1;
    }
    return st.st_size;
}

/* ################################################################## */
/* file get/put contents */
int zfile_put_contents(const char *filename, const void *data, int len)
{
    int fd;
    int ret;
    int wlen = 0;
    int errno2;

    while (((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1) && (errno == EINTR)) {
        continue;
    }

    if (fd == -1) {
        return -1;
    }

    while (len > wlen) {
        ret = write(fd, (const char *)data + wlen, len - wlen);
        if (ret > -1) {
            wlen += ret;
            continue;
        }
        errno2 = errno;
        if (errno == EINTR) {
            continue;
        }
        close(fd);
        errno = errno2;
        return -1;
    }

    close(fd);

    return 1;
}

int zfile_get_contents(const char *filename, zbuf_t *bf)
{
    int fd;
    int errno2;
    int ret;
    char buf[4096 + 1];
    int rlen = 0;

    while (((fd = open(filename, O_RDONLY)) == -1) && (errno == EINTR)) {
        continue;
    }
    if (fd == -1) {
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        return -1;
    }
    zbuf_reset(bf);
    zbuf_need_space(bf, st.st_size);

    while(1) {
        ret = read(fd, buf, 4096);
        if (ret < 0) {
            errno2 = errno;
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            errno = errno2;
            return -1;
        }
        if (ret == 0) {
            break;
        }
        zbuf_memcat(bf, buf, ret);
        rlen += ret;
    }
    close(fd);

    return rlen;
}

int zfile_get_contents_sample(const char *filename, zbuf_t *bf)
{
    int ret = zfile_get_contents(filename, bf);
    if (ret < 0) {
        zinfo("ERR load from %s (%m)", filename);
        exit(1);
    }
    return ret;
}

int zstdin_get_contents(zbuf_t *bf)
{
    int fd = 0;
    int errno2;
    int ret;
    char buf[4096 + 1];
    int rlen = 0;
    zbuf_reset(bf);

    while(1) {
        ret = read(fd, buf, 4096);
        if (ret < 0) {
            errno2 = errno;
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            errno = errno2;
            return -1;
        }
        if (ret == 0) {
            break;
        }
        zbuf_memcat(bf, buf, ret);
        rlen += ret;
    }

    return rlen;
}

/* ################################################################## */
int zmmap_reader_init(zmmap_reader_t * reader, const char *filename)
{
    int fd;
    int size;
    void *data;
    struct stat st;
    int errno2;

    reader->fd = -1;
    reader->data = 0;
    reader->len = 0;

    while (((fd = open(filename, O_RDONLY)) == -1) && (errno == EINTR)) {
        continue;
    }
    if (fd == -1) {
        return -1;
    }
    if (fstat(fd, &st) == -1) {
        errno2 = errno;
        close(fd);
        errno = errno2;
        return -1;
    }
    size = st.st_size;
    data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        errno2 = errno;
        close(fd);
        errno = errno2;
        return -1;
    }

    reader->fd = fd;
    reader->len = size;
    reader->data = (char *)data;

    return 1;
}

int zmmap_reader_fini(zmmap_reader_t * reader)
{
    munmap(reader->data, reader->len);
    close(reader->fd);

    return 0;
}
