/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-03-18
 * ================================
 */

#include "zc.h"
#include "charset_utf8.hpp"

int zvar_charset_debug = 0;
const char *zvar_charset_chinese[] = { "UTF-8", "GB18030", "BIG5", 0 };
const char *zvar_charset_japanese[] = { "UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", 0 };
const char *zvar_charset_korean[] = { "UTF-8", "KS_C_5601", "KS_C_5861", 0 };
const char *zvar_charset_cjk[] = { "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "KS_C_5601", "KS_C_5861", 0 };

static inline int utf8_len(char *buf, int len)
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

static inline unsigned long ___chinese_word_score(unsigned char *word, int ulen)
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

static double ___chinese_score(char *str, int len, int omit_invalid_bytes_count)
{
    int i = 0, ulen;
    unsigned long score = 0;
    unsigned long count = 0;;

    while (i + 1 < len) {
        ulen = utf8_len(str + i, len - i);
        if ((ulen == 2) || (ulen == 3)) {
            score += ___chinese_word_score((unsigned char *)str + i, ulen);
            count++;
        }
        i += ulen;
    }

    if (count == 0) {
        return 0;
    }

    if (zvar_charset_debug) {
        fprintf(stderr, "charset detact score:%ld, count:%ld, omit_invalid_bytes_count:%d\n", score, count, omit_invalid_bytes_count);
    }
    return ((double)score / (count + omit_invalid_bytes_count));
}

zbool_t zcharset_detect(const char *data, size_t len, char *charset_ret, const char **charset_list)
{
    size_t ret, i, max_i;
    const char **csp, *fromcode;
    char out_string[4096*3 + 16 + 10];
    int len_to_use, list_len;
    int omit_invalid_bytes_count;
    double result_score_list[1024], max_score;
    ZSTACK_BUF_FROM(zout_string, out_string, 4096*3 + 16);

    list_len = 0;
    len_to_use = (len>4096?4096:len);
    csp = charset_list;
    for (fromcode = *csp; fromcode; csp++, fromcode = *csp) {
        list_len++;
    }
    if (list_len > 1000) {
        list_len = 1000;
    }

    max_score = 0;
    max_i = -1;
    for (i = 0; i < list_len; i++) {
        zbuf_reset(zout_string);
        result_score_list[i] = 0;
        fromcode = charset_list[i];

        ret = zcharset_iconv(fromcode, data, len_to_use
                , "UTF-8", (char *)zout_string, Z_DF_ZBUF
                , 0
                , -1, &omit_invalid_bytes_count);

        if (ret < 0) {
            continue;
        }
        if (omit_invalid_bytes_count > 5) {
            //continue;
        }
        zbuf_terminate(zout_string);
        result_score_list[i] = ___chinese_score(out_string, ret, omit_invalid_bytes_count);
        if (max_score < result_score_list[i]) {
            max_i = i;
            max_score = result_score_list[i];
        }
        if (zvar_charset_debug) {
            fprintf(stderr, "charset detact, charset:%s, score:%f\n", fromcode, result_score_list[i]);
        }
    }

    if (max_i == -1) {
        return 0;
    }
    strcpy(charset_ret, charset_list[max_i]);

    return 1;
}
