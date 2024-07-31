/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-08-31
 * ================================
 */

#include "zc.h"

void zxml_unescape_string(zbuf_t *content, const char *data, int len)
{
    if (len < 0)
    {
        len = strlen(data);
    }
    char *ps = (char *)data, *end = ps + len;
    for (; ps < end;)
    {
        int ch = ps[0];
        if (ch == '<')
        {
            break;
        }
        ps++;
        if (ch == '&')
        {
            if (ps[0] == '#')
            {
                ps += 1;
                int u = 0, x = 10;
                if ((ps[0] == 'x') || (ps[0] == 'X'))
                {
                    x = 16;
                    ps++;
                }
                for (; ps < end; ps++)
                {
                    if (ps[0] == ';')
                    {
                        break;
                    }
                    u = u * x + zhexval(ps[0]);
                }
                char wchar[6 + 1];
                int nlen = zncr_decode(u, wchar);
                if (nlen > 0)
                {
                    zbuf_memcat(content, wchar, nlen);
                }
            }
            else
            {
                if (!strncmp(ps, "lt;", 3))
                {
                    ps += 3;
                    ch = '<';
                }
                else if (!strncmp(ps, "gt;", 3))
                {
                    ps += 3;
                    ch = '>';
                }
                else if (!strncmp(ps, "amp;", 4))
                {
                    ps += 4;
                    ch = '&';
                }
                else if (!strncmp(ps, "apos;", 5))
                {
                    ps += 5;
                    ch = '\'';
                }
                else if (!strncmp(ps, "quot;", 5))
                {
                    ps += 5;
                    ch = '"';
                }
                else
                {
                    ch = 0;
                }
                if (ch)
                {
                    zbuf_put(content, ch);
                }
            }
        }
        else
        {
            zbuf_put(content, ch);
        }
    }
}
