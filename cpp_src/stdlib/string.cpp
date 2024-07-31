/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include <cstdarg>
#include <algorithm>
#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

char var_blank_buffer[1] = {0};
const std::string var_blank_string = "";
const std::list<std::string> var_blank_list;
const std::vector<std::string> var_blank_vector;
const std::map<std::string, std::string> var_blank_map;

/* ################################################################## */
unsigned const char var_char_xdigitval_vector[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/* trim */
#define ___SKIP(start, var, cond)            \
    for (var = start; *var && (cond); var++) \
        ;
#define ___DELETE(ch) ((isascii(ch) && std::isspace(ch)) || std::iscntrl(ch))
#define ___TRIM(s)                                                  \
    {                                                               \
        char *p;                                                    \
        for (p = (s) + strlen(s); p > (s) && ___DELETE(p[-1]); p--) \
            ;                                                       \
        *p = 0;                                                     \
    }

char *trim_left(char *str)
{
    char *np;

    ___SKIP(str, np, ___DELETE(*np));

    return np;
}

char *trim_right(char *str)
{

    ___TRIM(str);

    return str;
}

char *trim(char *str)
{
    char *np;

    ___SKIP(str, np, ___DELETE(*np));
    ___TRIM(np);

    return np;
}

/* skip */
char *skip_left(const char *str, const char *ignores)
{
    char ch;
    int i = 0;

    while ((ch = str[i]))
    {
        if (std::strchr(ignores, ch))
        {
            i++;
            continue;
        }
        return (char *)str + i;
    }
    return 0;
}

int skip_right(const char *str, int len, const char *ignores)
{
    int i = 0;
    if (len < 0)
    {
        len = std::strlen(str);
    }
    for (i = len - 1; i >= 0; i--)
    {
        if (std::strchr(ignores, str[i]))
        {
            continue;
        }
        break;
    }
#if 0
    if (i < 0 /* i == -1 */) {
        return 0;
    }
#endif
    return i + 1;
}

int skip(const char *line, int len, const char *ignores_left, const char *ignores_right, char **start)
{
    const char *ps, *pend;
    int i, ch;

    if (len < 0)
    {
        len = std::strlen(line);
    }
    pend = line + len;

    for (i = 0; i < len; i++)
    {
        ch = line[i];
        if (std::strchr(ignores_left, ch))
        {
            continue;
        }
        break;
    }
    if (i == len)
    {
        return 0;
    }

    if (!ignores_right)
    {
        ignores_right = ignores_left;
    }
    ps = line + i;
    len = pend - ps;
    for (i = len - 1; i >= 0; i--)
    {
        ch = ps[i];
        if (std::strchr(ignores_right, ch))
        {
            continue;
        }
        break;
    }
    if (i < 0)
    {
        return 0;
    }

    *start = (char *)ps;

    return i + 1;
}

bool str_to_bool(const char *s, bool def)
{
    if (!s)
    {
        return def;
    }
    char first = s[0];
    switch (first)
    {
    case '1':
    case 'y':
    case 't':
    case 'Y':
    case 'T':
        return true;
    case '0':
    case 'n':
    case 'f':
    case 'N':
    case 'F':
        return false;
    }

    return def;
}

int64_t str_to_second(const char *s, int64_t def)
{
    char unit, junk;
    int64_t intval;

    if (!s)
    {
        return def;
    }

    switch (std::sscanf(s, "%zd%c%c", &intval, &unit, &junk))
    {
    case 1:
        unit = 's';
    case 2:
        switch (unit)
        {
        case 'w':
        case 'W':
            return (intval * (7 * 24 * 3600));
        case 'd':
        case 'D':
            return (intval * (24 * 3600));
        case 'h':
        case 'H':
            return (intval * 3600);
        case 'm':
        case 'M':
            return (intval * 60);
        case 's':
        case 'S':
        default:
            return (intval);
        }
    }

    return def;
}

int64_t str_to_size(const char *s, int64_t def)
{
    char unit, junk;
    int64_t intval;

    if (!s)
    {
        return def;
    }

    switch (sscanf(s, "%zd%c%c", &intval, &unit, &junk))
    {
    case 1:
        unit = 'b';
    case 2:
        switch (unit)
        {
        case 'g':
        case 'G':
            return (intval * (1024 * 1024 * 1024));
        case 'm':
        case 'M':
            return (intval * (1024 * 1024));
        case 'k':
        case 'K':
            return (intval * 1024);
        case 'b':
        case 'B':
        default:
            return (intval);
        }
    }

    return def;
}

std::string hunman_byte_size(int64_t a)
{
    std::string r;
    char buf[300], *p = buf, ch;
    int len, m, i;
    int tl = 0;

    std::sprintf(buf, "%zd", a);
    len = strlen(buf);
    m = len % 3;

    while (1)
    {
        for (i = 0; i < m; i++)
        {
            ch = *p++;
            if (ch == '\0')
            {
                goto over;
            }
            r.push_back(ch);
        }
        if (!r.empty())
        {
            r.push_back(',');
        }
        m = 3;
    }

over:
    if ((!r.empty()) && (r.back() == ','))
    {
        r.pop_back();
    }
    return r;
}

char *tolower(char *str)
{
    unsigned char *ps = (unsigned char *)str;
    while (*ps)
    {
        *ps = tolower(*ps);
        ps++;
    }
    return str;
}

char *toupper(char *str)
{
    unsigned char *ps = (unsigned char *)str;
    while (*ps)
    {
        *ps = toupper(*ps);
        ps++;
    }
    return str;
}

char *clear_null(char *data, int64_t size)
{
    char *p = data;
    int64_t i;
    if (size < 0)
    {
        size = std::strlen(p);
    }
    for (i = 0; i < size; i++)
    {
        if (p[i] == '\0')
        {
            p[i] = ' ';
        }
    }
    return data;
}

std::string &clear_null(std::string &data)
{
    int64_t size = data.size();
    for (int64_t i = 0; i < size; i++)
    {
        if (data[i] == '\0')
        {
            data[i] = ' ';
        }
    }
    return data;
}

void *no_memmem(const void *l, int64_t l_len, const void *s, int64_t s_len)
{
    register char *cur, *last;
    const char *cl = (const char *)l;
    const char *cs = (const char *)s;

    /* we need something to compare */
    if (l_len == 0 || s_len == 0)
        return NULL;

    /* "s" must be smaller or equal to "l" */
    if (l_len < s_len)
        return NULL;

    /* special case where s_len == 1 */
    if (s_len == 1)
        return (char *)(void *)std::memchr(l, (int)*cs, l_len);

    /* the last position where its possible to find "s" in "l" */
    last = (char *)cl + l_len - s_len;

    for (cur = (char *)cl; cur <= last; cur++)
        if (cur[0] == cs[0] && std::memcmp(cur, cs, s_len) == 0)
            return cur;

    return NULL;
}

std::string &vsprintf_1024(std::string &str, const char *format, va_list ap)
{
    char buf[1024 + 1];
    std::vsnprintf(buf, 1024, format, ap);
    str.append(buf);
    return str;
}

std::string &sprintf_1024(std::string &str, const char *format, ...)
{
    va_list ap;
    char buf[1024 + 1];

    va_start(ap, format);
    std::vsnprintf(buf, 1024, format, ap);
    va_end(ap);
    str.append(buf);
    return str;
}

static inline int _tolower_xxx(int ch)
{
    return zcc::tolower(ch);
}
std::string &tolower(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), _tolower_xxx);
    return str;
}

static inline int _toupper_xxx(int ch)
{
    return zcc::tolower(ch);
}
std::string &toupper(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), _toupper_xxx);
    return str;
}

std::string &trim_right(std::string &str, const char *delims)
{
    if (empty(delims))
    {
        delims = "\r\n \t";
    }
    int olen = (int)(str.size()), len = olen;
    char *p = (char *)(void *)str.c_str() + len;
    for (; len > 0; len--)
    {
        p--;
        if (!std::strchr(delims, *p))
        {
            break;
        }
    }
    if (len != olen)
    {
        str.resize(len);
    }
    return str;
}

std::string &trim_line_end_rn(std::string &s)
{
    uint64_t size = s.size();
    if (size && (s[size - 1] == '\n'))
    {
        s.resize(size - 1);
        size--;
        if (size && (s[size - 1] == '\r'))
        {
            s.resize(size - 1);
        }
    }
    return s;
}

/* vector */
std::vector<std::string> split(const char *s, int len, const char *delims, bool ignore_empty_token)
{
    const unsigned char *ps = (const unsigned char *)s;
    std::vector<std::string> r;

    if (len < 0)
    {
        len = std::strlen(s);
    }

    if (empty(delims))
    {
        r.push_back(std::string(s, len));
        return r;
    }

    int ch;
    std::string stmp;
    bool added = false;
    for (int i = 0; i < len; i++)
    {
        ch = ps[i];
        if (std::strchr(delims, ch))
        {
            if (!added)
            {
                if ((!ignore_empty_token) || (!stmp.empty()))
                {
                    r.push_back(stmp);
                }
                stmp.clear();
                added = true;
            }
            continue;
        }
        stmp.push_back(ch);
        added = false;
    }
    if ((!ignore_empty_token) || (!stmp.empty()))
    {
        r.push_back(stmp);
    }
    stmp.clear();
    if (r.empty())
    {
        r.push_back("");
    }

    return r;
}
std::vector<std::string> split(const char *s, int len, int delim, bool ignore_empty_token)
{
    const unsigned char *ps = (const unsigned char *)s;
    std::vector<std::string> r;

    if (len < 0)
    {
        len = std::strlen(s);
    }

    int ch;
    std::string stmp;
    bool added = false;
    for (int i = 0; i < len; i++)
    {
        ch = ps[i];
        if (ch == delim)
        {
            if (!added)
            {
                if ((!ignore_empty_token) || (!stmp.empty()))
                {
                    r.push_back(stmp);
                }
                stmp.clear();
                added = true;
            }
            continue;
        }
        stmp.push_back(ch);
        added = false;
    }
    if ((!ignore_empty_token) || (!stmp.empty()))
    {
        r.push_back(stmp);
    }
    stmp.clear();
    if (r.empty())
    {
        r.push_back("");
    }

    return r;
}
#ifdef _WIN64
char *strcasestr(const char *haystack, const char *needle)
{
    int hl = strlen(haystack);
    int nl = strlen(needle);
    int clen = hl - nl;
    if (clen < 0)
    {
        return 0;
    }
    clen++;
    for (int i = 0; i < clen; i++)
    {
        int r = strncasecmp(haystack + i, needle, nl);
        if (r == 0)
        {
            return (char *)(void *)(haystack + i);
        }
    }
    return 0;
}
#endif // _WIN6

zcc_namespace_end;
