/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

static inline int ___child_head_count(zcc::mail_parser::mime_node *m)
{
    int count = 0;
    for (m = m->get_child(); m; m = m->get_next())
    {
        count++;
    }
    return count;
}

void mail_parser::mime_node::_section_walk(const std::string &parent_section, int idx)
{
    auto tmp = parent_section;
    if (!tmp.empty())
    {
        tmp.append(".");
    }
    tmp.append(std::to_string(idx + 1));
    imap_section_ = tmp;

    int i = 0;
    for (auto &n = child_head_; n; n = n->next_)
    {
        n->_section_walk(tmp, i);
        i++;
    }
}

void mail_parser::imap_section()
{
    if (imap_section_flag_)
    {
        return;
    }
    imap_section_flag_ = true;
    top_mime_->imap_section_ = "1";
    int i = 0;
    for (auto &n = top_mime_->child_head_; n; n = n->next_)
    {
        n->_section_walk("", i);
        i++;
    }
}

zcc_namespace_end;
