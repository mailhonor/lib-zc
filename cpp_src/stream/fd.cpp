/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-15
 * ================================
 */

#include "zc.h"

namespace zcc
{

static int _close(iostream &fp, bool release)
{
    int ret = 0, fd = fp.get_fd();
    if (release && (fd > -1)) {
        ret = zclose(fd);
    }
    return ret;
}

static int _read(iostream &fp, void *buf, int len)
{
    return ztimed_read(fp.get_fd(), buf, len, fp.get_read_wait_timeout());
}

static int _write(iostream &fp, const void *buf, int len)
{
    return ztimed_write(fp.get_fd(), buf, len, fp.get_write_wait_timeout());
}

static int _timed_read_wait(iostream &fp, int read_wait_timeout)
{
    return ztimed_read_wait(fp.get_fd(), fp.get_read_wait_timeout());
}

static int _timed_write_wait(iostream &fp, int write_wait_timeout)
{
    return ztimed_write_wait(fp.get_fd(), fp.get_write_wait_timeout());
}

iostream &iostream::open_fd(int fd)
{
    close(1);
    fd_ = fd;
    engine_read_ = _read;
    engine_write_ = _write;
    engine_timed_read_wait_ = _timed_read_wait;
    engine_timed_write_wait_ = _timed_write_wait;
    engine_close_ = _close;
    return *this;
}

bool iostream::open_destination(const char *destination, int timeout)
{
    close(1);
    int fd = zconnect(destination, timeout);
    if (fd < 0) {
        return false;
    }
    znonblocking(fd, 1);
    open_fd(fd);
    return true;
}

} /* namespace zcc */

