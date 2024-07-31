/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-11
 * ================================
 */

#include "zcc/zcc_mime.h"
#include "zcc/zcc_thread.h"
#include <atomic>
#include <functional>

std::atomic<int> mail_sum(0);

static void do_parse(const char *eml_fn)
{
    std::string text;
    zcc::mail_parser *parser = zcc::mail_parser::create_from_file(eml_fn);
    if (parser == 0)
    {
        zcc_error_and_exit("open %s", eml_fn);
    }
    text.append(parser->get_subject_utf8()).append("\n");
    text.append(parser->get_from_utf8().name_utf8_).append("\n");
    for (auto it = parser->get_to_utf8().begin(); it != parser->get_to_utf8().end(); it++)
    {
        text.append(it->name_utf8_).append("\n");
    }
    for (auto it = parser->get_cc_utf8().begin(); it != parser->get_cc_utf8().end(); it++)
    {
        text.append(it->name_utf8_).append("\n");
    }
    for (auto it = parser->get_bcc_utf8().begin(); it != parser->get_bcc_utf8().end(); it++)
    {
        text.append(it->name_utf8_).append("\n");
    }
    for (auto it = parser->get_text_mimes().begin(); it != parser->get_text_mimes().end(); it++)
    {
        text.append((*it)->get_decoded_content_utf8()).append("\n");
    }
    for (auto it = parser->get_attachment_mimes().begin(); it != parser->get_attachment_mimes().end(); it++)
    {
        text.append((*it)->get_filename_utf8()).append("\n");
        text.append((*it)->get_name_utf8()).append("\n");
    }
    delete parser;

    const char *fn = std::strrchr(eml_fn, '/');
    if (fn)
    {
        std::string pn = "text";
        pn.append(fn);
        if (zcc::file_put_contents(pn.c_str(), text) < 1)
        {
            zcc_error_and_exit("write: %s => %s", eml_fn, pn.c_str());
        }
    }
    mail_sum--;
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() == 0)
    {
        zcc_error_and_exit("USAGE: %s eml_fn...\n", zcc::progname);
    }
    auto flist = zcc::find_file_sample(zcc::main_argument::var_parameters);
    mail_sum = flist.size();
    auto pool = zcc::thread_pool();
    pool.create_thread(8);
    for (auto &fn : flist)
    {
        pool.enter_task(std::bind(do_parse, fn.c_str()));
    }
    while (1)
    {
        zcc::sleep(1);
        if (mail_sum == 0)
        {
            break;
        }
    }
    pool.softstop();
    pool.wait_all_stopped(10);

    return 0;
}
