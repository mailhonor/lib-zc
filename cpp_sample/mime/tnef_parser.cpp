/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-11
 * ================================
 */

#include "zcc/zcc_mime.h"

static int enable_att = 0;
static const char *name_prefix = "";
static int idx = 0;

static int save_att(zcc::tnef_parser *parser, zcc::tnef_parser::mime_node *mime)
{
    const char *sname;
    char tmpname[256];

    idx++;
    sname = mime->get_show_name().c_str();
    if (zcc::empty(sname))
    {
        std::sprintf(tmpname, "%s%d_unknown.dat", name_prefix, idx);
        if (mime->get_content_type() == "text/rtf")
        {
            std::sprintf(tmpname, "%s%d_unknown.rtf", name_prefix, idx);
        }
        else if (mime->get_content_type() == "text/html")
        {
            std::sprintf(tmpname, "%s%d_unknown.html", name_prefix, idx);
        }
        else if (mime->get_content_type() == "text/plain")
        {
            std::sprintf(tmpname, "%s%d_unknown.txt", name_prefix, idx);
        }
    }
    else
    {
        std::snprintf(tmpname, 255, "%s%d_%s", name_prefix, idx, sname);
    }
    std::string pathname = "att/";
    pathname.append(zcc::format_filename(tmpname));

    std::printf("save attachment %s\n", pathname.c_str());
    if (zcc::file_put_contents(pathname.c_str(), mime->get_body_data(), mime->get_body_size()) < 1)
    {
        std::printf("ERROR decode_mime_body: save\n");
    }
    return 0;
}

static int save_all_attachments(zcc::tnef_parser *parser)
{
    auto &allm = parser->get_all_mimes();
    for (auto it = allm.begin(); it != allm.end(); it++)
    {
        save_att(parser, *it);
    }
    return 0;
}

static void do_parse(const char *eml_fn)
{
    zcc::tnef_parser *parser = zcc::tnef_parser::create_from_file(eml_fn, 0);
    if (parser == 0)
    {
        std::printf("ERROR open %s\n", eml_fn);
        zcc::exit(1);
    }
    parser->debug_show();
    if (enable_att)
    {
        save_all_attachments(parser);
    }
    delete parser;
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    enable_att = zcc::var_main_config.get_bool("att");
    int times = zcc::var_main_config.get_int("loog", 1);

    if (zcc::main_argument::var_parameters.size() == 0)
    {
        std::printf("USAGE: %s [--att] [ -loop 1] eml_fn...\n", zcc::progname);
        zcc::exit(0);
    }

    if (times < 2)
    {
        for (const char *fn : zcc::main_argument::var_parameters)
        {
            do_parse(fn);
        }
    }
    else
    {
        std::string con = zcc::file_get_contents(zcc::main_argument::var_parameters[0]);
        const char *data = con.data();
        int64_t len = con.size();

        std::printf("eml     : %s\n", zcc::main_argument::var_parameters[0]);
        std::printf("size    : %zd(bytes)\n", len);
        std::printf("loop    : %d\n", times);
        int64_t t = zcc::millisecond();
        for (int i = 0; i < times; i++)
        {
            delete (zcc::tnef_parser::create_from_data(data, len, 0));
        }
        t = zcc::millisecond() - t;
        std::printf("elapse  : %zd.%03zd(second)\n", t / 1000, t % 1000);
        std::printf("%%second : %lf(bytes)\n", (len * times) / (((1.0 * t) / 1000) * 1024 * 1024));
    }

    return 0;
}
