/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-05-30
 * ================================
 */

#include "zcc/zcc_mime.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    zcc::mail_builder builder;
    builder.set_subject("测试主题");
    builder.set_from("发件人", "xxx@aaa.com");
    builder.add_to("收件人1", "xxx1@aaa.com");
    builder.add_to("", "xxx2@aaa.com");
    builder.add_cc("抄送vvv", "xxx3@aaa.com");
    //
    zcc::mail_builder::attachment att;
    att.filename = "就是一个文件名.txt";
    att.content_data = "这是一个文件内容";

    zcc::mail_builder::attachment att2;
    att2.filename = "hosts 文件.txt";
    att2.content_data = "127.0.0.1 localhsot\r\n";
    builder.add_attachment(att2);

    //
    std::string eml = builder.build();
    std::printf("eml:\n%s\n", eml.c_str());
    //
    return 0;
}
