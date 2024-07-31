/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-11
 * ================================
 */

#include "zcc/zcc_mime.h"


static bool enable_att = 0;
static const char *name_prefix = "";

static int idx = 0;
static int save_att(zcc::mail_parser *parser, zcc::mail_parser::mime_node *mime)
{
    const char *sname;
    char tmpname[256];

    idx++;
    sname = mime->get_show_name().c_str();
    if (zcc::empty(sname))
    {
        std::sprintf(tmpname, "%s%d_unknown.dat", name_prefix, idx);
    }
    else
    {
        std::snprintf(tmpname, 255, "%s%d_%s", name_prefix, idx, sname);
    }
    std::string pathname = "att/";
    pathname.append(zcc::format_filename(tmpname));
    std::string con = mime->get_decoded_content();

    std::printf("save attachment %s\n", pathname.c_str());
    if (zcc::file_put_contents(pathname.c_str(), con) < 1)
    {
        std::printf("ERROR decode_mime_body: save\n");
    }
    return 0;
}

static int save_all_attachments(zcc::mail_parser *parser)
{
    auto &allm = parser->get_attachment_mimes();
    for (auto it = allm.begin(); it != allm.end(); it++)
    {
        save_att(parser, *it);
    }
    return 0;
}

static void do_parse(const char *eml_fn)
{
    zcc::mail_parser *parser = zcc::mail_parser::create_from_file(eml_fn);
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
    name_prefix = zcc::var_main_config.get_cstring("np");

    if (zcc::main_argument::var_parameters.size() == 0)
    {
        std::printf("USAGE: %s [--att] eml_fn...\n", zcc::progname);
        zcc::exit(0);
    }

    for (auto fn : zcc::main_argument::var_parameters)
    {
        do_parse(fn);
    }

    return 0;
}
