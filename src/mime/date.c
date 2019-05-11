/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-15
 * ================================
 */

#include "zc.h"
#include <time.h>

long zmime_header_line_decode_date(const char *str)
{
#define ___IGNORE_BLANK(p)	{while((*p) && (*p== ' ' )) p++;}
#define ___FIND_BLANK(p)	{while((*p) && (*p!= ' ' )) p++;}
#define ___FIND_COLON(p)	{while((*p) && (*p!= ':' )) p++;}
#define ___FIND_DIGIT(p)	{while((*p) && ((*p<'0')||(*p>'9'))) p++;}
#define ___ERR()		{if(!*p) goto err;}

    char str_copy[128];
    char *ps, *p, *p2, *p3;
    int v, sign, offset;
    struct tm tm;
    long result = -1;

    memset(&tm, 0, sizeof(struct tm));
    strncpy(str_copy, str, 127);
    str_copy[127] = 0;
    zstr_tolower(str_copy);
    for (p = str_copy;*p;p++) {
        if (*p == '-') {
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
    if (p - ps != 3) {
        goto err;
    }
    *(p++) = 0;
    v = -1;
    p2 = p + 1;
    p3 = p + 2;
    switch (*ps) {
    case 'a':
        if (*p2 == 'p') {
            v = 3;
        } else {
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
        if (*p2 == 'a') {
            v = 0;
        } else if (*p3 == 'n') {
            v = 5;
        } else {
            v = 6;
        }
        break;
    case 'm':
        if (*p3 == 'r') {
            v = 2;
        } else {
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
    if (v == -1) {
        goto err;
    }
    tm.tm_mon = v;

    ___IGNORE_BLANK(p);
    ___ERR();
    ps = p;
    ___FIND_BLANK(p);
    ___ERR();
    if (p - ps != 4) {
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
    ps = p;
    if (strlen(ps) < 5) {
        goto err;
    }
    sign = (ps[0] == '-') ? -1 : 1;
    ps++;
    offset = (ps[0] - '0') * 10 + (ps[1] - '0');
    ps += 2;
    offset = offset * 60 + (ps[0] - '0') * 10 + (ps[1] - '0');
    offset *= 60;

    result = timegm(&tm);
    if (result == -1) {
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
