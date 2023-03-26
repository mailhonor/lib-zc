/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-11-18
 * ================================
 */

#include "zc.h"

namespace zcc
{

msgbox::msgbox()
{
    msgbox__ = this;
}

msgbox::~msgbox()
{
}

void msgbox::clear_allmsg()
{
    errmsg_.clear();
    infomsg_.clear();
}

void msgbox::set_errmsg(const char *source_fn, size_t line_number, const char *fmt, ...)
{
    va_list ap;
    char buf[10240 + 1];
    va_start(ap, fmt);
    vsnprintf(buf, 10240, fmt, ap);
    va_end(ap);
    errmsg_ = buf;
}

void msgbox::vset_errmsg(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    char buf[10240 + 1];
    vsnprintf(buf, 10240, fmt, ap);
    errmsg_ = buf;
}

bool msgbox::errmsg_is_ok()
{
    if (errmsg_.empty() || (errmsg_ == "OK"))
    {
        return true;
    }
    return false;
}

} // namespace zcc

