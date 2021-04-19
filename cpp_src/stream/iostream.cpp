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

static int _read(iostream &fp, void *buf, int len)
{
    return -1;
}

static int _write(iostream &fp, const void *buf, int len)
{
    return -1;
}

static int _timed_read_wait(iostream &fp, int read_wait_timeout)
{
    return -1;
}

static int _timed_write_wait(iostream &fp, int write_wait_timeout)
{
    return -1;
}

static int _close(iostream &fp, bool close_fd_or_release_ssl)
{
    return -1;
}

iostream::iostream()
{
    engine_close_ = _close;
    close(1);
}

iostream::~iostream()
{
    close(1);
}

int iostream::close(bool close_fd_or_release_ssl)
{
    int ret = engine_close_(*this, close_fd_or_release_ssl);
    ssl_ = 0;
    fd_ = -1;
    read_wait_timeout_ = -1;
    write_wait_timeout_ = -1;
    engine_read_ = _read;
    engine_write_ = _write;
    engine_timed_read_wait_ = _timed_read_wait;
    engine_timed_write_wait_ = _timed_write_wait;
    engine_close_ = _close;
    return ret;
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
    return engine_timed_read_wait_(*this, read_wait_timeout);
}

int iostream::timed_write_wait(int write_wait_timeout)
{
    return engine_timed_write_wait_(*this, write_wait_timeout);
}

int iostream::engine_read(void *buf, int len)
{
    return engine_read_(*this, buf, len);
}

int iostream::engine_write(const void *buf, int len)
{
    return engine_write_(*this, buf, len);
}

}

