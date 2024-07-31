/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-09-07
 * ================================
 */

#include <cstdio>
#include "zcc/zcc_mime.h"

static const char *default_charset;
static const char *type;

static void usage()
{
    zcc_fatal("USAGE: %s [ -default-charset gb18030 ] [ -type general/address ]  header_file", zcc::progname);
}

static void (*show_decoded_line_utf8)(std::string &line) = 0;

static void _general_header_line(std::string &line)
{
    std::string r = zcc::mail_parser::header_line_get_utf8(default_charset, line);
    zcc_info("\n#########################");
    zcc_info("%s", r.c_str());
}

static void _address_header_line(std::string &line)
{
    auto pos = line.find(":");
    if (pos == std::string::npos)
    {
        return;
    }
    std::string prefix = line.substr(0, pos);
    zcc::tolower(prefix);
    if ((prefix != "from") && (prefix != "to") && (prefix != "cc"))
    {
        return;
    }
    std::string val = line.substr(pos + 1);
    std::vector<zcc::mail_parser::mail_address> address_vec = zcc::mail_parser::header_line_get_address_vector_utf8(default_charset, val);
    zcc_info("\n#########################");
    for (auto it = address_vec.begin(); it != address_vec.end(); it++)
    {
        auto &ma = *it;
        zcc_info("    %s <%s>    ###     %s", ma.name_utf8_.c_str(), ma.mail_.c_str(), ma.name_.c_str());
    }
}

static void header_line_walk_test(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        zcc_fatal("open %s", filename);
    }
    char buf[102400 + 10];
    std::string line;
    bool header_over = false;

    while (1)
    {
        if (header_over || ferror(fp) || feof(fp))
        {
            break;
        }
        line.clear();
        bool have_data = false;
        while (1)
        {
            size_t last_seek = ftell(fp);
            if (!fgets(buf, 102400, fp))
            {
                break;
            }
            if ((!std::strcmp(buf, "\n")) || (!std::strcmp(buf, "\r\n")))
            {
                header_over = true;
            }
            if (buf[0] == ' ' || buf[0] == '\t')
            {
                line.append(buf + 1);
                zcc::trim_line_end_rn(line);
                continue;
            }
            if (have_data)
            {
                fseek(fp, last_seek, SEEK_SET);
                break;
            }
            else
            {
                line.append(buf);
                zcc::trim_line_end_rn(line);
                have_data = true;
            }
        }
        show_decoded_line_utf8(line);
        line.clear();
    }
    if (!line.empty())
    {
        show_decoded_line_utf8(line);
    }

    fclose(fp);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() == 0)
    {
        usage();
    }
    default_charset = zcc::var_main_config.get_cstring("default-charset", "gb18030");
    type = zcc::var_main_config.get_cstring("type", "general");
    if (!std::strcmp(type, "address"))
    {
        show_decoded_line_utf8 = _address_header_line;
    }
    else
    {
        show_decoded_line_utf8 = _general_header_line;
    }

    auto &args = zcc::main_argument::var_parameters;
    for (auto it = args.begin(); it != args.end(); it++)
    {
        zcc_info("\n\n\n########################## %s", *it);
        header_line_walk_test((*it));
    }
    return 0;
}
