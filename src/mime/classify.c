/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "./mime.h"

#define _ZPMT_MULTIPART 1
#define _ZPMT_ATTACHMENT 2
#define _ZPMT_PLAIN 3
#define _ZPMT_HTML 4
static int ___mime_identify_type(zmime_t *mime)
{
    char *type = mime->type, *disposition = mime->disposition;
    if (!type)
    {
        type = zblank_buffer;
    }
    if (!disposition)
    {
        disposition = zblank_buffer;
    }
    if (ZEMPTY(type))
    {
        return _ZPMT_MULTIPART;
    }
    if (ZSTR_N_EQ(type, "multipart/", 10))
    {
        return _ZPMT_MULTIPART;
    }
    if (ZSTR_N_EQ(type, "application/", 12))
    {
        if (strstr(type + 12, "tnef"))
        {
            mime->is_tnef = 1;
        }
        return _ZPMT_ATTACHMENT;
    }
    if ((!ZEMPTY(disposition)) && (ZSTR_N_EQ(disposition, "attachment", 10)))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "image/", 6))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "audio/", 6))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "video/", 6))
    {
        return _ZPMT_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "text/", 5))
    {
        if (strstr(disposition, "attachment"))
        {
            return _ZPMT_ATTACHMENT;
        }
        if (!strcmp(type + 5, "html"))
        {
            return _ZPMT_HTML;
        }
        if (!strcmp(type + 5, "plain"))
        {
            return _ZPMT_PLAIN;
        }
        if (!strcmp(type + 5, "calendar"))
        {
            return _ZPMT_PLAIN;
        }
        return _ZPMT_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "message/", 8))
    {
        if (strstr(type + 8, "delivery"))
        {
            return _ZPMT_PLAIN;
        }
        if (strstr(type + 8, "notification"))
        {
            return _ZPMT_PLAIN;
        }
        return _ZPMT_ATTACHMENT;
    }

    return _ZPMT_ATTACHMENT;
}

/* ################################################################## */
typedef struct
{
    zmime_t *alternative;
    zmime_t *self;
} ___view_mime_t;

static int ___mime_identify_view_part(zmime_t *mime, ___view_mime_t *view_list, int *view_len)
{
    int i, mime_type;
    zmime_t *parent;

    mime_type = mime->mime_type;

    if ((mime_type != _ZPMT_PLAIN) && (mime_type != _ZPMT_HTML))
    {
        return 0;
    }
    if (ZEMPTY(mime->type))
    {
        return 0;
    }

    for (parent = mime->parent; parent; parent = parent->parent)
    {
        if (parent->type && ZSTR_EQ(parent->type, "multipart/alternative"))
        {
            break;
        }
    }
    if (!parent)
    {
        view_list[*view_len].alternative = 0;
        view_list[*view_len].self = mime;
        *view_len = *view_len + 1;
        return 0;
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
        return 0;
    }

    view_list[i].alternative = parent;
    if (ZSTR_EQ(mime->type, "text/html"))
    {
        view_list[i].self = mime;
    }

    return 0;
}

void zmime_classify(zmail_t *parser)
{
    if (parser->classify_flag)
    {
        return;
    }
    parser->classify_flag = 1;

    do
    {
        /* classify */
        int text_count = 0, att_count = 0;
        ZVECTOR_WALK_BEGIN(parser->all_mimes, zmime_t *, m)
        {
            zmime_get_disposition(m);
            m->mime_type = ___mime_identify_type(m);
            if ((m->mime_type == _ZPMT_PLAIN) || (m->mime_type == _ZPMT_HTML))
            {
                text_count++;
            }
            else if (m->mime_type == _ZPMT_ATTACHMENT)
            {
                att_count++;
            }
        }
        ZVECTOR_WALK_END;

        parser->text_mimes = zvector_create(text_count + 1);

        parser->attachment_mimes = zvector_create(att_count + 1);

        ZVECTOR_WALK_BEGIN(parser->all_mimes, zmime_t *, m)
        {
            m->mime_type = m->mime_type;
            if ((m->mime_type == _ZPMT_PLAIN) || (m->mime_type == _ZPMT_HTML))
            {
                zvector_push(parser->text_mimes, m);
            }
            else if (m->mime_type == _ZPMT_ATTACHMENT)
            {
                zvector_push(parser->attachment_mimes, m);
            }
        }
        ZVECTOR_WALK_END;
    } while (0);

    do
    {
        /* similar to the above text-mime,
         * in addition to the case of alternative, html is preferred */
        int view_len_max = zvector_len(parser->all_mimes) + 10;
        ___view_mime_t *view_mime = (___view_mime_t *)zmalloc(sizeof(___view_mime_t) * view_len_max);
        int view_len = 0;
        ZVECTOR_WALK_BEGIN(parser->all_mimes, zmime_t *, m)
        {
            if (view_len < view_len_max)
            {
                ___mime_identify_view_part(m, view_mime, &view_len);
            }
        }
        ZVECTOR_WALK_END;

        parser->show_mimes = zvector_create(view_len + 1);
        for (int i = 0; i < view_len; i++)
        {
            zvector_push(parser->show_mimes, view_mime[i].self);
        }
        zfree(view_mime);
    } while (0);
}
