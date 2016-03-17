/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-02
 * ================================
 */

#include "libzc.h"

/* should check c1 and c2 are hex */
#define ___hex_val(ccc) { ccc = zhex_to_dec_list[ccc];}

#define ___get_next_ch(c0123)    while(1){ \
    if(src_pos >= src_size){ goto over; } \
    c0123 = src_c[src_pos++]; \
    break; \
}

int zquoted_printable_decode_2045_to_df(void *src, int src_size, void *filter, int filter_type)
{
    unsigned char *src_c = src;
    int src_pos = 0;
    unsigned char *dest_result = (unsigned char *)filter;
    int len_result = 0;
    unsigned char c0, c1, c2;
    ZDATA_FILTER_BUF(filter, filter_type);

    while (1)
    {
        ___get_next_ch(c0);
        if (c0 != '=')
        {
            if (filter_type > 0)
            {
                if (len_result + 1 > filter_type)
                {
                    break;
                }
                dest_result[len_result++] = c0;
            }
            else
            {
                ZDATA_FILTER_PUTC(filter, c0);
                len_result++;
            }
            continue;
        }
        ___get_next_ch(c1);
        if (c1 == '\r' || c1 == '\n')
        {
            ___get_next_ch(c2);
            if (c2 != '\r' && c2 != '\n')
            {
                src_pos--;
            }
            continue;
        }
        ___get_next_ch(c2);
        ___hex_val(c1);
        ___hex_val(c2);
        if (filter_type > 0)
        {
            if (len_result + 1 > filter_type)
            {
                break;
            }
            dest_result[len_result++] = ((c1 << 4) | c2);
        }
        else
        {
            ZDATA_FILTER_PUTC(filter, ((c1 << 4) | c2));
            len_result++;
        }
    }
over:
    ZDATA_FILTER_FLUSH(filter);

    return len_result;
}

int zquoted_printable_decode_2047_to_df(void *src, int src_size, void *filter, int filter_type)
{
    unsigned char *src_c = src;
    int src_pos = 0;
    unsigned char *dest_result = (unsigned char *)filter;
    int len_result = 0;
    unsigned char c0, c1, c2;
    unsigned char addch;
    ZDATA_FILTER_BUF(filter, filter_type);

    while (1)
    {
        ___get_next_ch(c0);
        if (c0 == '_')
        {
            addch = ' ';
        }
        else if (c0 != '=')
        {
            addch = c0;
        }
        else
        {
            ___get_next_ch(c1);
            ___get_next_ch(c2);
            ___hex_val(c1);
            ___hex_val(c2);
            addch = (c1 << 4 | c2);
        }

        if (filter_type > 0)
        {
            if (len_result + 1 > filter_type)
            {
                break;
            }
            dest_result[len_result++] = addch;
        }
        else
        {
            ZDATA_FILTER_PUTC(filter, addch);
            len_result++;
        }
    }

over:
    ZDATA_FILTER_FLUSH(filter);

    return len_result;
}

int zquoted_printable_decode_validate(void *src, int src_size, int *valid_len)
{
    unsigned char *src_c = (unsigned char *)src;
    int i;
    unsigned char ch;

    for (i = 0; i < src_size; i++)
    {
        ch = src_c[i];
        if ((ch < 33) || (ch > 126))
        {
            if (valid_len)
            {
                *valid_len = i;
            }
            return -1;
        }
    }

    return 0;
}
