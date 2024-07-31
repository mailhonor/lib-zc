/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-11
 * ================================
 */

#include "zcc/zcc_mime.h"

static void ___usage()
{
    zcc_fatal("USAGE: %s [ -loop 1000 ] [--onlymime ] eml_filename", zcc::progname);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);

    if (zcc::main_argument::var_parameters.empty())
    {
        ___usage();
    }
    const char *eml_fn = zcc::main_argument::var_parameters[0];
    int times = zcc::var_main_config.get_int("loop", 1000);
    bool onlymime = zcc::var_main_config.get_bool("onlymime", false);
    std::string eml_data = zcc::file_get_contents_sample(eml_fn);

    zcc_info("eml     : %s", eml_fn);
    zcc_info("size    : %zd(bytes)", eml_data.size());
    zcc_info("loop    : %d", times);
    std::string h = zcc::hunman_byte_size(eml_data.size() * times);
    zcc_info("total   : %s(bytes)", h.c_str());

    int64_t t = zcc::millisecond();
    for (int i = 0; i < times; i++)
    {
        zcc::mail_parser *parser = zcc::mail_parser::create_from_data(eml_data);
        if (!parser)
        {
            zcc_fatal("mail_parser open");
        }
        if (!onlymime)
        {
            parser->get_message_id();
            parser->get_subject();
            parser->get_subject_utf8();
            parser->get_date();
            parser->get_in_reply_to();
            parser->get_date_unix();
            parser->get_from();
            parser->get_from_utf8();
            parser->get_sender();
            parser->get_reply_to();
            parser->get_receipt();
            parser->get_to();
            parser->get_to_utf8();
            parser->get_cc();
            parser->get_cc_utf8();
            parser->get_bcc();
            parser->get_bcc_utf8();
            parser->get_references();
            parser->get_top_mime();
            parser->get_all_mimes();
            parser->get_text_mimes();
            parser->get_show_mimes();
            parser->get_attachment_mimes();
        }
        delete parser;
    }
    t = zcc::millisecond() - t;
    if (t > 0)
    {
        zcc_info("elapse  : %zd.%03zd(second)", t / 1000, t % 1000);
        zcc_info("%%second : %s(bytes)", zcc::hunman_byte_size(((eml_data.size() * times) / ((1.0 * t) / 1000))).c_str());
    }

    return 0;
}
