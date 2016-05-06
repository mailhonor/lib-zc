/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-04
 * ================================
 */

#include "libzc.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

int zfile_get_size(char *filename)
{
    struct stat st;

    if (stat(filename, &st) == -1) {
        return -1;
    }
    return st.st_size;
}

/* ################################################################## */
/* file get/put contents */
int zfile_put_contents(char *filename, void *data, int len)
{
    int fd;
    int ret;
    int wlen = 0;
    int errno2;

    while ((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1 && errno == EINTR) {
        zmsleep(1);
        continue;
    }

    if (fd == -1) {
        return -1;
    }

    while (len > wlen) {
        ret = write(fd, data + wlen, len - wlen);
        if (ret > -1) {
            wlen += ret;
            continue;
        }
        errno2 = errno;
        if (errno == EINTR) {
            zmsleep(1);
            continue;
        }
        close(fd);
        errno = errno2;
        return -1;
    }

    close(fd);

    return 0;
}

int zfile_get_contents(char *filename, void *data, int len)
{
    zmmap_reader reader;

    if (zmmap_reader_init(&reader, filename) < 0) {
        return -1;
    }
    if (len > reader.len) {
        len = reader.len;
    }
    memcpy(data, reader.data, len);
    zmmap_reader_fini(&reader);

    return len;
}

int zfile_get_contents_to_zbuf(char *filename, zbuf_t * bf)
{
    zmmap_reader reader;

    if (zmmap_reader_init(&reader, filename) < 0) {
        return -1;
    }
    zbuf_memcat(bf, reader.data, reader.len);
    zmmap_reader_fini(&reader);

    return reader.len;
}

/* ################################################################## */
/* mmap ptr */
int zmmap_reader_init(zmmap_reader * reader, char *filename)
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
        zmsleep(1);
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
    reader->data = data;

    return 0;
}

int zmmap_reader_fini(zmmap_reader * reader)
{
    munmap(reader->data, reader->len);
    close(reader->fd);

    return 0;
}
