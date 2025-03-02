/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-01-06
 * ================================
 */

#include "zcc/zcc_charset.h"
#include <cmath>

#define mydebug         \
    if (var_debug_mode) \
    zcc_info

zcc_namespace_begin;
zcc_general_namespace_begin(charset);

#pragma pack(push, 8)
#include "./char_score.h"
#pragma pack(pop)

detect_data *var_default_detect_data = &___detect_data;

const char *chinese[] = {"UTF-8", "GB18030", "BIG5", "UTF-7", 0};
const char *japanese[] = {"UTF-8", "EUC-JP", "SHIFT-JIS", "ISO-2022-JP", "UTF-7", 0};
const char *korean[] = {"UTF-8", "EUC-KR", "UTF-7", 0};
const char *cjk[] = {"WINDOWS-1252", "UTF-8", "GB18030", "BIG5", "EUC-JP", "SHIFT-JIS", "ISO-2022-JP", "EUC-KR", "UTF-7", 0};

static inline int chinese_word_score(detect_data *dd, unsigned char *word, int ulen)
{
    int start = 0, middle, end;
    unsigned int mint, wint;
    unsigned char *wp;
    unsigned char *wlist;

    if (ulen == 2)
    {
        wlist = (unsigned char *)(dd->data2);
        end = dd->count2 - 1;
        wint = (word[0] << 8) | (word[1]);
    }
    else
    {
        wlist = (unsigned char *)((dd->data3)[(*word) & 0X0F]);
        end = (dd->count3)[(*word) & 0X0F] - 1;
        wint = (word[1] << 8) | (word[2]);
    }

    while (1)
    {
        if (start > end)
        {
            return 0;
        }
        middle = (start + end) / 2;
        wp = wlist + middle * 5;
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
        int r = wp[2];
        r = (r << 8) + wp[3];
        r = (r << 8) + wp[4];
        return r;
    }

    return 0;
}

static void _check_info(unsigned char *str, int len, int *is_7bit)
{
    *is_7bit = 0;
    int i = 0, c, have_plus = 0;
    int plus_count = 0, plus_error = 0;
    for (; i < len; i++)
    {
        c = str[i];
        if (c & 0X80)
        {
            return;
        }
    }

    if (i == len)
    {
        *is_7bit = 1;
    }
}

static int _check_is_not_1252(unsigned char *str, int len)
{
    if (len < 4)
    {
        return 1;
    }
    int count = 0;
    for (int i = 0; i < len; i++)
    {
        int c = str[i];
        if (c & 0X80)
        {
            count++;
            if (count > 3)
            {
                return 1;
            }
        }
        else
        {
            count = 0;
        }
    }
    return 0;
}

static double chinese_get_score(detect_data *dd, const char *fromcode, const char *str, int len, int *valid_count, int invalid_bytes)
{
    int i = 0, ulen;
    uint64_t score = 0, token_score;
    uint64_t count = 0;
    if (valid_count)
    {
        *valid_count = 0;
    }

    while (i + 1 < len)
    {
        ulen = utf8_len((const unsigned char *)str + i);
        if ((ulen == 2) || (ulen == 3))
        {
            token_score = (uint64_t)chinese_word_score(dd, (unsigned char *)str + i, ulen);
            // std::string r(str + i, ulen);
            // std::printf("SSSSSSSSSSSSSSSSSSSS:%d, %s", token_score, r.c_str());
            // for (int i = 0; i < ulen; i++)
            // {
            //     std::printf("= %d, ", r.c_str()[i]);
            // }
            // std::printf("\n");
            score += token_score;
            count++;
            if (token_score == 0)
            {
                count++;
            }
        }
        i += ulen;
    }

    auto r = 0.0;

    uint16_t a = count;
    if (invalid_bytes > 100)
    {
        a += invalid_bytes * 10;
    }
    else if (invalid_bytes > 20)
    {
        a += invalid_bytes * 7;
    }
    else if (invalid_bytes > 10)
    {
        a += invalid_bytes * 5;
    }
    else if (invalid_bytes > 3)
    {
        a += invalid_bytes * 3;
    }
    else
    {
        a += invalid_bytes * 2;
    }
    if (a > 0)
    {
        r = ((double)score / a);
    }

    mydebug("        # %-20s, score:%lu(%f), count:%lu, invalid_bytes:%d", fromcode, score, r, count, invalid_bytes);
    if (count == 0)
    {
        return 0;
    }
    if (valid_count)
    {
        *valid_count = count;
    }

    return r;
}

std::string detect(detect_data *dd, const char **charset_list, const char *data, int size)
{
    if (!dd)
    {
        dd = var_default_detect_data;
    }
    int i;
    int ret, max_i_invalid, max_i_no_invalid, min_invalid_bytes_i, min_invalid_bytes;
    const char **csp, *fromcode;
    int len_to_use, list_len;
    double result_score, max_score_invalid, max_score_no_invalid;
    int max_count, tmp_count, invalid_count;
    int invalid_bytes;
    int is_7bit = 0;
    int is_not_windows1252 = 0;

    list_len = 0;
    len_to_use = (size > 102400 ? 102400 : size);
    csp = charset_list;
    for (fromcode = *csp; fromcode; csp++, fromcode = *csp)
    {
        list_len++;
    }
    if (list_len > 1000)
    {
        list_len = 1000;
    }

    _check_info((unsigned char *)(void *)data, size, &is_7bit);

    if (is_7bit)
    {
        convert("UTF-7", data, len_to_use, "UTF-8", &invalid_bytes);
        if (invalid_bytes > 0)
        {
            mydebug(" 7bit, invalid_bytes: %d, not utf-7, return", invalid_bytes);
            return "ASCII";
        }
        mydebug(" 7bit, invalid_bytes: 0, maybe utf-7, return", invalid_bytes);
        return "UTF-7";
    }
    else
    {
        is_not_windows1252 = _check_is_not_1252((unsigned char *)(void *)data, size);
    }

    std::string out_bf;
    max_score_invalid = 0;
    max_score_no_invalid = 0;
    max_i_invalid = -1;
    max_i_no_invalid = -1;
    min_invalid_bytes = len_to_use * 2 + 100;
    min_invalid_bytes_i = -1;
    max_count = 0;
    invalid_count = 0;
    mydebug("###########");
    for (i = 0; i < list_len; i++)
    {
        ret = 0;
        result_score = 0;
        fromcode = charset_list[i];
        if (is_7bit == 0)
        {
            if (!strcmp(fromcode, "UTF-7"))
            {
                mydebug("        # %-20s, skip utf7", fromcode);
                continue;
            }
        }

        if (is_not_windows1252)
        {
            if (!strcmp(fromcode, "WINDOWS-1252"))
            {
                mydebug("        # %-20s, skip windows-1252", fromcode);
                continue;
            }
        }
        out_bf = convert(fromcode, data, len_to_use, "UTF-8", &invalid_bytes);
        if (ret < 0)
        {
            mydebug("        # %-20s, convert failure", fromcode);
            continue;
        }
        if (invalid_bytes < min_invalid_bytes)
        {
            min_invalid_bytes = invalid_bytes;
            min_invalid_bytes_i = i;
        }
        result_score = chinese_get_score(dd, fromcode, out_bf.c_str(), (int)out_bf.size(), &tmp_count, invalid_bytes);
        if (invalid_bytes == 0)
        {
            if (max_score_no_invalid < result_score)
            {
                max_i_no_invalid = i;
                max_score_no_invalid = result_score;
            }
        }
        else
        {
            if (max_score_invalid < result_score)
            {
                max_i_invalid = i;
                max_score_invalid = result_score;
                max_count = tmp_count;
                invalid_count = invalid_bytes;
            }
        }
    }
    if (max_i_no_invalid > -1)
    {
        return charset_list[max_i_no_invalid];
    }
    if (max_i_invalid > -1)
    {
        if (max_count > invalid_count * 3)
        {
            return charset_list[max_i_invalid];
        }
        return "WINDOWS-1252";
    }

    if (min_invalid_bytes_i == -1)
    {
        return "";
    }
    return charset_list[min_invalid_bytes_i];
}

std::string detect_cjk(const char *data, int size)
{
    return detect(nullptr, cjk, data, size);
}

zcc_general_namespace_end(charset);
zcc_namespace_end;
