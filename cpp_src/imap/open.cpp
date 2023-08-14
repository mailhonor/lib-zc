/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

void imap_client::set_ssl_tls(bool ssl_mode, bool tls_mode, SSL_CTX *ssl_ctx)
{
    ssl_mode_ = ssl_mode;
    tls_mode_ = tls_mode;
    ssl_ctx_ = ssl_ctx;
}

void imap_client::set_destination(const char *destination)
{
    destination_ = destination;
}
void imap_client::set_timeout(int timeout)
{
    timeout_ = timeout;
}

bool imap_client::connect(int times)
{
    if (connected_)
    {
        ok_no_bad_ = result_onb::ok;
        return true;
    }
    if (times > 0)
    {
        for (int i = 0; i < times; i++)
        {
            if (connect(0))
            {
                ok_no_bad_ = result_onb::ok;
                return true;
            }
        }
        return false;
    }
    connection_error_ = true;
    if (!fp_.connect(destination_.c_str(), timeout_))
    {
        connection_error_ = true;
        zcc_imap_client_error("连接(%s)", destination_.c_str());
        return false;
    }
    if (ssl_mode_)
    {
        if (fp_.tls_connect(ssl_ctx_) < 0)
        {
            connection_error_ = true;
            zcc_imap_client_error("建立SSL(%s)", destination_.c_str());
            return false;
        }
        ssl_flag_ = true;
    }
    connected_ = true;
    ok_no_bad_ = result_onb::ok;
    return true;
}

void imap_client::disconnect()
{
    fp_.close();
    opened_ = false;
    connected_ = false;
    ssl_flag_ = false;
    error_ = false;
    connection_error_ = false;
    need_close_connection_ = false;
    password_error_ = false;
    logic_error_ = false;
}

bool imap_client::welcome()
{
    std::string linebuf;
    if (fp_gets(linebuf, 10240) < 0)
    {
        zcc_imap_client_info("错误: imap 读取 welcome 失败");
        return false;
    }
    trim_rn(linebuf);
    zcc_imap_client_debug("imap 读: %s", linebuf.c_str());
    if (ZSTR_N_EQ(linebuf.c_str(), "* OK ", 5))
    {
        ok_no_bad_ = result_onb::ok;
    }
    else
    {
        ok_no_bad_ = result_onb::bad;
    }
    return true;
}

bool imap_client::open()
{
    if (opened_)
    {
        ok_no_bad_ = result_onb::ok;
        return true;
    }

    if (!connect(3))
    {
        return false;
    }
    if (ok_no_bad_ != result_onb::ok)
    {
        return true;
    }

    if (!welcome())
    {
        return false;
    }
    if (ok_no_bad_ != result_onb::ok)
    {
        return true;
    }

    if ((!auth()) || (!result_is_ok()))
    {
        return false;
    }
    if (ok_no_bad_ != result_onb::ok)
    {
        return true;
    }

    if (get_capability("id"))
    {
        if (!cmd_id())
        {
            return false;
        }
        if (ok_no_bad_ != result_onb::ok)
        {
            return true;
        }
    }

    return true;
}

void imap_client::close()
{
    disconnect();
}

zcc_namespace_end;
