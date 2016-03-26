/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-03-18
 * ================================
 */

#include "libzc.h"
#include "word_utf8.h"

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

static inline int ___chinese_word_find(unsigned char *word, int ulen)
{
    int start = 0, middle, end;
    unsigned int mint, wint;
    unsigned char *wp;
    unsigned char *wlist;

    if (ulen == 2)
    {
        wlist = (unsigned char *)utf8_list2;
        end = utf8_list_count2 - 1;
        wint = (word[0] << 8) | (word[1]);
    }
    else
    {
        wlist = (unsigned char *)(utf8_list3[(*word)&0X0F]);
        end = utf8_list_count3[(*word)&0X0F] - 1;
        wint = (word[1] << 8) | (word[2]);
    }

    while (1)
    {
        if (start > end)
        {
            return 0;
        }
        middle = (start + end) / 2;
        wp = wlist + middle * 2;
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
        if ((ulen == 2) || (ulen == 3))
        {
            count += ___chinese_word_find((unsigned char *)str + i, ulen);
        }
        i += ulen;
    }

    return count;
}

int zcharset_detect(char *data, int len, char *charset_ret, char **charset_list)
{
    int ret;
    char **csp, *cc, *fromcode;
    char out_string[10250];
    int out_len = 10230, w_count_max = 0, w_count;
    int result_len_list[1024], list_len, max_len, max_i, max_eq_count, i;

    list_len = 0;
    csp = charset_list;
    for (fromcode = *csp; fromcode; csp++, fromcode = *csp)
    {
        list_len++;
    }
    if (list_len > 1000)
    {
        list_len = 1000;
    }

    max_len = 0;
    max_eq_count = 0;
    for (i=0; i < list_len; i++)
    {
        fromcode = charset_list[i];

        ZICONV_CREATE(ic);
        ic->from_charset = fromcode;
        ic->to_charset = fromcode;
        ic->in_str = (char *)data;
        ic->in_len = len;
        ic->filter = out_string;
        ic->filter_type = out_len;
        ret = zcharset_iconv(ic);
        ZICONV_FREE(ic);

        result_len_list[i] = ret;
        if (ret > max_len)
        {
            max_len = ret;
            max_eq_count = 1;
            max_i = i;
        }
        if (ret == max_len)
        {
            max_eq_count ++;
        }
    }

    if (max_len == 0)
    {
        return -1;
    }

    if (max_eq_count == 1)
    {
        strcpy(charset_ret, charset_list[max_i]);
        return 0;
    }

    cc = 0;
    for (i=0; i < list_len; i++)
    {
        if (result_len_list[i] != max_len)
        {
            continue;
        }
        fromcode = charset_list[i];
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

/* ################################################################## */

char *zvar_charset_chinese[] = { "UTF-8", "GB18030", "BIG5", 0 };
char *zvar_charset_japanese[] = { "UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", 0 };
char *zvar_charset_korean[] = { "UTF-8", "KS_C_5601", "KS_C_5861", 0 };
char *zvar_charset_cjk[] = { "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "KS_C_5601", "KS_C_5861", 0 };
