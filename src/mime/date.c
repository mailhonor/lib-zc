/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-15
 * ================================
 */

#include <time.h>
#include "./mime.h"

ssize_t zmime_header_line_decode_date(const char *str)
{
#define ___IGNORE_BLANK(p)          \
    {                               \
        while ((*p) && (*p == ' ')) \
            p++;                    \
    }
#define ___FIND_BLANK(p)            \
    {                               \
        while ((*p) && (*p != ' ')) \
            p++;                    \
    }
#define ___FIND_COLON(p)            \
    {                               \
        while ((*p) && (*p != ':')) \
            p++;                    \
    }
#define ___FIND_DIGIT(p)                           \
    {                                              \
        while ((*p) && ((*p < '0') || (*p > '9'))) \
            p++;                                   \
    }
#define ___ERR()      \
    {                 \
        if (!*p)      \
            goto err; \
    }

    char str_copy[128];
    char *ps, *p;
    int v, sign, offset, last_offset = 0;
    struct tm tm;
    ssize_t result = -1;

    memset(&tm, 0, sizeof(struct tm));
    strncpy(str_copy, str, 127);
    str_copy[127] = 0;
    zstr_tolower(str_copy);
    for (p = str_copy; *p; p++)
    {
        if (*p == '-')
        {
            last_offset = (int)(p - str_copy);
            *p = ' ';
        }
    }
    p = str_copy;

    ___FIND_DIGIT(p);
    ___ERR();

    ps = p;
    ___FIND_BLANK(p);
    ___ERR();
    *(p++) = 0;
    tm.tm_mday = atoi(ps);

    ___IGNORE_BLANK(p);
    ___ERR();
    ps = p;
    ___FIND_BLANK(p);
    ___ERR();
    if (p - ps != 3)
    {
        goto err;
    }
    *(p++) = 0;
    v = -1;
    switch (*ps)
    {
    case 'a':
        if (p[1] == 'p')
        {
            v = 3;
        }
        else
        {
            v = 7;
        }
        break;
    case 'd':
        v = 11;
        break;
    case 'f':
        v = 1;
        break;
    case 'j':
        if (ps[1] == 'a')
        {
            v = 0;
        }
        else if (ps[2] == 'n')
        {
            v = 5;
        }
        else
        {
            v = 6;
        }
        break;
    case 'm':
        if (ps[2] == 'r')
        {
            v = 2;
        }
        else
        {
            v = 4;
        }
        break;
    case 'n':
        v = 10;
        break;
    case 'o':
        v = 9;
        break;
    case 's':
        v = 8;
        break;
    }
    if (v == -1)
    {
        goto err;
    }
    tm.tm_mon = v;

    ___IGNORE_BLANK(p);
    ___ERR();
    ps = p;
    ___FIND_BLANK(p);
    ___ERR();
    if (p - ps != 4)
    {
        goto err;
    }
    *(p++) = 0;
    tm.tm_year = atoi(ps) - 1900;

    ___IGNORE_BLANK(p);
    ___ERR();
    ps = p;
    ___FIND_COLON(p);
    ___ERR();
    *(p++) = 0;
    tm.tm_hour = atoi(ps);

    ps = p;
    ___FIND_COLON(p);
    ___ERR();
    *(p++) = 0;
    tm.tm_min = atoi(ps);

    ps = p;
    ___FIND_BLANK(p);
    ___ERR();
    *(p++) = 0;
    tm.tm_sec = atoi(ps);

    ___IGNORE_BLANK(p);
    ___ERR();
    if ((int)(p - str_copy) == last_offset + 1)
    {
        p[-1] = '-';
        p--;
    }
    ps = p;
    offset = 0;
    sign = 0;
    if (strlen((char *)ps) < 5)
    {
        goto break_offset;
    }
    sign = (ps[0] == '-') ? -1 : 1;
    ps++;
    offset = (ps[0] - '0') * 10 + (ps[1] - '0');
    ps += 2;
    offset = offset * 60 + (ps[0] - '0') * 10 + (ps[1] - '0');
    offset *= 60;
break_offset:

    result = ztimegm(&tm);
    if (result == -1)
    {
        goto err;
    }
    result = result - sign * offset;

#undef ___IGNORE_BLANK
#undef ___FIND_BLANK
#undef ___FIND_COLON
#undef ___FIND_DIGIT
#undef ___ERR

err:
    return result;
}
