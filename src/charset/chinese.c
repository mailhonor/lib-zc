/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-08
 * ================================
 */
#include "libzc.h"
#include "chinese_utf8.h"

/* charset for chinese */

static inline int utf8_len(char *buf, int len)
{
    unsigned char *ptr;
    int ret;
    ptr = (unsigned char *)buf;
    if (((*ptr) <= 0x7F))
    {
        ret = 1;
    }
    else if (((*ptr) & 0xF0) == 0xF0)
    {
        ret = 4;
    }
    else if (((*ptr) & 0xE0) == 0xE0)
    {
        ret = 3;
    }
    else if (((*ptr) & 0xC0) == 0xC0)
    {
        ret = 2;
    }
    else
    {
        ret = 5;
    }

    return ret;
}

static inline int ___chinese_word_find(unsigned char *word)
{
    int start = 0, middle, end = chinese_utf8_list_count - 1;
    unsigned int mint, wint;
    unsigned char *wp;

    wint = (word[0] << 16) | (word[1] << 8) | (word[2]);
    while (1)
    {
        if (start > end)
        {
            return 0;
        }
        middle = (start + end) / 2;
        wp = chinese_utf8_list + middle * 3;
        mint = (wp[0] << 16) | (wp[1] << 8) | (wp[2]);

        if (wint < mint)
        {
            end = middle - 1;
            continue;
        }
        if (mint < wint)
        {
            start = middle + 1;
            continue;
        }
        return 1;
    }

    return 0;
}

static inline int ___chinese_word_find2(unsigned char *word)
{
    int start = 0, middle, end = chinese_utf8_list2_count - 1;
    unsigned int mint, wint;
    unsigned char *wp;

    wint = (word[0] << 8) | (word[1]);
    while (1)
    {
        if (start > end)
        {
            return 0;
        }
        middle = (start + end) / 2;
        wp = chinese_utf8_list2 + middle * 2;
        mint = ((wp[0] << 8) | (wp[1]));

        if (wint < mint)
        {
            end = middle - 1;
            continue;
        }
        if (mint < wint)
        {
            start = middle + 1;
            continue;
        }
        return 1;
    }

    return 0;
}

static int ___chinese_word_count(char *str, int len)
{
    int i = 0, ulen, count = 0;

    while (i + 1 < len)
    {
        ulen = utf8_len(str + i, len - i);
        if (ulen == 3)
        {
            count += ___chinese_word_find((unsigned char *)str + i);
        }
        else if(ulen == 2)
        {
            count += ___chinese_word_find2((unsigned char *)str + i);
        }
        i += ulen;
    }

    return count;
}

int zcharset_detect_chinese(char *data, int len, char *charset_ret)
{
    int ret;
    char *cs[] = { "GB18030", "BIG5", "UTF-8", 0 }, **csp, *cc, *fromcode;
    char out_string[10250];
    int out_len = 10240, w_count_max = 0, w_count;

    csp = cs;
    cc = 0;
    for (fromcode = *csp; fromcode; csp++, fromcode = *csp)
    {
        ZICONV_CREATE(ic);
        ic->from_charset = fromcode;
        ic->to_charset = "UTF-8";
        ic->in_str = (char *)data;
        ic->in_len = len;
        ic->filter = out_string;
        ic->filter_type = out_len;
        ret = zcharset_iconv(ic);
        ZICONV_FREE(ic);
        if (ret < 1)
        {
            continue;
        }
        w_count = ___chinese_word_count(out_string, ret);
        if ((w_count > w_count_max))
        {
            w_count_max = w_count;
            cc = fromcode;
        }
    }
    if (!cc)
    {
        return -1;
    }

    strcpy(charset_ret, cc);

    return 0;
}
