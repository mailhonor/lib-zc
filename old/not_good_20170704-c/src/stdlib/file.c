/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-04
 * ================================
 */

#include "zc.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

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

    while ((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1 && errno == EINTR) {
        continue;
    }

    if (fd == -1) {
        return -1;
    }

    while (len > wlen) {
        ret = write(fd, (char *)data + wlen, len - wlen);
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
        return 0;
    }

    close(fd);

    return 0;
}


int zfile_get_contents(const char *filename, zbuf_t * bf)
{
    int fd;
    struct stat st;
    int errno2;
    int true_len, rlen;
    int ret;

    while ((fd = open(filename, O_RDONLY)) == -1 && errno == EINTR) {
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
    true_len = st.st_size;
    zbuf_need_space(bf, true_len);
    if (true_len > ZBUF_LEFT(bf)) {
        true_len = ZBUF_LEFT(bf);
    }
    char *ps = ZBUF_DATA(bf); 
    rlen = 0;
    while(rlen < true_len) {
        ret = read(fd, ps + rlen, true_len - rlen);
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
        rlen += ret;
    }
    close(fd);
    ZBUF_SET_LEN(bf, ZBUF_LEN(bf) + rlen);
    ZBUF_TERMINATE(bf);

    return rlen;
}

int zfile_get_contents_mmap(const char *filename, zbuf_t * bf)
{
    zmmap_reader_t reader;

    if (zmmap_reader_init(&reader, filename) < 0) {
        return -1;
    }
    zbuf_memcat(bf, reader.data, reader.len);
    zmmap_reader_fini(&reader);

    return reader.len;
}

void zfile_get_contents_sample(const char *filename, zbuf_t * dest)
{
    if (zfile_get_contents_mmap(filename, dest) < 0) {
        zfatal("file_get_contents from %s (%m)", filename);
    }
}

/* ################################################################## */
/* mmap ptr */
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

    while ((fd = open(filename, O_RDONLY)) == -1 && errno == EINTR) {
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

    return 0;
}

int zmmap_reader_fini(zmmap_reader_t * reader)
{
    munmap(reader->data, reader->len);
    close(reader->fd);

    return 0;
}
