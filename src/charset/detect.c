/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-03-18
 * ================================
 */

#include "libzc.h"

int zcharset_detect(char *data, int len, char *charset_ret, char **charset_list)
{
    return -1;
}

/* ################################################################## */

char *zvar_charset_chinese[] = { "UTF-8", "GB18030", "BIG5", 0 };
char *zvar_charset_japanese[] = { "UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", 0 };
char *zvar_charset_korean[] = { "UTF-8", "KS_C_5601", "KS_C_5861", 0 };
char *zvar_charset_cjk[] = { "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "KS_C_5601", "KS_C_5861", 0 };
