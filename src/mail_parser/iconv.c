/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "libzc.h"

int zmail_parser_iconv(zmail_parser_t * parser, char *from_charset, char *in, int in_len, void *filter, int filter_type)
{
    int ret = -1;
    char f_charset[128];
    int i, times = 2;

    if (ZEMPTY(from_charset))
    {
        times = 1;
    }
    for (i = 0;i < times;i++)
    {
        if (ZEMPTY(from_charset))
        {
            if (zcharset_detect_cjk(in, in_len, f_charset) < 0)
            {
                strcpy(f_charset, parser->default_src_charset);
            }
        }
        else
        {
            strncpy(f_charset, from_charset, 125);
        }
        ZICONV_CREATE(ic);
        ic->from_charset = f_charset;
        ic->to_charset = parser->default_dest_charset;
        ic->in_str = in;
        ic->in_len = in_len;
        ic->filter = filter;
        ic->filter_type = filter_type;

        ret = zcharset_iconv(ic);
        ZICONV_FREE(ic);
        if (ret >-1)
        {
            break;
        }
        from_charset = 0;
    }
    return ret;
}
