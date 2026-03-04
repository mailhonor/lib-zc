/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2026-01-31
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

static std::map<std::string, file_category_t> var_file_category_map;

file_category_t get_file_category_from_suffix(const std::string &suffix)
{
    if (suffix.empty())
    {
        return file_category_t::unknown;
    }
    auto lower_suffix = suffix;
    tolower(lower_suffix);

    auto it = var_file_category_map.find(suffix);
    if (it != var_file_category_map.end())
    {
        return it->second;
    }

    if ((suffix.size() == 3) && (suffix[0] == 'r'))
    {
        if (isdigit(suffix[1]) && isdigit(suffix[2]))
        {
            return file_category_t::archive;
        }
    }
    return file_category_t::unknown;
}

file_category_t get_file_category_from_filename(const std::string &filename)
{
    auto pos = filename.find_last_of(".");
    if (pos == std::string::npos)
    {
        return file_category_t::unknown;
    }
    return get_file_category_from_suffix(filename.substr(pos + 1));
}

file_category_t convert_string_to_file_category(const std::string &str)
{
    static const std::map<std::string, file_category_t> stc = {
        {"unknown", file_category_t::unknown},
        {"document", file_category_t::document},
        {"code", file_category_t::code},
        {"image", file_category_t::image},
        {"audio", file_category_t::audio},
        {"video", file_category_t::video},
        {"archive", file_category_t::archive},
        {"exe", file_category_t::exe},
        {"none", file_category_t::none},
    };
    auto it = stc.find(str);
    if (it != stc.end())
    {
        return it->second;
    }
    return file_category_t::unknown;
}

const std::string &convert_file_category_to_string(file_category_t ft)
{
    static std::map<file_category_t, std::string> cts = {
        {file_category_t::document, "document"},
        {file_category_t::code, "code"},
        {file_category_t::image, "image"},
        {file_category_t::audio, "audio"},
        {file_category_t::video, "video"},
        {file_category_t::archive, "archive"},
        {file_category_t::exe, "exe"},
        {file_category_t::none, "none"},
    };
    auto it = cts.find(ft);
    if (it != cts.end())
    {
        return it->second;
    }
    static std::string unknown_str = "unknown";
    return unknown_str;
}

void file_category_register(const std::string &suffix, file_category_t fc)
{
    auto s = suffix;
    tolower(s);
    var_file_category_map[s] = fc;
}

const std::string get_suffix_from_filename(const std::string &filename)
{
    auto pos = filename.find_last_of(".");
    if (pos == std::string::npos)
    {
        return "";
    }
    auto r = filename.substr(pos + 1);
    tolower(r);
    if (std::string::npos != r.find("/"))
    {
        return "";
    }
    if (std::string::npos != r.find("\\"))
    {
        return "";
    }
    return r;
}

static void file_category_init()
{
    std::map<file_category_t, std::vector<const char *>> qr;
    qr[file_category_t::document] = {
        "chm",    //
        "clda",   //
        "csv",    //
        "dat",    //
        "doc",    //
        "docm",   //
        "docx",   //
        "dot",    //
        "dotx",   //
        "dps",    //
        "dpt",    //
        "eml",    //
        "et",     //
        "ett",    //
        "htm",    //
        "html",   //
        "ics",    //
        "mht",    //
        "msg",    //
        "odp",    //
        "ods",    //
        "odt",    //
        "ofd",    //
        "pdf",    //
        "php",    //
        "phtm",   //
        "phtml",  //
        "pot",    //
        "ppa",    //
        "ppam",   //
        "pps",    //
        "ppt",    //
        "pptm",   //
        "pptx",   //
        "rtf",    //
        "shtm",   //
        "shtml",  //
        "text",   //
        "tnef",   //
        "txt",    //
        "vcf",    //
        "wps",    //
        "wpt",    //
        "xhtm",   //
        "xhtml",  //
        "xla",    //
        "xlam",   //
        "xls",    //
        "xlsb",   //
        "xlsm",   //
        "xlsx",   //
        "xlt",    //
        "xltm",   //
        "xltx",   //
        "xml",    //
        "xps",    //
        "zzzzzz", //
        ""};

    qr[file_category_t::image] = {
        "bie",    //
        "bmp",    //
        "dcx",    //
        "emf",    //
        "emz",    //
        "eps",    //
        "fax",    //
        "gif",    //
        "icon",   //
        "jp2",    //
        "jpeg",   //
        "jpg",    //
        "miff",   //
        "mono",   //
        "mtv",    //
        "pbm",    //
        "pcd",    //
        "pcx",    //
        "pgm",    //
        "pict",   //
        "pjpeg",  //
        "png",    //
        "ppm",    //
        "ps",     //
        "rad",    //
        "rgba",   //
        "rla",    //
        "rle",    //
        "sgi",    //
        "svg",    //
        "tif",    //
        "tiff",   //
        "viff",   //
        "webp",   //
        "wmf",    //
        "wmz",    //
        "zzzzzz", //
        ""};

    qr[file_category_t::video] = {
        "3gp",    //
        "asf",    //
        "ass",    //
        "avi",    //
        "divx",   //
        "dv",     //
        "f4v",    //
        "flv",    //
        "idx",    //
        "m2ts",   //
        "m2v",    //
        "m4v",    //
        "mkv",    //
        "mov",    //
        "mp4",    //
        "mpeg",   //
        "mpg",    //
        "mts",    //
        "ogv",    //
        "rm",     //
        "rmvb",   //
        "srt",    //
        "ssa",    //
        "sub",    //
        "ts",     //
        "vob",    //
        "vtt",    //
        "webm",   //
        "xvid",   //
        "zzzzzz", //
        ""};

    qr[file_category_t::audio] = {
        "aac",    //
        "aif",    //
        "aiff",   //
        "amr",    //
        "flac",   //
        "m4a",    //
        "mid",    //
        "midi",   //
        "mka",    //
        "mp3",    //
        "oga",    //
        "ogg",    //
        "opus",   //
        "pcm",    //
        "wav",    //
        "wma",    //
        "zzzzzz", //
        ""};

    qr[file_category_t::code] = {
        "bash",   //
        "c",      //
        "cc",     //
        "cfg",    //
        "conf",   //
        "cpp",    //
        "cxx",    //
        "erl",    //
        "go",     //
        "gradle", //
        "h",      //
        "hh",     //
        "hpp",    //
        "hxx",    //
        "ini",    //
        "java",   //
        "js",     //
        "json",   //
        "jsx",    //
        "kt",     //
        "kts",    //
        "lua",    //
        "php",    //
        "pl",     //
        "pm",     //
        "ps1",    //
        "py",     //
        "rb",     //
        "rs",     //
        "scala",  //
        "sh",     //
        "swift",  //
        "toml",   //
        "ts",     //
        "tsx",    //
        "yaml",   //
        "yml",    //
        "zzzzzz", //
        ""};
    qr[file_category_t::exe] = {
        "a",      //
        "bat",    //
        "bin",    //
        "com",    //
        "exe",    //
        "o",      //
        "pif",    //
        "vbs",    //
        "zzzzzz", //
        ""};
    qr[file_category_t::archive] = {
        "7z",     //
        "ace",    //
        "arj",    //
        "bfd",    //
        "bz",     //
        "bz2",    //
        "cab",    //
        "cpio",   //
        "deb",    //
        "gz",     //
        "img",    //
        "iso",    //
        "jar",    //
        "lha",    //
        "lz",     //
        "lzh",    //
        "lzip",   //
        "lzs",    //
        "pma",    //
        "rar",    //
        "rpm",    //
        "so",     //
        "tar",    //
        "tb2",    //
        "tbz",    //
        "tbz2",   //
        "tgz",    //
        "tlz",    //
        "txz",    //
        "udf",    //
        "uue",    //
        "xxe",    //
        "xz",     //
        "z",      //
        "zip",    //
        "zipx",   //
        "zzzzzz", //
        ""};

    for (auto &ii : qr)
    {
        auto fc = ii.first;
        for (auto &suffix : ii.second)
        {
            if (empty(suffix) || !strcmp(suffix, "zzzzzz"))
            {
                continue;
            }
            file_category_register(suffix, fc);
        }
    }
}

zcc_global_init(file_category_init());

zcc_namespace_end;
