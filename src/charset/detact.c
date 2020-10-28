/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-01-06
 * ================================
 */

#include "zc.h"

#define mydebug          if (zvar_charset_debug)zinfo

#include "charset_utf8.h"

static inline __attribute__((always_inline)) int utf8_len(char *buf, int len)
{
    unsigned char *ptr;
    int ret;
    ptr = (unsigned char *)buf;
    if (((*ptr) <= 0x7F)) {
        ret = 1;
    } else if (((*ptr) & 0xF0) == 0xF0) {
        ret = 4;
    } else if (((*ptr) & 0xE0) == 0xE0) {
        ret = 3;
    } else if (((*ptr) & 0xC0) == 0xC0) {
        ret = 2;
    } else {
        ret = 5;
    }

    return ret;
}

static inline __attribute__((always_inline)) unsigned long chinese_word_score(unsigned char *word, int ulen)
{
    int start = 0, middle, end;
    unsigned int mint, wint;
    unsigned char *wp;
    unsigned char *wlist;

    if (ulen == 2) {
        wlist = (unsigned char *)utf8_list2;
        end = utf8_list_count2 - 1;
        wint = (word[0] << 8) | (word[1]);
    } else {
        wlist = (unsigned char *)(utf8_list3[(*word) & 0X0F]);
        end = utf8_list_count3[(*word) & 0X0F] - 1;
        wint = (word[1] << 8) | (word[2]);
    }

    while (1) {
        if (start > end) {
            return 0;
        }
        middle = (start + end) / 2;
        wp = wlist + middle * 3;
        mint = ((wp[0] << 8) | (wp[1]));

        if (wint < mint) {
            end = middle - 1;
            continue;
        }
        if (mint < wint) {
            start = middle + 1;
            continue;
        }
        return wp[2];
    }

    return 0;
}

static double chinese_get_score(const char *fromcode, char *str, int len, int omit_invalid_bytes_count)
{
    int i = 0, ulen;
    unsigned long score = 0;
    unsigned long count = 0;;

    while (i + 1 < len) {
        ulen = utf8_len(str + i, len - i);
        if ((ulen == 2) || (ulen == 3)) {
            score += chinese_word_score((unsigned char *)str + i, ulen);
            count++;
        }
        i += ulen;
    }

    if (count == 0) {
        return 0;
    }

    mydebug("        # %-20s, score:%lu, count:%lu, omit:%d" , fromcode, score, count, omit_invalid_bytes_count);
    return ((double)score / (count + omit_invalid_bytes_count));
}

char *zcharset_detect(const char **charset_list, const char *data, int size, zbuf_t *charset_result)
{
    int i;
    int ret, max_i;
    const char **csp, *fromcode;
    int len_to_use, list_len;
    double result_score, max_score;
    zbuf_t *out_bf = zbuf_create(1024);
    int converted_len, omit_invalid_bytes_count;
    zbuf_reset(charset_result);

    list_len = 0;
    len_to_use = (size>4096?4096:size);
    csp = charset_list;
    for (fromcode = *csp; fromcode; csp++, fromcode = *csp) {
        list_len++;
    }
    if (list_len > 1000) {
        list_len = 1000;
    }

    max_score = 0;
    max_i = -1;
    mydebug("###########");
    for (i = 0; i < list_len; i++) {
        result_score = 0;
        fromcode = charset_list[i];

        ret = zcharset_convert(fromcode, data, len_to_use, "UTF-8", out_bf, &converted_len, -1, &omit_invalid_bytes_count);
        if (ret < 0) {
            mydebug("        # %-20s, convert failure", fromcode);
            continue;
        }
        if (omit_invalid_bytes_count > 5) {
            mydebug("        # %-20s, omit_invalid_bytes: %d", fromcode, omit_invalid_bytes_count);
            continue;
        }
        if (converted_len < 1) {
            continue;
        }
        result_score = chinese_get_score(fromcode, zbuf_data(out_bf), ret, omit_invalid_bytes_count);
        if (max_score < result_score) {
            max_i = i;
            max_score = result_score;
        }
    }
    zbuf_free(out_bf);

    if (max_i == (ssize_t)-1) {
        return 0;
    }
    zbuf_puts(charset_result, charset_list[max_i]);

    return zbuf_data(charset_result);
}

char *zcharset_detect_cjk(const char *data, int size, zbuf_t *charset_result)
{
    return zcharset_detect(zvar_charset_cjk, data, size, charset_result);
}

/* ################################################################## */

const char *zvar_charset_chinese[] = { "UTF-8", "GB18030", "BIG5", "UTF-7", 0 };
const char *zvar_charset_japanese[] = { "UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "UTF-7", 0 };
const char *zvar_charset_korean[] = { "UTF-8", "KS_C_5601", "KS_C_5861", "UTF-7", 0 };
const char *zvar_charset_cjk[] = { "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "KS_C_5601", "KS_C_5861", "UTF-7", 0 };
