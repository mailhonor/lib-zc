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

struct recursion_info_t
{
    recursion_info_t *prev{nullptr};
    recursion_info_t *next{nullptr};
    zcc::mail_parser::mime_node *mime{nullptr};
    zcc::mail_parser::mime_node *child_head_mime{nullptr};
    std::string *section_bf{nullptr};
    std::string *nsection{nullptr};
    std::vector<std::string> *argv{nullptr};
    int child_head_count{0};
    int i{0};
};

void mail_parser::imap_section()
{
    if (imap_section_flag_)
    {
        return;
    }
    imap_section_flag_ = true;

    char intbuf[16];
    int child_head_count;
    zcc::mail_parser::mime_node *mime, *cm, *fm;
    const char *section;
    std::string *nsection, *top_section_bf;
    std::vector<std::string> *argv;
    recursion_info_t *rhead = 0, *rtail = 0, *rnode;
    rnode = new recursion_info_t();
    ZCC_MLINK_APPEND(rhead, rtail, rnode, prev, next);
    rnode->mime = top_mime_;
    rnode->i = -1;
    top_section_bf = rnode->section_bf = new std::string();
    rnode->section_bf->append("0");

    while (rtail)
    {
        rnode = rtail;
        mime = rnode->mime;
        section = rnode->section_bf->c_str();
        mime->imap_section_.clear();
        mime->imap_section_ = *(rnode->section_bf);
        child_head_count = ___child_head_count(mime);
        if (child_head_count == 0)
        {
            ZCC_MLINK_DETACH(rhead, rtail, rnode, prev, next);
            delete rnode->nsection;
            delete rnode->argv;
            delete rnode;
            continue;
        }
        if (rnode->i == -1)
        {
            rnode->argv = new std::vector<std::string>();
            *(rnode->argv) = split(*(rnode->section_bf), '.');
            rnode->child_head_count = ___child_head_count(mime);
            rnode->child_head_mime = mime->child_head_;
            rnode->nsection = new std::string();
        }
        else
        {
            rnode->child_head_mime = rnode->child_head_mime->next_;
        }
        rnode->i++;
        if (rnode->i == rnode->child_head_count)
        {
            ZCC_MLINK_DETACH(rhead, rtail, rnode, prev, next);
            delete rnode->nsection;
            delete rnode->argv;
            delete rnode;
            continue;
        }

        argv = rnode->argv;
        fm = mime->child_head_;
        cm = rnode->child_head_mime;
        nsection = rnode->nsection;
        if (!strcmp(section, ""))
        {
            std::sprintf(intbuf, "%d", rnode->i + 1);
            (*nsection) = intbuf;
        }
        else if ((argv->size() > 1) && (strncmp(fm->content_type_.c_str(), "multipart/", 11)))
        {
            nsection->clear();
            for (uint64_t k = 1; k < argv->size(); k++)
            {
                nsection->append((*argv)[k - 1]).append(".");
            }
            std::sprintf(intbuf, "%d", rnode->i + 1);
            nsection->append(intbuf);
        }
        else
        {
            if (argv->back() == "0")
            {
                nsection->clear();
                for (uint64_t k = 1; k < argv->size(); k++)
                {
                    nsection->append((*argv)[k - 1]).append(".");
                }
                std::sprintf(intbuf, "%d", rnode->i + 1);
                nsection->append(intbuf);
            }
            else
            {
                *nsection = section;
            }
        }
        if (cm->child_head_)
        {
            if (!((!strncmp(cm->content_type_.c_str(), "message/", 8)) && (!strncmp(cm->child_head_->content_type_.c_str(), "multipart/", 11))))
            {
                nsection->append(".0");
            }
        }
        rnode = new recursion_info_t();
        ZCC_MLINK_APPEND(rhead, rtail, rnode, prev, next);
        rnode->mime = cm;
        rnode->i = -1;
        rnode->section_bf = nsection;
    }

    // 只有一个 part 且不是 multipart
    mime = top_mime_;
    if (mime->child_head_ == 0)
    {
        if (strncmp(mime->content_type_.c_str(), "multipart/", 11))
        {
            mime->imap_section_[0] = '1';
        }
    }

    delete top_section_bf;
}

zcc_namespace_end;
