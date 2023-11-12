/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#include "./mime.h"

static zinline char *ignore_chs(char *p, int plen, const char *ignore, int ignore_len)
{
    char *chs = (char *)(void *)ignore;
    int i, j;
    for (i = 0; i < plen; i++)
    {
        for (j = 0; j < ignore_len; j++)
        {
            if (p[i] == chs[j])
            {
                break;
            }
        }
        if (j == ignore_len)
        {
            return (p + i);
        }
    }
    return p + plen;
}

static zinline char *find_delim(char *p, int plen, const char *delim, int delim_len)
{
    char *chs = (char *)(void *)delim;
    int i, j;
    for (i = 0; i < plen; i++)
    {
        for (j = 0; j < delim_len; j++)
        {
            if (p[i] == chs[j])
            {
                return (p + i);
            }
        }
    }
    return 0;
}

static int find_value(char *buf, int len, zbuf_t *value, char **nbuf, int is_filename)
{
    zbuf_reset(value);
    char *ps = buf, *pend = ps + len, *p;
    int ch;

    ps = ignore_chs(ps, pend - ps, " \t", 2);
    if (ps == pend)
    {
        return -1;
    }
    if (*ps == '"')
    {
        ps++;
        while (ps < pend)
        {
            ch = *ps++;
            if (ch == '"')
            {
                break;
            }
            else if (ch == '\\')
            {
                if (ps == pend)
                {
                    break;
                }
                int ch2 = *ps++;
                if ((ch2 != '*') && (ch2 != '\\'))
                {
                    // 兼容 Foxmail 较早的版本
                    zbuf_put(value, ch);
                }
                zbuf_put(value, ch2);
            }
            else
            {
                zbuf_put(value, ch);
            }
        }
    }
    else
    {
        p = find_delim(ps, pend - ps, " \t;", 3);
        if (!p)
        {
            zbuf_memcat(value, ps, pend - ps);
            ps = pend;
        }
        else
        {
            zbuf_memcat(value, ps, p - ps);
            ps = p + 1;
        }
    }

    *nbuf = ps;
    return 0;
}

static int find_next_kv(char *buf, int len, zbuf_t *key, zbuf_t *value, char **nbuf)
{
    zbuf_reset(key);
    zbuf_reset(value);
    int is_filename = 0;

    char *ps = buf, *pend = ps + len, *p;
    int ch;

    ps = ignore_chs(ps, pend - ps, " \t;", 3);
    if (ps == pend)
    {
        return -1;
    }

    p = find_delim(ps, pend - ps, "= \t", 3);
    if (!p)
    {
        zbuf_memcat(key, ps, pend - ps);
        *nbuf = pend;
        return 0;
    }
    zbuf_memcat(key, ps, p - ps);
    if (*p == '=')
    {
        ps = p + 1; // begin value
    }
    else
    {
        ps = ignore_chs(p, pend - p, " \t", 2);
        if (ps == pend)
        {
            *nbuf = pend;
            return 0;
        }
        if (*ps != '=')
        {
            *nbuf = ps;
            return 0;
        }
        ps = ps + 1;
    }

    if (!strcasecmp(zbuf_data(key), "filename"))
    {
        is_filename = 1;
    }
    if (find_value(ps, pend - ps, value, &ps, is_filename))
    {
        *nbuf = pend;
        return 0;
    }

    *nbuf = ps;
    return 0;
}

void zmime_header_line_get_params(const char *in_line, int in_len, zbuf_t *val, zdict_t *params)
{
    char *ps = (char *)(void *)in_line, *pend = ps + in_len;
    if (find_value(ps, pend - ps, val, &ps, 0))
    {
        return;
    }

    zbuf_t *param_key = zbuf_create(64);
    zbuf_t *param_value = zbuf_create(128);

    while (ps < pend)
    {
        if (find_next_kv(ps, pend - ps, param_key, param_value, &ps))
        {
            break;
        }
        zdict_update(params, zbuf_data(param_key), param_value);
    }

    zbuf_free(param_key);
    zbuf_free(param_value);
}

/* */
void zmime_header_line_decode_content_type_inner(zmail_t *parser, const char *data, int len, char **_value, char **boundary, int *boundary_len, char **charset, char **name)
{
    zbuf_t *param_key = zbuf_create(64);
    zbuf_t *param_value = zbuf_create(128);

    char *ps = (char *)(void *)data, *pend = ps + len;
    if (find_value(ps, pend - ps, param_value, &ps, 0))
    {
        zbuf_free(param_key);
        zbuf_free(param_value);
        return;
    }
    zfree(*_value);
    *_value = zmemdupnull(zbuf_data(param_value), zbuf_len(param_value));

    while (ps < pend)
    {
        if (find_next_kv(ps, pend - ps, param_key, param_value, &ps))
        {
            break;
        }
        char *key = zbuf_data(param_key);
        int key_len = zbuf_len(param_key);

        if (key_len == 8 && ZSTR_N_CASE_EQ(key, "boundary", key_len))
        {
            zfree(*boundary);
            *boundary = zmemdupnull(zbuf_data(param_value), zbuf_len(param_value));
            *boundary_len = zbuf_len(param_value);
        }
        else if (key_len == 7 && ZSTR_N_CASE_EQ(key, "charset", key_len))
        {
            zfree(*charset);
            *charset = zmemdupnull(zbuf_data(param_value), zbuf_len(param_value));
        }
        else if (key_len == 4 && ZSTR_N_CASE_EQ(key, "name", key_len))
        {
            zfree(*name);
            *name = zmemdupnull(zbuf_data(param_value), zbuf_len(param_value));
        }
    }

    zbuf_free(param_key);
    zbuf_free(param_value);
}

/* inner use */
void zmime_header_line_decode_content_disposition_inner(zmail_t *parser, const char *data, int len, char **_value, char **filename, char **filename_2231, int *filename_2231_with_charset_flag)
{
    zbuf_t *param_key = zbuf_create(64);
    zbuf_t *param_value = zbuf_create(128);

    char *ps = (char *)(void *)data, *pend = ps + len;
    if (find_value(ps, pend - ps, param_value, &ps, 0))
    {
        zbuf_free(param_key);
        zbuf_free(param_value);
        return;
    }
    zfree(*_value);
    *_value = zmemdupnull(zbuf_data(param_value), zbuf_len(param_value));

    zbuf_t *filename2231_tmpbf = 0;
    int flag_2231 = 0;
    int charset_2231 = 0;
    while (ps < pend)
    {
        if (find_next_kv(ps, pend - ps, param_key, param_value, &ps))
        {
            break;
        }
        char *key = zbuf_data(param_key);
        int key_len = zbuf_len(param_key);

        if (key_len == 8 && ZSTR_N_CASE_EQ(key, "filename", key_len))
        {
            zfree(*filename);
            *filename = zmemdupnull(zbuf_data(param_value), zbuf_len(param_value));
        }
        else if ((key_len > 8) && (ZSTR_N_CASE_EQ(key, "filename*", 9)))
        {
            if (filename2231_tmpbf == 0)
            {
                filename2231_tmpbf = zmail_zbuf_cache_require(parser, 1024);
            }
            char *value = zbuf_data(param_value);
            int value_len = zbuf_len(param_value);
            zbuf_memcat(filename2231_tmpbf, value, value_len);
            if (!flag_2231)
            {
                flag_2231 = 1;
                if (key_len == 9)
                {
                    int count = 0;
                    for (int i = 0; i < value_len; i++)
                    {
                        if (((unsigned char *)value)[i] == '\'')
                        {
                            count++;
                        }
                    }
                    if (count > 1)
                    {
                        charset_2231 = 1;
                    }
                    else
                    {
                        charset_2231 = 0;
                    }
                }
                else if (key_len == 10)
                {
                    charset_2231 = 0;
                }
                else if (key_len == 11)
                {
                    charset_2231 = 1;
                }
            }
        }
    }

    zbuf_free(param_key);
    zbuf_free(param_value);

    *filename_2231_with_charset_flag = charset_2231;
    if (filename2231_tmpbf)
    {
        zfree(*filename_2231);
        *filename_2231 = zmemdupnull(zbuf_data(filename2231_tmpbf), zbuf_len(filename2231_tmpbf));
        zmail_zbuf_cache_release(parser, filename2231_tmpbf);
    }
}

/* inner use */
void zmime_header_line_decode_content_transfer_encoding_inner(zmail_t *parser, const char *data, int len, char **_value)
{
    zbuf_t *param_value = zbuf_create(128);

    char *ps = (char *)(void *)data, *pend = ps + len;
    if (find_value(ps, pend - ps, param_value, &ps, 0))
    {
        zbuf_free(param_value);
        return;
    }
    zfree(*_value);
    *_value = zmemdupnull(zbuf_data(param_value), zbuf_len(param_value));
    zbuf_free(param_value);
}
