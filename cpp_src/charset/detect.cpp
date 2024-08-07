/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-01-06
 * ================================
 */

#include "zcc/zcc_charset.h"

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
const char *japanese[] = {"UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "UTF-7", 0};
const char *korean[] = {"UTF-8", "EUC-KR", "UTF-7", 0};
const char *cjk[] = {"WINDOWS-1252", "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "EUC-KR", "UTF-7", 0};

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

static void _check_info(unsigned char *str, int len, int *is_7bit, int *is_maybe_utf7)
{
    int i = 0, c, have_plus = 0;
    int plus_count = 0, plus_error = 0;
    for (; i < len; i++)
    {
        c = str[i];
        if (c & 0X80)
        {
            return;
        }
        if (c == '+')
        {
            if (have_plus == 1)
            {
                plus_error = 1;
            }
            have_plus = 1;
            continue;
        }
        if (c == '-')
        {
            if (have_plus)
            {
                plus_count++;
            }
            have_plus = 0;
            continue;
        }
        if (c == '\n')
        {
            if (have_plus)
            {
                plus_count++;
            }
            have_plus = 0;
            continue;
        }
        if (c == '\r')
        {
            continue;
        }
        if (have_plus == 1)
        {
            if (var_base64_decode_table[c] == 0XFF)
            {
                break;
            }
        }
    }

    for (; i < len; i++)
    {
        c = str[i];
        if (c & 0X80)
        {
            break;
        }
    }
    if (i == len)
    {
        *is_7bit = 1;
        if ((plus_count > 0) && (plus_error < 1))
        {
            *is_maybe_utf7 = 1;
        }
    }
}

static int _check_is_not_1252(unsigned char *str, int len)
{
    int i = 0, c, have_plus = 0;
    int count = 0;
    for (; i < len; i++)
    {
        c = str[i];
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

static double chinese_get_score(detect_data *dd, const char *fromcode, const char *str, int len, int *valid_count, int omit_invalid_bytes_count)
{
    int i = 0, ulen;
    unsigned int score = 0, token_score;
    unsigned int count = 0;
    if (valid_count)
    {
        *valid_count = 0;
    }

    while (i + 1 < len)
    {
        ulen = utf8_len((const unsigned char *)str + i);
        if ((ulen == 2) || (ulen == 3))
        {
            token_score = chinese_word_score(dd, (unsigned char *)str + i, ulen);
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
    if (count + omit_invalid_bytes_count > 0)
    {
        r = ((double)score / (count + omit_invalid_bytes_count));
    }
    if (omit_invalid_bytes_count > 1)
    {
        r = r / 10.0;
    }

    mydebug("        # %-20s, score:%lu(%f), count:%lu, omit:%d", fromcode, score, r, count, omit_invalid_bytes_count);
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
    int ret, max_i, min_omit_invalid_bytes_count_i, min_omit_invalid_bytes_count;
    const char **csp, *fromcode;
    int len_to_use, list_len;
    double result_score, max_score;
    int converted_len, omit_invalid_bytes_count;
    int is_7bit = 0, is_maybe_utf7 = 0;
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

    _check_info((unsigned char *)(void *)data, size, &is_7bit, &is_maybe_utf7);

    if (is_7bit)
    {
        if (is_maybe_utf7 == 0)
        {
            mydebug("        # %-20s, ASCII, NOT UTF-7", "");
            return "ASCII";
        }
        mydebug("        # %-20s, ASCII, MAYBE UTF-7, continue", "");
    }
    else
    {
        is_not_windows1252 = _check_is_not_1252((unsigned char *)(void *)data, size);
    }

    std::string out_bf;
    max_score = -1;
    max_i = -1;
    min_omit_invalid_bytes_count = len_to_use * 2 + 100;
    min_omit_invalid_bytes_count_i = -1;
    mydebug("###########");
    for (i = 0; i < list_len; i++)
    {
        ret = 0;
        result_score = 0;
        fromcode = charset_list[i];
        if (is_maybe_utf7 == 0)
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
        ret = convert(fromcode, data, len_to_use, "UTF-8", out_bf, &converted_len, -1, &omit_invalid_bytes_count);
        if (ret < 0)
        {
            mydebug("        # %-20s, convert failure", fromcode);
            continue;
        }
        if (omit_invalid_bytes_count < min_omit_invalid_bytes_count)
        {
            min_omit_invalid_bytes_count = omit_invalid_bytes_count;
            min_omit_invalid_bytes_count_i = i;
        }
        if (omit_invalid_bytes_count > 2)
        {
            mydebug("        # %-20s, omit_invalid_bytes: %d", fromcode, omit_invalid_bytes_count);
            continue;
        }
        if (converted_len < 1)
        {
            mydebug("        # %-20s, converted_len < 1", fromcode);
            continue;
        }
        if (omit_invalid_bytes_count > 0)
        {
            if (len_to_use < 1024)
            {
                mydebug("        # %-20s, omit_invalid_bytes: %d, skip", fromcode, omit_invalid_bytes_count);
                continue;
            }
            if (len_to_use - converted_len > 6)
            {
                mydebug("        # %-20s, omit_invalid_bytes: %d, not tail", fromcode, omit_invalid_bytes_count);
                continue;
            }
        }
        result_score = chinese_get_score(dd, fromcode, out_bf.c_str(), ret, 0, omit_invalid_bytes_count);
        if (max_score < result_score)
        {
            max_i = i;
            max_score = result_score;
        }
    }

    if (max_i == -1)
    {
        if (min_omit_invalid_bytes_count_i == -1)
        {
            return "";
        }
        max_i = min_omit_invalid_bytes_count_i;
    }
    return charset_list[max_i];
}

std::string detect_1252(detect_data *dd, const char *data, int size)
{
    if (!dd)
    {
        dd = var_default_detect_data;
    }
    int i;
    int ret, max_i, valid_count, omit_invalid_bytes_count;
    const char **csp, *fromcode;
    int len_to_use, list_len;
    double result_score, max_score;
    int converted_len;
    int is_7bit = 0, is_maybe_utf7 = 0;

    list_len = 0;
    len_to_use = (size > 102400 ? 102400 : size);
    const char *charset_list[] = {"WINDOWS-1252", "UTF-8", "GB18030", "BIG5", 0};
    csp = charset_list;
    for (fromcode = *csp; fromcode; csp++, fromcode = *csp)
    {
        list_len++;
    }
    if (list_len > 1000)
    {
        list_len = 1000;
    }

    _check_info((unsigned char *)(void *)data, size, &is_7bit, &is_maybe_utf7);

    if (is_7bit)
    {
        if (is_maybe_utf7 == 0)
        {
            mydebug("        # %-20s, ASCII, NOT UTF-7", "");
            return "ASCII";
        }
        mydebug("        # %-20s, ASCII, MAYBE UTF-7, continue", "");
    }

    std::string out_bf;
    max_score = -1;
    max_i = -1;
    mydebug("###########");
    for (i = 0; i < list_len; i++)
    {
        ret = 0;
        result_score = 0;
        fromcode = charset_list[i];
        if (is_maybe_utf7 == 0)
        {
            if ((fromcode[0] == 'u') || (fromcode[0] == 'U'))
            {
                if ((fromcode[1] == 't') || (fromcode[1] == 'T'))
                {
                    if ((!std::strcmp(fromcode + 3, "7")) || (!std::strcmp(fromcode + 3, "-7")))
                    {
                        mydebug("        # %-20s, skip utf7", fromcode);
                        continue;
                    }
                }
            }
        }

        ret = convert(fromcode, data, len_to_use, "UTF-8", out_bf, &converted_len, 5, &omit_invalid_bytes_count);
        if (ret < 0)
        {
            mydebug("        # %-20s, convert failure", fromcode);
            continue;
        }
        if (omit_invalid_bytes_count > 2)
        {
            mydebug("        # %-20s, omit_invalid_bytes: %d", fromcode, omit_invalid_bytes_count);
            continue;
        }
        if (omit_invalid_bytes_count > 0)
        {
            if (len_to_use - converted_len > 6)
            {
                mydebug("        # %-20s, omit_invalid_bytes: %d, not tail", fromcode, omit_invalid_bytes_count);
                continue;
            }
        }
        if (converted_len < 1)
        {
            mydebug("        # %-20s, converted_len < 1", fromcode);
            continue;
        }
        result_score = chinese_get_score(dd, fromcode, out_bf.c_str(), ret, &valid_count, omit_invalid_bytes_count);
        if (valid_count < 3)
        {
            continue;
        }
        if (max_score < result_score)
        {
            max_i = i;
            max_score = result_score;
        }
    }

    if (max_i == -1)
    {
        return "";
    }
    return charset_list[max_i];
}

std::string detect_cjk(const char *data, int size)
{
    return detect(nullptr, cjk, data, size);
}

zcc_general_namespace_end(charset);
zcc_namespace_end;
