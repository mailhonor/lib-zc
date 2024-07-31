/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-08
 * ================================
 */

#include "zc.h"
#include <pthread.h>
#include <iconv.h>
#include <errno.h>
int zvar_charset_debug = 0;
int zvar_charset_uconv_mode = 0;

#define mydebug(fmt, args...) \
    if (zvar_charset_debug)   \
    {                         \
        zinfo(fmt, ##args);   \
    }

#define ZCHARSET_ICONV_ERROR_OPEN (-2016)

typedef struct charset_iconv_t charset_iconv_t;
struct charset_iconv_t
{
    char *to_charset;
    char *from_charset;
    unsigned char charset_regular : 1;
    int in_converted_len;
    int omit_invalid_bytes;
    int omit_invalid_bytes_count;
    iconv_t ic;
};

char *zcharset_correct_charset(const char *charset)
{
    char tmpcharset[32];
    int charsetlen = strlen(charset);
    if (charsetlen > 30)
    {
        return (char *)charset;
    }
    strcpy(tmpcharset, charset);
    zstr_toupper(tmpcharset);

    typedef struct
    {
        const char *from;
        const char *to;
    } correct_t;

    correct_t vector[] = {
        {"866", "IBM866"},
        {"ANSI_X3.4-1968", "WINDOWS-1252"},
        {"ARABIC", "ISO-8859-6"},
        {"ASCII", "WINDOWS-1252"},
        {"ASMO-708", "ISO-8859-6"},
        {"BIG5", "BIG5"},
        {"BIG5-HKSCS", "BIG5"},
        {"CHINESE", "GB18030"},
        {"CN-BIG5", "BIG5"},
        {"CP1250", "WINDOWS-1250"},
        {"CP1251", "WINDOWS-1251"},
        {"CP1252", "WINDOWS-1252"},
        {"CP1253", "WINDOWS-1253"},
        {"CP1254", "WINDOWS-1254"},
        {"CP1255", "WINDOWS-1255"},
        {"CP1256", "WINDOWS-1256"},
        {"CP1257", "WINDOWS-1257"},
        {"CP1258", "WINDOWS-1258"},
        {"CP819", "WINDOWS-1252"},
        {"CP866", "IBM866"},
        {"CSBIG5", "BIG5"},
        {"CSEUCKR", "EUC-KR"},
        {"CSEUCPKDFMTJAPANESE", "EUC-JP"},
        {"CSGB2312", "GB18030"},
        {"CSIBM866", "IBM866"},
        {"CSISO2022JP", "ISO-2022-JP"},
        {"CSISO2022KR", "ISO-2022-KR"},
        {"CSISO58GB231280", "GB18030"},
        {"CSISO88596E", "ISO-8859-6"},
        {"CSISO88596I", "ISO-8859-6"},
        {"CSISO88598E", "ISO-8859-8"},
        {"CSISO88598I", "ISO-8859-8-I"},
        {"CSISOLATIN1", "WINDOWS-1252"},
        {"CSISOLATIN2", "ISO-8859-2"},
        {"CSISOLATIN3", "ISO-8859-3"},
        {"CSISOLATIN4", "ISO-8859-4"},
        {"CSISOLATIN5", "WINDOWS-1254"},
        {"CSISOLATIN6", "ISO-8859-10"},
        {"CSISOLATIN9", "ISO-8859-15"},
        {"CSISOLATINARABIC", "ISO-8859-6"},
        {"CSISOLATINCYRILLIC", "ISO-8859-5"},
        {"CSISOLATINGREEK", "ISO-8859-7"},
        {"CSISOLATINHEBREW", "ISO-8859-8"},
        {"CSKOI8R", "KOI8-R"},
        {"CSKSC56011987", "EUC-KR"},
        {"CSMACINTOSH", "MACINTOSH"},
        {"CSSHIFTJIS", "SHIFT_JIS"},
        {"CSUNICODE", "UTF-16LE"},
        {"CYRILLIC", "ISO-8859-5"},
        {"DOS-874", "WINDOWS-874"},
        {"ECMA-114", "ISO-8859-6"},
        {"ECMA-118", "ISO-8859-7"},
        {"ELOT_928", "ISO-8859-7"},
        {"EUC-JP", "EUC-JP"},
        {"EUC-KR", "EUC-KR"},
        {"GB_2312-80", "GB18030"},
        {"GB2312", "GB18030"},
        {"GB_2312", "GB18030"},
        {"GB2312", "GB18030"},
        {"GBK", "GB18030"},
        {"GREEK8", "ISO-8859-7"},
        {"GREEK", "ISO-8859-7"},
        {"HEBREW", "ISO-8859-8"},
        {"HZ-GB-2312", "GB18030"},
        {"IBM819", "WINDOWS-1252"},
        {"IBM866", "IBM866"},
        {"ISO-10646-UCS-2", "UTF-16LE"},
        {"ISO-2022-CN-EXT", "ISO-2022-CN"},
        {"ISO8859-10", "ISO-8859-10"},
        {"ISO885910", "ISO-8859-10"},
        {"ISO_8859-1:1987", "WINDOWS-1252"},
        {"ISO-8859-11", "WINDOWS-874"},
        {"ISO8859-11", "WINDOWS-874"},
        {"ISO885911", "WINDOWS-874"},
        {"ISO8859-13", "ISO-8859-13"},
        {"ISO885913", "ISO-8859-13"},
        {"ISO8859-14", "ISO-8859-14"},
        {"ISO885914", "ISO-8859-14"},
        {"ISO_8859-15", "ISO-8859-15"},
        {"ISO8859-15", "ISO-8859-15"},
        {"ISO885915", "ISO-8859-15"},
        {"ISO-8859-1", "WINDOWS-1252"},
        {"ISO_8859-1", "WINDOWS-1252"},
        {"ISO8859-1", "WINDOWS-1252"},
        {"ISO88591", "WINDOWS-1252"},
        {"ISO_8859-2:1987", "ISO-8859-2"},
        {"ISO-8859-2", "ISO-8859-2"},
        {"ISO_8859-2", "ISO-8859-2"},
        {"ISO8859-2", "ISO-8859-2"},
        {"ISO88592", "ISO-8859-2"},
        {"ISO_8859-3:1988", "ISO-8859-3"},
        {"ISO-8859-3", "ISO-8859-3"},
        {"ISO_8859-3", "ISO-8859-3"},
        {"ISO8859-3", "ISO-8859-3"},
        {"ISO88593", "ISO-8859-3"},
        {"ISO_8859-4:1988", "ISO-8859-4"},
        {"ISO_8859-4", "ISO-8859-4"},
        {"ISO8859-4", "ISO-8859-4"},
        {"ISO88594", "ISO-8859-4"},
        {"ISO_8859-5:1988", "ISO-8859-5"},
        {"ISO_8859-5", "ISO-8859-5"},
        {"ISO8859-5", "ISO-8859-5"},
        {"ISO88595", "ISO-8859-5"},
        {"ISO_8859-6:1987", "ISO-8859-6"},
        {"ISO-8859-6-E", "ISO-8859-6"},
        {"ISO-8859-6-I", "ISO-8859-6"},
        {"ISO_8859-6", "ISO-8859-6"},
        {"ISO8859-6", "ISO-8859-6"},
        {"ISO88596", "ISO-8859-6"},
        {"ISO_8859-7:1987", "ISO-8859-7"},
        {"ISO_8859-7", "ISO-8859-7"},
        {"ISO8859-7", "ISO-8859-7"},
        {"ISO88597", "ISO-8859-7"},
        {"ISO_8859-8:1988", "ISO-8859-8"},
        {"ISO-8859-8-E", "ISO-8859-8"},
        {"ISO-8859-8-I", "ISO-8859-8-I"},
        {"ISO-8859-8", "ISO-8859-8"},
        {"ISO_8859-8", "ISO-8859-8"},
        {"ISO8859-8", "ISO-8859-8"},
        {"ISO88598", "ISO-8859-8"},
        {"ISO_8859-9:1989", "WINDOWS-1254"},
        {"ISO-8859-9", "WINDOWS-1254"},
        {"ISO_8859-9", "WINDOWS-1254"},
        {"ISO8859-9", "WINDOWS-1254"},
        {"ISO88599", "WINDOWS-1254"},
        {"ISO-IR-100", "WINDOWS-1252"},
        {"ISO-IR-101", "ISO-8859-2"},
        {"ISO-IR-109", "ISO-8859-3"},
        {"ISO-IR-110", "ISO-8859-4"},
        {"ISO-IR-126", "ISO-8859-7"},
        {"ISO-IR-127", "ISO-8859-6"},
        {"ISO-IR-138", "ISO-8859-8"},
        {"ISO-IR-144", "ISO-8859-5"},
        {"ISO-IR-148", "WINDOWS-1254"},
        {"ISO-IR-149", "EUC-KR"},
        {"ISO-IR-157", "ISO-8859-10"},
        {"ISO-IR-58", "GB18030"},
        {"KOI8", "KOI8-R"},
        {"KOI8_R", "KOI8-R"},
        {"KOI8-RU", "KOI8-U"},
        {"KOI8-U", "KOI8-U"},
        {"KOI", "KOI8-R"},
        {"KOREAN", "EUC-KR"},
        {"KS_C_5601", "EUC-KR"},
        {"KS_C_5601-1987", "EUC-KR"},
        {"KS_C_5601-1989", "EUC-KR"},
        {"KSC_5601", "EUC-KR"},
        {"KSC5601", "EUC-KR"},
        {"L1", "WINDOWS-1252"},
        {"L2", "ISO-8859-2"},
        {"L3", "ISO-8859-3"},
        {"L4", "ISO-8859-4"},
        {"L5", "WINDOWS-1254"},
        {"L6", "ISO-8859-10"},
        {"L9", "ISO-8859-15"},
        {"LATIN1", "WINDOWS-1252"},
        {"LATIN2", "ISO-8859-2"},
        {"LATIN3", "ISO-8859-3"},
        {"LATIN4", "ISO-8859-4"},
        {"LATIN5", "WINDOWS-1254"},
        {"LATIN6", "ISO-8859-10"},
        {"LOGICAL", "ISO-8859-8-I"},
        {"MACINTOSH", "MACINTOSH"},
        {"MAC", "MACINTOSH"},
        {"MS932", "SHIFT_JIS"},
        {"MS_KANJI", "SHIFT_JIS"},
        {"SHIFT-JIS", "SHIFT_JIS"},
        {"SHIFT_JIS", "SHIFT_JIS"},
        {"SJIS", "SHIFT_JIS"},
        {"SUN_EU_GREEK", "ISO-8859-7"},
        {"TIS-620", "WINDOWS-874"},
        {"UCS-2", "UTF-16LE"},
        {"UNICODE-1-1-UTF-7", "UTF-7"},
        {"UNICODE-1-1-UTF-8", "UTF-8"},
        {"UNICODE11UTF8", "UTF-8"},
        {"UNICODE20UTF8", "UTF-8"},
        {"UNICODEFEFF", "UTF-16LE"},
        {"UNICODEFFFE", "UTF-16BE"},
        {"UNICODE", "UTF-16LE"},
        {"US-ASCII", "WINDOWS-1252"},
        {"UTF-16BE", "UTF-16BE"},
        {"UTF-16LE", "UTF-16LE"},
        {"UTF7", "UTF-7"},
        {"UTF8", "UTF-8"},
        {"UTF8", "UTF-8"},
        {"VISUAL", "ISO-8859-8"},
        {"WINDOWS-31J", "SHIFT_JIS"},
        {"WINDOWS-874", "WINDOWS-874"},
        {"WINDOWS-949", "EUC-KR"},
        {"X-CP1250", "WINDOWS-1250"},
        {"X-CP1251", "WINDOWS-1251"},
        {"X-CP1252", "WINDOWS-1252"},
        {"X-CP1253", "WINDOWS-1253"},
        {"X-CP1254", "WINDOWS-1254"},
        {"X-CP1255", "WINDOWS-1255"},
        {"X-CP1256", "WINDOWS-1256"},
        {"X-CP1257", "WINDOWS-1257"},
        {"X-CP1258", "WINDOWS-1258"},
        {"X-EUC-JP", "EUC-JP"},
        {"X-GB18030", "GB18030"},
        {"X-MAC-CYRILLIC", "X-MAC-CYRILLIC"},
        {"X-MAC-ROMAN", "MACINTOSH"},
        {"X-MAC-UKRAINIAN", "X-MAC-CYRILLIC"},
        {"X-SJIS", "SHIFT_JIS"},
        {"X-UNICODE20UTF8", "UTF-8"},
        {"X-X-BIG5", "BIG5"},
        {0, 0}};
    static zdict_t *dt = 0;
    if (!dt)
    {
        static pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&locker);
        if (!dt)
        {
            dt = zdict_create();
            for (correct_t *c = vector; c->from; c++)
            {
                zdict_update_string(dt, c->from, c->to, -1);
            }
        }
        pthread_mutex_unlock(&locker);
    }
    zbuf_t *v = 0;
    if (!zdict_find(dt, tmpcharset, &v))
    {
        return (char *)charset;
    }
    return zbuf_data(v);
}

static inline int charset_iconv_base(charset_iconv_t *ic, char *_in_str, int _in_len, char *_out_s, int _out_l)
{
    char *in_str = _in_str;
    size_t in_len = (size_t)(_in_len);
    int in_converted_len = 0;

    char *out_str = _out_s;
    size_t out_len = (size_t)(_out_l);
    int out_converted_len = 0;

    int ic_ret;
    int errno2;
    char *in_str_o, *out_tmp;
    size_t out_len_tmp;
    int t_ilen, t_olen;

    ic->in_converted_len = 0;

    if (_in_len < 1)
    {
        return 0;
    }
    if (ic->omit_invalid_bytes_count > ic->omit_invalid_bytes)
    {
        return 0;
    }

    /* correct charset */
    if (!(ic->charset_regular))
    {
        if (zempty(ic->from_charset) || zempty(ic->to_charset))
        {
            return -1;
        }
        ic->charset_regular = 1;
        ic->to_charset = zcharset_correct_charset(ic->to_charset);
        ic->from_charset = zcharset_correct_charset(ic->from_charset);
    }

    /* */
    if (ic->ic == 0)
    {
        ic->ic = iconv_open(ic->to_charset, ic->from_charset);
    }

    if (ic->ic == (iconv_t)-1)
    {
        return ZCHARSET_ICONV_ERROR_OPEN;
    }

    /* do ic */
    while (in_len > 0)
    {
        in_str_o = in_str;
        out_tmp = out_str;
        out_len_tmp = out_len;

        ic_ret = iconv(ic->ic, &in_str, &in_len, &out_tmp, &out_len_tmp);
        t_ilen = in_str - in_str_o;
        in_converted_len += t_ilen;

        t_olen = out_tmp - out_str;
        out_str = out_tmp;
        out_len = out_len_tmp;
        out_converted_len += t_olen;

        if (ic_ret != (int)-1)
        {
            ic_ret = iconv(ic->ic, NULL, NULL, &out_tmp, &out_len_tmp);
            t_olen = out_tmp - out_str;
            out_str = out_tmp;
            out_len = out_len_tmp;
            out_converted_len += t_olen;
            break;
        }
        errno2 = zget_errno();
        if (errno2 == E2BIG)
        {
            break;
        }
        else if (errno2 == EILSEQ || errno2 == EINVAL)
        {
            if (in_str != _in_str)
            {
                break;
            }
            in_str++;
            in_len--;
            ic->omit_invalid_bytes_count++;
            if (ic->omit_invalid_bytes_count > ic->omit_invalid_bytes)
            {
                break;
            }
            in_converted_len += 1;
            continue;
        }
        else
        {
            break;
        }
    }

    ic->in_converted_len = in_converted_len;

    return out_converted_len;
}

int zcharset_iconv(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count)
{
    if (zvar_charset_uconv_mode)
    {
        zfatal("run zcharset_convert_use_uconv first");
    }
    charset_iconv_t ic_buf, *ic = &ic_buf;
    char buf[4910];
    int len;
    char *in_str;
    int in_len;
    int out_converted_len = 0;
    char *str_running;
    int len_running;
    zbuf_reset(dest);

    memset(ic, 0, sizeof(charset_iconv_t));
    ic->from_charset = (char *)(from_charset);
    ic->to_charset = (char *)(to_charset);
    if (omit_invalid_bytes_limit < 0)
    {
        ic->omit_invalid_bytes = (256 * 256 * 256 * 127 - 1);
    }
    else
    {
        ic->omit_invalid_bytes = omit_invalid_bytes_limit;
    }

    in_str = (char *)(src);
    in_len = src_len;
    if (in_len < 0)
    {
        in_len = strlen(src);
    }

    while (in_len > 0)
    {
        str_running = buf;
        len_running = 4096;
        len = charset_iconv_base(ic, in_str, in_len, str_running, len_running);
        if (len < 0)
        {
            out_converted_len = -1;
            break;
        }
        in_str += ic->in_converted_len;
        in_len -= ic->in_converted_len;

        if (len == 0)
        {
            break;
        }
        out_converted_len += len;
        zbuf_memcat(dest, buf, len);
    }

    if ((ic->ic) && (ic->ic != (iconv_t)-1))
    {
        iconv_close(ic->ic);
    }

    if (src_converted_len)
    {
        *src_converted_len = in_str - (char *)(src);
    }
    if (omit_invalid_bytes_count)
    {
        *omit_invalid_bytes_count = ic->omit_invalid_bytes_count;
    }

    return out_converted_len;
}
/*
 * iconv static lib, missing libiconv and GCONV_PATH mismatched.
 * 1, download latest libiconv;
 * 2, ./configure --enable-static=PKGS
 * 3, make
 * 4, ls lib/.libs/iconv.o lib/.libs/localcharset.o lib/.libs/relocatable.o
 */

int (*zcharset_convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count) = zcharset_iconv;
static void _clear_null_inner(zbuf_t *bf)
{
    char *p = zbuf_data(bf);
    int size = zbuf_len(bf), i;
    for (i = 0; i < size; i++)
    {
        if (p[i] == '\0')
        {
            p[i] = ' ';
        }
    }
}

extern char *zcharset_detect_1252(const char *data, int size, char *charset_result);

static int _mime_iconv_1252(const char *data, int size, zbuf_t *result)
{
    zbuf_reset(result);
    int omit_invalid_bytes_count = 0;

    const char *charset = "windows-1252";
    char f_charset_buf[zvar_charset_name_max_size + 1];
    charset = zcharset_detect_1252(data, size, f_charset_buf);
    if (!charset)
    {
        charset = "windows-1252";
    }

    if (zcharset_convert(charset, data, size,
                         "UTF-8", result, 0,
                         10, &omit_invalid_bytes_count) < 0)
    {
        return 0;
    }

    if (omit_invalid_bytes_count > 0)
    {
        return 0;
    }
    return 1;
}

void zcharset_convert_to_utf8(const char *from_charset, const char *data, int size, zbuf_t *result)
{
    if (size < 0)
    {
        size = strlen(data);
    }
    if (size < 1)
    {
        return;
    }

    if (from_charset)
    {
        if ((!strcasecmp(from_charset, "iso-8859-1")) || (!strcasecmp(from_charset, "windows-1252")))
        {
            if (_mime_iconv_1252(data, size, result))
            {
                return;
            }
            from_charset = 0;
        }
    }

    const char *default_charset = "WINDOWS-1252";
    if (from_charset && (from_charset[0] == '?'))
    {
        default_charset = from_charset + 1;
        from_charset = 0;
    }

    char f_charset_buf[zvar_charset_name_max_size + 1];
    const char *f_charset;
    int detected = 0;

    zbuf_reset(result);
    f_charset = from_charset;
    if (ZEMPTY(f_charset))
    {
        detected = 1;
        if (zcharset_detect_cjk(data, size, f_charset_buf))
        {
            f_charset = f_charset_buf;
        }
        else
        {
            f_charset = default_charset;
        }
    }
    else
    {
        f_charset = zcharset_correct_charset(f_charset);
    }

    if (zcharset_convert(f_charset, data, size,
                         "UTF-8", result, 0,
                         -1, 0) > 0)
    {
        _clear_null_inner(result);
        return;
    }

    if (detected)
    {
        zbuf_memcpy(result, data, size);
        _clear_null_inner(result);
        return;
    }

    if (zcharset_detect_cjk(data, size, f_charset_buf))
    {
        f_charset = f_charset_buf;
    }
    else
    {
        f_charset = default_charset;
    }
    zbuf_reset(result);
    if (zcharset_convert(f_charset, data, size,
                         "UTF-8", result, 0,
                         -1, 0) > 0)
    {
        _clear_null_inner(result);
        return;
    }

    zbuf_memcpy(result, data, size);
    _clear_null_inner(result);
    return;
}
