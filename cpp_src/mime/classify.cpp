/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

#define _ZPMT_MULTIPART 1
#define _ZPMT_ATTACHMENT 2
#define _ZPMT_PLAIN 3
#define _ZPMT_HTML 4

int64_t mail_parser::classify_mime_identify_type(mime_node *mime)
{
    const char *type = mime->get_content_type().c_str();
    const char *disposition = mime->get_disposition().c_str();

    if (empty(type))
    {
        return _ZPMT_MULTIPART;
    }
    if (ZCC_STR_N_EQ(type, "multipart/", 10))
    {
        return _ZPMT_MULTIPART;
    }
    if (ZCC_STR_N_EQ(type, "application/", 12))
    {
        if (std::strstr(type + 12, "tnef"))
        {
            mime->is_tnef_ = true;
        }
        return _ZPMT_ATTACHMENT;
    }
    if ((!empty(disposition)) && (ZCC_STR_N_EQ(disposition, "attachment", 10)))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "image/", 6))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "audio/", 6))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "video/", 6))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "text/", 5))
    {
        if (std::strstr(disposition, "attachment"))
        {
            return _ZPMT_ATTACHMENT;
        }
        if (!std::strcmp(type + 5, "html"))
        {
            return _ZPMT_HTML;
        }
        if (!std::strcmp(type + 5, "plain"))
        {
            return _ZPMT_PLAIN;
        }
        if (!std::strcmp(type + 5, "calendar"))
        {
            return _ZPMT_ATTACHMENT;
        }
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "message/", 8))
    {
        if (std::strstr(type + 8, "delivery"))
        {
            return _ZPMT_PLAIN;
        }
        if (std::strstr(type + 8, "notification"))
        {
            return _ZPMT_PLAIN;
        }
        return _ZPMT_ATTACHMENT;
    }

    return _ZPMT_ATTACHMENT;
}

struct ___view_mime_t
{
    mail_parser::mime_node *alternative;
    mail_parser::mime_node *self;
};

void mail_parser::classify_mime_identify_view_part(mime_node *mime, void *_view_list, int64_t *view_len)
{
    ___view_mime_t *view_list = (___view_mime_t *)_view_list;
    int64_t i, mime_type;
    mail_parser::mime_node *parent;

    mime_type = mime->mime_type_;

    if ((mime_type != _ZPMT_PLAIN) && (mime_type != _ZPMT_HTML))
    {
        return;
    }
    if (mime->content_type_.empty())
    {
        return;
    }

    for (parent = mime->parent_; parent; parent = parent->parent_)
    {
        if (parent->content_type_ == "multipart/alternative")
        {
            break;
        }
    }
    if (!parent)
    {
        view_list[*view_len].alternative = 0;
        view_list[*view_len].self = mime;
        *view_len = *view_len + 1;
        return;
    }

    for (i = 0; i < *view_len; i++)
    {
        if (view_list[i].alternative == parent)
        {
            break;
        }
    }

    if (i == *view_len)
    {
        view_list[*view_len].alternative = parent;
        view_list[*view_len].self = mime;
        *view_len = *view_len + 1;
        return;
    }

    view_list[i].alternative = parent;
    if (mime->content_type_ == "text/html")
    {
        view_list[i].self = mime;
    }
}

void mail_parser::classify()
{
    if (classify_flag_)
    {
        return;
    }
    classify_flag_ = true;

    for (auto it = all_mimes_.begin(); it != all_mimes_.end(); it++)
    {
        auto m = *it;
        m->mime_type_ = classify_mime_identify_type(m);
        if ((m->mime_type_ == _ZPMT_PLAIN) || (m->mime_type_ == _ZPMT_HTML))
        {
            text_mimes_.push_back(m);
        }
        else if (m->mime_type_ == _ZPMT_ATTACHMENT)
        {
            attachment_mimes_.push_back(m);
        }
    }

    /* similar to the above text-mime,
     * in addition to the case of alternative, html is preferred */
    int64_t view_len_max = all_mimes_.size() + 10;
    ___view_mime_t *view_mime = (___view_mime_t *)zcc::calloc(sizeof(___view_mime_t), view_len_max);
    int64_t view_len = 0;
    for (auto it = all_mimes_.begin(); it != all_mimes_.end(); it++)
    {
        auto m = *it;
        if (view_len < view_len_max)
        {
            classify_mime_identify_view_part(m, view_mime, &view_len);
        }
    }

    for (int64_t i = 0; i < view_len; i++)
    {
        show_mimes_.push_back(view_mime[i].self);
    }
    zcc::free(view_mime);
}

zcc_namespace_end;
