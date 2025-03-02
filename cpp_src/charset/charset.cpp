/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_charset.h"

zcc_namespace_begin;
zcc_general_namespace_begin(charset);

bool var_debug_mode = false;

const char *correct_name(const char *charset)
{
    char tmpcharset[32];
    int charsetlen = std::strlen(charset);
    if (charsetlen > 30)
    {
        return charset;
    }
    std::memcpy(tmpcharset, charset, charsetlen);
    tmpcharset[charsetlen] = 0;
    toupper(tmpcharset);

    static std::map<std::string, std::string> formats = {
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
        {"CSISO88598I", "ISO-8859-8"},
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
        {"ISO-8859-8-I", "ISO-8859-8"},
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
        {"LOGICAL", "ISO-8859-8"},
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
        {"X-GBK", "GB18030"},
        {"X-MAC-ROMAN", "MACINTOSH"},
        {"X-MAC-UKRAINIAN", "X-MAC-CYRILLIC"},
        {"X-SJIS", "SHIFT_JIS"},
        {"X-UNICODE20UTF8", "UTF-8"},
        {"X-X-BIG5", "BIG5"},
    };
    auto it = formats.find(tmpcharset);
    if (it != formats.end())
    {
        return it->second.c_str();
    }
    return charset;
}

static void _clear_null_inner(std::string &bf)
{
    const char *p = bf.c_str();
    uint64_t size = bf.size(), i;
    for (i = 0; i < size; i++)
    {
        if (p[i] == '\0')
        {
            bf[i] = ' ';
        }
    }
}

static bool check_is_7bit(const unsigned char *str, int len)
{
    for (int i = 0; i < len; i++)
    {
        unsigned c = str[i];
        if (c & 0X80)
        {
            return false;
        }
    }
    return true;
}

static void _convert_to_utf8(const char *from_charset, const char *data, int size, std::string &result)
{
    result.clear();
    if (size < 0)
    {
        size = std::strlen(data);
    }
    if (size < 1)
    {
        return;
    }
    std::string from_charset_buf;
    std::string charset;

    if (zcc::empty(from_charset))
    {
        if (check_is_7bit((const unsigned char *)data, size))
        {
            result.clear();
            result.append(data, size);
            _clear_null_inner(result);
            return;
        }
    }

    if (from_charset)
    {
        from_charset_buf = from_charset;
        zcc::tolower(from_charset_buf);
        from_charset = from_charset_buf.c_str();
    }

    const char *default_charset = "WINDOWS-1252";
    if (from_charset && (from_charset[0] == '?'))
    {
        default_charset = from_charset + 1;
        from_charset = 0;
    }

    std::string f_charset_buf;
    const char *f_charset;
    int detected = 0;

    f_charset = from_charset;
    if (zcc::empty(f_charset))
    {
        detected = 1;
        f_charset_buf = detect_cjk(data, size);
        if (!f_charset_buf.empty())
        {
            f_charset = f_charset_buf.c_str();
        }
        else
        {
            f_charset = default_charset;
        }
    }
    else
    {
        f_charset = correct_name(f_charset);
    }

    result = convert(f_charset, data, size, "UTF-8");
    if (!result.empty())
    {
        _clear_null_inner(result);
        return;
    }

    if (detected)
    {
        result.clear();
        return;
    }

    f_charset_buf = detect_cjk(data, size);
    if (!f_charset_buf.empty())
    {
        f_charset = f_charset_buf.c_str();
    }
    else
    {
        f_charset = default_charset;
    }

    result = convert(f_charset, data, size, "UTF-8");
    _clear_null_inner(result);
    return;
}

std::string convert_to_utf8(const char *from_charset, const char *data, int size)
{
    std::string r;
    _convert_to_utf8(from_charset, data, size, r);
    return r;
}

std::string (*convert_engine)(const char *from_charset, const char *src, int src_len, const char *to_charset, int *invalid_bytes) = iconv_convert;

zcc_general_namespace_end(charset);
zcc_namespace_end;
