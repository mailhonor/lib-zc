/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2018-02-24
 * ================================
 */

#include "zc.h"

zbuf_t *zsqlite3_escape_append(zbuf_t *sql, const void *data, int size)
{
    int i;
    const char *s = (const char *)data;
    if (size < 0) {
        size = strlen(s);
    }
    int ch;
    for(i = 0; i < size; i++) {
        ch = s[i];
        if (ch == '\'') {
            zbuf_put(sql, ch);
            zbuf_put(sql, ch);
            continue;
        }
#if 0
        switch(ch) {
            case '/':
            case '[':
            case ']':
            case '%':
            case '&':
            case '_':
            case '(':
            case ')':
                zbuf_put(sql, '\\');
        }
#endif
        zbuf_put(sql, ch);
    }
    return sql;
}
