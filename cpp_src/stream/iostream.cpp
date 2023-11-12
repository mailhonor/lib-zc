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

iostream &iostream::open_fd(int fd)
{
    close(true);
    fd_ = fd;
    return *this;
}

bool iostream::open_destination(const char *destination, int timeout)
{
    close(true);
    int fd = zconnect(destination, timeout);
    if (fd < 0) {
        return false;
    }
    znonblocking(fd, 1);
    open_fd(fd);
    return true;
}

iostream::iostream()
{
    fd_ = -1;
    read_wait_timeout_ = -1;
    write_wait_timeout_ = -1;
    close(true);
}

iostream::~iostream()
{
    close(true);
}

int iostream::close(bool close_fd_or_release_ssl)
{
    int ret = 0, fd = get_fd();
    if (close_fd_or_release_ssl && (fd > -1)) {
        ret = zclosesocket(fd);
    }
    fd_ = -1;
    read_wait_timeout_ = -1;
    write_wait_timeout_ = -1;
    return ret;
}

bool iostream::is_closed()
{
    return (fd_ < 0);
}

iostream &iostream::set_read_wait_timeout(int read_wait_timeout)
{
    read_wait_timeout_ = read_wait_timeout;
    return *this;
}

iostream &iostream::set_write_wait_timeout(int write_wait_timeout)
{
    write_wait_timeout_ = write_wait_timeout;
    return *this;
}

int iostream::timed_read_wait(int read_wait_timeout)
{
    return ztimed_read_wait(fd_, read_wait_timeout_);
}

int iostream::timed_write_wait(int write_wait_timeout)
{
    return ztimed_write_wait(fd_, write_wait_timeout_);
}

int iostream::engine_read(void *buf, int len)
{
    return ztimed_read(fd_, buf, len, read_wait_timeout_);
}

int iostream::engine_write(const void *buf, int len)
{
    return ztimed_write(fd_, buf, len, write_wait_timeout_);
}

} /* namespace zcc */

