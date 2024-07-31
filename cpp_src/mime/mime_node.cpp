/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

mail_parser::mime_node::mime_node(mail_parser &parser) : parser_(parser)
{
}

mail_parser::mime_node::~mime_node()
{
}

zcc_namespace_end;
