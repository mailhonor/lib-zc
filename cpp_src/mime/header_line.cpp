/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

std::string mail_parser::header_line_unescape(const char *in_line, int64_t in_len)
{
    std::string result;
    int64_t ch, i;
    char *src = (char *)(void *)(in_line);

    for (i = 0; i < in_len; i++)
    {
        ch = src[i];
        if (ch == '\0')
        {
            continue;
        }
        if (ch == '\r')
        {
            continue;
        }
        if (ch == '\n')
        {
            i++;
            if (i == in_len)
            {
                break;
            }
            ch = src[i];
            if (ch == '\t')
            {
                continue;
            }
            if (ch == ' ')
            {
                continue;
            }
            result.push_back('\n');
        }
        result.push_back(ch);
    }
    return result;
}

int64_t mail_parser::header_line_get_first_token(const char *line_, int64_t in_len, char **val)
{
    int64_t i, vlen, ch, len = in_len;
    char *line = (char *)(void *)line_, *ps, *pend = line + len;

    *val = line;
    vlen = 0;
    for (i = 0; i < len; i++)
    {
        ch = line[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '<'))
        {
            continue;
        }
        break;
    }
    if (i == len)
    {
        return vlen;
    }
    ps = line + i;
    len = pend - ps;
    for (i = len - 1; i >= 0; i--)
    {
        ch = ps[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '>'))
        {
            continue;
        }
        break;
    }
    if (i < 0)
    {
        return vlen;
    }

    *val = ps;
    vlen = i + 1;
    return vlen;
}

std::string mail_parser::header_line_get_first_token(const char *line, int64_t in_len)
{
    std::string result;
    char *v;
    int64_t l = header_line_get_first_token(line, in_len, &v);
    if (l > 0)
    {
        result.append(v, l);
    }
    return result;
}

void mail_parser::header_line_node::reset()
{
    data_.clear();
    charset_.clear();
    encode_type_ = mail_header_encode_none;
}

std::vector<mail_parser::header_line_node> mail_parser::header_line_to_node_vector(const char *in_line, int64_t in_len, int64_t max_count)
{
    std::vector<header_line_node> node_vec;
    if (in_len < 0)
    {
        in_len = std::strlen(in_line);
    }
    if (in_len < 1)
    {
        return node_vec;
    }
    header_line_node node;
    int64_t clen, dlen, found, encode;
    const char *ps = in_line, *pend = ps + in_len, *p, *p_s, *p_charset, *p_encode, *p_data, *ps_next;
    while ((ps < pend) && (pend - ps > 6))
    {
        found = 0;
        p_s = ps;
        while ((p_s < pend) && (pend - p_s > 6))
        {
            if (!(p_s = (char *)memmem(p_s, pend - p_s, "=?", 2)))
            {
                break;
            }
            if ((pend <= p_s) || (pend - p_s < 6))
            {
                break;
            }
            p_charset = p_s + 2;

            if (!(p_encode = (char *)std::memchr(p_charset, '?', pend - p_charset)))
            {
                break;
            }
            clen = p_encode - p_charset;
            if (pend - p_encode < 3)
            {
                break;
            }
            p_encode++;
            encode = toupper(p_encode[0]);
            if (((encode != 'B') && (encode != 'Q')) || (p_encode[1] != '?'))
            {
                p_s = p_encode - 1;
                continue;
            }
            p_data = p_encode + 2;
            if (pend <= p_data)
            {
                break;
            }
            found = 1;
            p = (char *)memmem(p_data, pend - p_data, "?=", 2);
            if (p)
            {
                ps_next = p + 2;
                dlen = p - p_data;
            }
            else
            {
                ps_next = pend;
                dlen = pend - p_data;
            }
            break;
        }
        if (!found)
        {
            break;
        }
        if (ps < p_s)
        {
            node.reset();
            node.data_.append(ps, p_s - ps);
            node_vec.push_back(node);
        }

        node.reset();
        node.data_.append(p_data, dlen);
        node.charset_.append(p_charset, clen);
        tolower(node.charset_);
        if (encode == 'B')
        {
            node.encode_type_ = mail_header_encode_base64;
        }
        else if (encode == 'Q')
        {
            node.encode_type_ = mail_header_encode_qp;
        }
        node_vec.push_back(node);

        ps = ps_next;
    }
    if (ps < pend)
    {
        node.reset();
        node.data_.append(ps, pend - ps);
        node_vec.push_back(node);
    }

    return node_vec;
}

std::string mail_parser::header_line_get_utf8(const char *src_charset_def, const char *in_line, int64_t in_len)
{
    std::string result;
    if (in_len == -1)
    {
        in_len = strlen(in_line);
    }
    if (in_len < 1)
    {
        return result;
    }

    std::string bq_join, tmp_string;
    int64_t i, mt_count;
    char *in_src = (char *)(void *)in_line;
    std::vector<header_line_node> mt_vec = header_line_to_node_vector(in_src, in_len, 102400);
    mt_count = mt_vec.size();

    for (i = 0; i < mt_count; i++)
    {
        header_line_node *mt = &(mt_vec[i]);
        if (mt->data_.empty())
        {
            continue;
        }
        if ((mt->encode_type_ != mail_header_encode_base64) && (mt->encode_type_ != mail_header_encode_qp))
        {
            tmp_string = charset_convert(src_charset_def, mt->data_);
            result.append(tmp_string);
            continue;
        }
        bq_join = mt->data_;
        header_line_node *mtn = ((i + 1 < mt_count) ? &(mt_vec[i + 1]) : nullptr);

        while (1)
        {
            if (i + 1 >= mt_count)
            {
                break;
            }
            if (mtn->encode_type_ == mail_header_encode_none)
            {
                int64_t j, dlen = mtn->data_.size();
                for (j = 0; j < dlen; j++)
                {
                    int64_t c = mtn->data_[j];
                    if (c == ' ')
                    {
                        continue;
                    }
                    break;
                }
                if (j == dlen)
                {
                    i++;
                    mtn = ((i + 1 < mt_count) ? &(mt_vec[i + 1]) : nullptr);
                    continue;
                }
                break;
            }
            if (mtn && (mt->data_.size()) && (mt->encode_type_ == mtn->encode_type_) && (mt->charset_ == mtn->charset_))
            {
                bq_join.append(mtn->data_);
                i++;
                mtn = ((i + 1 < mt_count) ? &(mt_vec[i + 1]) : nullptr);
                continue;
            }
            break;
        }
        tmp_string.clear();
        if (mt->encode_type_ == mail_header_encode_base64)
        {
            zcc::base64_decode(bq_join, tmp_string);
        }
        else if (mt->encode_type_ == mail_header_encode_qp)
        {
            zcc::qp_decode_2047(bq_join, tmp_string);
        }

        if (tmp_string.empty())
        {
            continue;
        }
        result.append(charset_convert(mt->charset_.c_str(), tmp_string));
    }
    return result;
}

zcc_namespace_end;
