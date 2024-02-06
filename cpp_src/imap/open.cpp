/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

void imap_client::set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool tls_try_mode)
{
    ssl_mode_ = ssl_mode;
    tls_mode_ = tls_mode;
    ssl_ctx_ = ssl_ctx;
    tls_try_mode_ = tls_try_mode;
}

void imap_client::set_timeout(int timeout)
{
    timeout_ = timeout;
}

int imap_client::do_startTLS()
{
    if (ssl_flag_)
    {
        return 1;
    }
    int r;
    if ((r = do_quick_cmd_simple_line_mode("S STARTTLS")) < 1)
    {
        return r;
    }

    if (fp_.tls_connect(ssl_ctx_) < 0)
    {
        need_close_connection_ = true;
        zcc_imap_client_error("建立SSL");
        return -1;
    }
    ssl_flag_ = true;

    return 1;
}

int imap_client::connect(const char *destination, int times)
{
    int r;
    if (connected_)
    {
        return 1;
    }
    if (times > 0)
    {
        for (int i = 0; i < times; i++)
        {
            if ((r = connect(destination, 0)) > 0)
            {
                return r;
            }
        }
        return -1;
    }
    if ((r = fp_.connect(destination, timeout_)) < 1)
    {
        need_close_connection_ = true;
        zcc_imap_client_error("连接(%s)", destination);
        return r;
    }
    if (ssl_mode_)
    {
        if (fp_.tls_connect(ssl_ctx_) < 0)
        {
            need_close_connection_ = true;
            zcc_imap_client_error("建立SSL(%s)", destination);
            return -1;
        }
        ssl_flag_ = true;
    }
    connected_ = true;
    return 1;
}

void imap_client::disconnect()
{
    fp_.close();
    opened_ = false;
    connected_ = false;
    ssl_flag_ = false;
    logout_ = false;
}

int imap_client::welcome()
{
    std::string linebuf;
    if (fp_gets(linebuf, 10240) < 0)
    {
        need_close_connection_ = true;
        zcc_imap_client_info("错误: imap 读取 welcome 失败");
        return -1;
    }
    trim_line_end_rn(linebuf);
    zcc_imap_client_debug("imap 读: %s", linebuf.c_str());
    if (ZSTR_N_EQ(linebuf.c_str(), "* OK ", 5))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int imap_client::open(const char *destination)
{
    int r;
    if (opened_)
    {
        return 1;
    }

    if ((r = connect(destination, 3)) < 1)
    {
        return r;
    }

    if ((r = welcome()) < 1)
    {
        return r;
    }

    if (tls_mode_)
    {
        if ((r = do_startTLS()) < 1)
        {
            if (r < 0)
            {
                return r;
            }
            if (!tls_try_mode_)
            {
                return -1;
            }
        }
    }

    return 1;
}

int imap_client::cmd_logout()
{
    if (logout_)
    {
        return 1;
    }
    if (need_close_connection_)
    {
        return 1;
    }
    do_quick_cmd_simple_line_mode("L LOGOUT");
    logout_ = true;
    need_close_connection_ = true;
    return 1;
}

void imap_client::close()
{
    cmd_logout();
    disconnect();
}

zcc_namespace_end;
