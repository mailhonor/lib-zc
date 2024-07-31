/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-11-16
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

imap_client::response_tokens::response_tokens()
{
    reset();
}

imap_client::response_tokens::~response_tokens()
{
}

void imap_client::response_tokens::reset()
{
    token_vector_.clear();
    first_line_.clear();
}

int imap_client::read_response_tokens_oneline(response_tokens &response_tokens, int &extra_length)
{
    if (need_close_connection_)
    {
        return -1;
    }
    auto &token_vector = response_tokens.token_vector_;
    extra_length = -1;
    int r = -1;
    bool last_quoted = false;
    std::string linebuf, tmp_token;
    bool tmp_token_begin;
    int ch;
    const char *ps, *end;

    if (fp_gets(linebuf, simple_line_length_limit_) < 0)
    {
        return -1;
    }

    trim_line_end_rn(linebuf);
    if (response_tokens.first_line_.empty()) {
        response_tokens.first_line_ = linebuf;
    }
    if (tmp_verbose_mode_)
    {
        tmp_verbose_line_++;
    }
    if ((tmp_verbose_line_ < 3) || (verbose_mode_))
    {
        zcc_imap_client_debug_read_line(linebuf);
    }
    if ((tmp_verbose_line_ == 3) && (!verbose_mode_))
    {
        std::string tmpline = linebuf;
        tmpline.append(" (后面忽略; 显示全部结果, 用参数 --verbose)");
        zcc_imap_client_debug_read_line(tmpline);
    }

    ps = linebuf.c_str();
    end = ps + linebuf.size();
    while (ps < end)
    {
        tmp_token_begin = false;
        tmp_token.clear();
        while (ps < end)
        {
            ch = *ps++;
            if (ch != ' ')
            {
                break;
            }
        }
        if (ch == ' ')
        {
            continue;
        }
        else if (ch == '"')
        {
            last_quoted = true;
            tmp_token_begin = true;
            while (ps < end)
            {
                ch = *ps++;
                if (ch == '"')
                {
                    token_vector.push_back(tmp_token);
                    break;
                }
                else if (ch == '\\')
                {
                    if (ps == end)
                    {
                        need_close_connection_ = true;
                        goto over;
                    }
                    ch = *ps++;
                    tmp_token.push_back(ch);
                }
                else
                {
                    tmp_token.push_back(ch);
                }
            }
        }
        else
        {
            last_quoted = false;
            tmp_token.push_back(ch);
            while (ps < end)
            {
                ch = *ps++;
                if (ch == ' ')
                {
                    break;
                }
                else
                {
                    tmp_token.push_back(ch);
                }
            }
            token_vector.push_back(tmp_token);
        }
    }
    r = 1;
over:
    while ((r > 0) && (!last_quoted) && token_vector.size())
    {
        char testbuf[128];
        std::string &tmpstr = token_vector.back();
        if ((tmpstr[0] != '{') || (tmpstr.back() != '}'))
        {
            break;
        }
        int next_len = atoi(tmpstr.c_str() + 1);
        std::sprintf(testbuf, "{%d}", next_len);
        if (tmpstr != testbuf)
        {
            break;
        }
        extra_length = next_len;
        break;
    }
    return r;
}

int imap_client::read_response_tokens(response_tokens &response_tokens)
{
    if (need_close_connection_)
    {
        return -1;
    }
    auto &token_vector = response_tokens.token_vector_;
    int r = -1;
    int extra_length;
    std::string tmpstr;
    while (1)
    {
        if ((r = read_response_tokens_oneline(response_tokens, extra_length)) < 0)
        {
            break;
        }
        if (extra_length < 0)
        {
            r = 1;
            break;
        }
        if (token_vector.empty())
        {
            r = 1;
            break;
        }
        std::string sizecon;
        if (extra_length > 0)
        {
            sizecon.reserve(extra_length);
            if (fp_readn(sizecon, extra_length) < extra_length)
            {
                r = -1;
                break;
            }
        }
        token_vector.back() = sizecon;
    }
    return r;
}

int imap_client::read_big_data(FILE *dest_fp, std::string &raw_content, int extra_length)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (extra_length < 0)
    {
        if ((raw_content.size()) && (raw_content != "NIL"))
        {
            fwrite(raw_content.c_str(), 1, raw_content.size(), dest_fp);
        }
        return 1;
    }

    int all_rlen = 0;
    std::string buf;
    while (all_rlen < extra_length)
    {
        int rlen = extra_length - all_rlen;
        if (rlen > 10240)
        {
            rlen = 10240;
        }
        buf.clear();
        if (fp_readn(buf, rlen) < rlen)
        {
            return -1;
        }
        if (all_rlen < 1024 * 1024 * 1024)
        {
            fwrite(buf.c_str(), 1, buf.size(), dest_fp);
        }
        all_rlen += rlen;
    }

    return 1;
}

int imap_client::ignore_left_token(int extra_length)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (extra_length > 0)
    {
        if (fp_readn(0, extra_length) < extra_length)
        {
            return -1;
        }
    }
    response_tokens response_tokens;
    return read_response_tokens(response_tokens);
}

zcc_namespace_end;
