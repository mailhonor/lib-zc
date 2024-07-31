/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-12-09
 * ================================
 */

#include "zcc/zcc_charset.h"

static void ___usage()
{
    zcc_fatal("USAGE: %s [ -match xxx ] -dump dump_fn dir/file [ ... dir/file [ ...] ]", zcc::progname);
}

static const char *utf16_fragments = "0100 0180 0250 02B0 0300 0370 0400 0500 0530 0590 0600 0700 0750 0780 07C0 0800 0840 0860 0870 08A0 0900 0980 0A00 0A80 0B00 0B80 0C00 0C80 0D00 0D80 0E00 0E80 0F00 1000 10A0 1100 1200 1380 13A0 1400 1680 16A0 1700 1720 1740 1760 1780 1800 18B0 1900 1950 1980 19E0 1A00 1A20 1AB0 1B00 1B80 1BC0 1C00 1C50 1C80 1C90 1CC0 1CD0 1D00 1D80 1DC0 1E00 1F00 2000 2070 20A0 20D0 2100 2150 2190 2200 2300 2400 2440 2460 2500 2580 25A0 2600 2700 27C0 27F0 2800 2900 2980 2A00 2B00 2C00 2C60 2C80 2D00 2D30 2D80 2DE0 2E00 2E80 2F00 2FE0 2FF0 3000 3040 30A0 3100 3130 3190 31A0 31C0 31F0 3200 3300 3400 4DC0 4E00 4f00 5000 5100 5200 5300 5400 5500 5600 5700 5800 5900 5A00 5B00 5C00 5D00 5E00 5F00 6100 6200 6300 6400 6500 6600 6700 6800 6900 6A00 6B00 6C00 6D00 6E00 6F00 7100 7200 7300 7400 7500 7600 7700 7800 7900 7A00 7B00 7C00 7D00 7E00 7F00 8100 8200 8300 8400 8500 8600 8700 8800 8900 8A00 8B00 8C00 8D00 8E00 8F00 9100 9200 9300 9400 9500 9600 9700 9800 9900 9A00 9B00 9C00 9D00 9E00 9F00 A100 A200 A300 A400 A500 A600 A700 A800 A900 AA00 AB00 AC00 AD00 AE00 AF00 B100 B200 B300 B400 B500 B600 B700 B800 B900 BA00 BB00 BC00 BD00 BE00 BF00 C100 C200 C300 C400 C500 C600 C700 C800 C900 CA00 CB00 CC00 CD00 CE00 CF00 D100 D200 D300 D400 D500 D600 D700 D800 D900 DA00 DB00 DC00 DD00 DE00 DF00 E100 E200 E300 E400 E500 E600 E700 E800 E900 EA00 EB00 EC00 ED00 EE00 EF00 F100 F200 F300 F400 F500 F600 F700 F800 F900 FA00 FB00 FC00 FD00 FE00 FF00 A000 A490 A4D0 A500 A640 A6A0 A700 A720 A800 A830 A840 A880 A8E0 A900 A930 A960 A980 A9E0 AA00 AA60 AA80 AAE0 AB00 AB30 AB70 ABC0 AC00 D7B0 D800 DB80 DC00 E000 F900 FB00 FB50 FE00 FE10 FE20 FE30 FE50 FE70 FF00 FFF0 10000 10080 10100 10140 10190 101D0 10200 10280 102A0 102E0 10300 10330 10350 10380 103A0 103E0 10400 10450 10480 104B0 10500 10530 10570 105C0 10600 10780 107C0 10800 10840 10860 10880 108B0 108E0 10900 10920 10940 10980 109A0 10A00 10A60 10A80 10AA0 10AC0 10B00 10B40 10B60 10B80 10BB0 10C00 10C50 10C80 10D00 10D40 10E60 10E80 10EC0 10F00 10F30 10F70 10FB0 10FE0 11000 11080 110D0 11100 11150 11180 111E0 11200 11250 11280 112B0 11300 11380 11400 11480 114E0 11580 11600 11660 11680 116D0 11700 11750 11800 11850 118A0 11900 11960 119A0 11A00 11A50 11AB0 11AC0 11B00 11B60 11C00 11C70 11CC0 11D00 11D60 11DB0 11EE0 11F00 11F60 11FB0 11FC0 12000 12400 12480 12550 12F90 13000 13430 13460 14400 14680 16800 16A40 16A70 16AD0 16B00 16B90 16E40 16EA0 16F00 16FA0 16FE0 17000 18800 18B00 18D00 18D80 1AFF0 1B000 1B100 1B130 1B170 1B300 1BC00 1BCA0 1BCB0 1CF00 1CFD0 1D000 1D100 1D200 1D250 1D2C0 1D2E0 1D300 1D360 1D380 1D400 1D800 1DAB0 1DF00 1E000 1E030 1E090 1E100 1E150 1E290 1E2C0 1E300 1E4D0 1E500 1E7E0 1E800 1E8E0 1E900 1E960 1EC70 1ECC0 1ED00 1ED50 1EE00 1EF00 1F000 1F030 1F0A0 1F100 1F200 1F300 1F600 1F650 1F680 1F700 1F780 1F800 1F900 1FA00 1FA70 1FB00 1FC00 20000 2A6E0 2A700 2B740 2B820 2CEB0 2EBF0 2EE60 2F800 2FA20 30000 31350 323B0 E0000 E0080 E0100 E01F0 F0000";

static std::map<int, int> data2;
static int count2;

static std::map<int, int> data3[16];
static int count3[16];

struct lang_info
{
    int sample_count{0};
    double scale{0.0};
};
typedef std::map<int64_t, lang_info> langs_info;
static langs_info global_langs_info;

typedef std::map<int64_t, int /* count */> chars_info;
static chars_info global_chars_info;

static std::string char_number_tostring(int char_number)
{
    int k = char_number;
    char buf[16 + 1];
    int i = 16;
    buf[i] = 0;
    for (; k;)
    {
        i--;
        buf[i] = (k & 0XFF);
        k = k >> 8;
    }
    return buf + i;
}

static lang_info *get_lang_info_by_utf8int(int n)
{
    auto it = global_langs_info.lower_bound(n);
    if (it == global_langs_info.end())
    {
        return nullptr;
    }
    return &(it->second);
}

static void build_langs_info_data()
{
    auto s = zcc::split(utf16_fragments, ' ');
    for (auto it = s.begin(); it != s.end(); it++)
    {
        lang_info li;
        int ncr = 0;
        unsigned char *ps = (unsigned char *)(it->c_str());
        if (*ps == '0')
        {
            ps++;
        }
        if (*ps == '0')
        {
            ps++;
        }
        for (; *ps; ps++)
        {
            int n = *ps;
            if (n <= '9')
            {
                n = n - '0';
            }
            else if (n <= 'F')
            {
                n = n - 'A' + 10;
            }
            ncr = (ncr << 4) + n;
        }
        unsigned char buf[32 + 1];
        int len = zcc::ncr_decode(ncr, (char *)buf);
        int64_t key = 0;
        for (int i = 0; i < len; i++)
        {
            key = (key << 8) + buf[i];
        }
        key--;
        global_langs_info[key] = li;
        buf[len] = 0;
    }
}

static chars_info get_chars_info_from_one_sample(const char *data, int64_t size)
{
    const unsigned char *ps = (const unsigned char *)data;
    chars_info r;
    int64_t left = size;
    while (left > 0)
    {
        int len = zcc::charset::utf8_len(ps);
        if (len > left)
        {
            break;
        }
        if (len < 2 || len > 3)
        {
            left -= len;
            ps += len;
            continue;
        }

        int64_t key = 0;
        for (int i = 0; i < len; i++)
        {
            key = (key << 8) + ps[i];
        }
        auto it = r.find(key);
        if (it == r.end())
        {
            r[key] = 1;
        }
        else
        {
            it->second++;
        }
        left -= len;
        ps += len;
    }
    return r;
}

static chars_info get_chars_info_from_one_file(const char *fn)
{
    std::string con = zcc::file_get_contents_sample(fn);
    return get_chars_info_from_one_sample(con.c_str(), con.size());
}

static std::map<lang_info *, int> get_langs_from_chars_info(chars_info &cinfo)
{
    std::map<lang_info *, int> r;
    for (auto it = cinfo.begin(); it != cinfo.end(); it++)
    {
        lang_info *li = get_lang_info_by_utf8int(it->first);
        if (!li)
        {
            continue;
        }
        auto it2 = r.find(li);
        if (it2 == r.end())
        {
            r[li] = 1;
        }
        else
        {
            it2->second++;
        }
    }
    return r;
}

static void compute_langs_info_scale(int all_count = 65536 * 64)
{
    for (auto it = global_langs_info.begin(); it != global_langs_info.end(); it++)
    {
        lang_info &info = it->second;
        if (info.sample_count < 1)
        {
            info.scale = 1;
        }
        else
        {
            info.scale = all_count * 1.0 / info.sample_count;
        }
    }
}

static void build_chars_map_one(int char_number, int count)
{
    lang_info *li = get_lang_info_by_utf8int(char_number);
    if (!li)
    {
        return;
    }
    int score = (li->scale * count);
    if (score < 1)
    {
        return;
    }
    if (char_number < 65536)
    {
        data2[char_number] = score;
        return;
    }
    int nc = char_number & (0XFFFF);
    int prefix = (char_number >> 16) & 0X0F;
    data3[prefix][nc] = score;
}

static void build_chars_score_map()
{
    for (auto it = global_chars_info.begin(); it != global_chars_info.end(); it++)
    {
        build_chars_map_one(it->first, it->second);
    }
    count2 = data2.size();
    for (int i = 0; i < 16; i++)
    {
        count3[i] = data3[i].size();
    }
}

static int _get_log(int64_t a)
{
    int i = 1;
    int k = 0;
    for (i = 1; i < a; i = i * 2)
    {
        k++;
    }
    if (k == 0)
    {
        k = 1;
    }
    return k;
}

static void load_one_file(const char *fn)
{
    chars_info cinfo = get_chars_info_from_one_file(fn);
    for (auto it = cinfo.begin(); it != cinfo.end(); it++)
    {
        int64_t ch = it->first;
        int count = it->second;
        count = _get_log(count);
        count = 1;
        auto it2 = global_chars_info.find(ch);
        if (it2 == global_chars_info.end())
        {
            global_chars_info[ch] = count;
        }
        else
        {
            it2->second += count;
        }
    }

    std::map<lang_info *, int> langs = get_langs_from_chars_info(cinfo);
    for (auto it = langs.begin(); it != langs.end(); it++)
    {
        it->first->sample_count++;
    }
}

static std::string build_include_data_one_char(int c)
{
    unsigned char dec2hex[18] = "0123456789abcdef";
    unsigned char ch = c & 0XFF;
    std::string r;
    r.append("\\x");
    r.push_back(dec2hex[ch >> 4]);
    r.push_back(dec2hex[ch & 0X0F]);
    return r;
}

static std::string build_include_data_one_data(const std::map<int, int> &char_score_map)
{
    int count = 0;
    std::string r;
    r.append("\"");
    for (auto it = char_score_map.begin(); it != char_score_map.end(); it++)
    {
        int cn = it->first;
        int score = it->second;
        r.append(build_include_data_one_char(cn >> 8));
        r.append(build_include_data_one_char(cn));
        r.append(build_include_data_one_char(score >> 16));
        r.append(build_include_data_one_char(score >> 8));
        r.append(build_include_data_one_char(score));
        if (count2++ > 1000)
        {
            count2 = 0;
            r.append("\"\n\"");
        }
    }
    r.append("\"");
    return r;
}

static std::string build_include_data()
{
    std::string r;
    r.append("static detect_data ___detect_data = {\n");
    r.append(build_include_data_one_data(data2)).append(",\n");
    r.append(std::to_string(data2.size())).append(",\n");
    r.append("{\n");
    for (int i = 0; i < 16; i++)
    {
        r.append(build_include_data_one_data(data3[i])).append(",\n");
    }
    r.append("},\n");
    r.append("{\n");
    for (int i = 0; i < 16; i++)
    {
        r.append(std::to_string(data3[i].size())).append(",\n");
    }
    r.append("},\n");
    r.append("};\n");
    r.append("\n");
    return r;
}

struct detect_data
{
    const char *data2;
    int count2;
    const char *data3[16];
    int count3[16];
};

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    const char *dump_fn = zcc::var_main_config.get_cstring("dump");
    const char *match = zcc::var_main_config.get_cstring("match");

    build_langs_info_data();
    std::vector<std::string> fs = zcc::find_file_sample(zcc::main_argument::var_parameters, match);
    for (auto it = fs.begin(); it != fs.end(); it++)
    {
        load_one_file(it->c_str());
    }
    compute_langs_info_scale();
    build_chars_score_map();

    std::string r = build_include_data();
    if (!zcc::empty(dump_fn))
    {
        if (zcc::file_put_contents(dump_fn, r) < 1)
        {
            zcc_error_and_exit("write file: %s", dump_fn);
        }
    }
    return 0;
}
