/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-16
 * ================================
 */

#include "libzc.h"

static int ___mime_identify_type(zmail_mime_t * mime)
{
    char *type;

    type = mime->type;
    if (ZEMPTY(type)) {
        return ZMAIL_PARSER_MIME_TYPE_MULTIPART;
    }
    if (ZSTR_N_EQ(type, "multipart/", 10)) {
        return ZMAIL_PARSER_MIME_TYPE_MULTIPART;
    }
    if (ZSTR_N_EQ(type, "application/", 12)) {
        if (strstr(type + 12, "tnef")) {
            mime->is_tnef = 1;
        }
        return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
    }
    if ((!ZEMPTY(mime->disposition)) && (ZSTR_N_EQ(mime->disposition, "attachment", 10))) {
        return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "image/", 6)) {
        return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "audio/", 6)) {
        return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "video/", 6)) {
        return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "text/", 5)) {
        if (!strcmp(type + 5, "html")) {
            if (strstr(mime->disposition, "attachment")) {
                return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
            }
            return ZMAIL_PARSER_MIME_TYPE_HTML;
        }
        if (!strcmp(type + 5, "plain")) {
            if (strstr(mime->disposition, "attachment")) {
                return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
            }
            return ZMAIL_PARSER_MIME_TYPE_PLAIN;
        }
        return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
    }
    if (ZSTR_N_EQ(type, "message/", 8)) {
        if (strstr(type + 8, "delivery")) {
            return ZMAIL_PARSER_MIME_TYPE_PLAIN;
        }
        if (strstr(type + 8, "notification")) {
            return ZMAIL_PARSER_MIME_TYPE_PLAIN;
        }
        return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
    }

    return ZMAIL_PARSER_MIME_TYPE_ATTACHMENT;
}

/* ################################################################## */
typedef struct {
    zmail_mime_t *alternative;
    zmail_mime_t *self;
} ___view_mime_t;

static int ___mime_identify_view_part(zmail_mime_t * mime, ___view_mime_t * view_list, int *view_len)
{
    char *type;
    int i, mime_type;
    zmail_mime_t *parent;

    type = mime->type;
    mime_type = mime->mime_type;

    if ((mime_type != ZMAIL_PARSER_MIME_TYPE_PLAIN) && (mime_type != ZMAIL_PARSER_MIME_TYPE_HTML)) {
        return 0;
    }
    if (ZEMPTY(type)) {
        return 0;
    }

    for (parent = mime->parent; parent; parent = parent->parent) {
        if (ZSTR_EQ(parent->type, "multipart/alternative")) {
            break;
        }
    }
    if (!parent) {
        view_list[*view_len].alternative = 0;
        view_list[*view_len].self = mime;
        *view_len = *view_len + 1;
        return 0;
    }

    for (i = 0; i < *view_len; i++) {
        if (view_list[i].alternative == parent) {
            break;
        }
    }

    if (i == *view_len) {
        view_list[*view_len].alternative = parent;
        view_list[*view_len].self = mime;
        *view_len = *view_len + 1;
        return 0;
    }

    view_list[i].alternative = parent;
    if (ZSTR_EQ(mime->type, "text/html")) {
        view_list[i].self = mime;
    }

    return 0;
}

/* ################################################################## */
int zmail_parser_mime_identify_type(zmail_parser_t * parser)
{
    zmpool_t *imp = parser->mpool;
    zmail_mime_t *mime = parser->mime;
    zmail_mime_t *m;
    int i;

    {
        /* classify */
        int type;
        zmail_mime_t *text_mime[1024];
        int text_len = 0;
        zmail_mime_t *att_mime[1024];
        int att_len = 0;

        ZMAIL_PARSER_MIME_WALK_BEGIN(mime, m) {
            type = ___mime_identify_type(m);
            m->mime_type = type;
            if ((type == ZMAIL_PARSER_MIME_TYPE_PLAIN) || (type == ZMAIL_PARSER_MIME_TYPE_HTML)) {
                if (text_len < 1000) {
                    text_mime[text_len++] = m;
                }
            } else if (type == ZMAIL_PARSER_MIME_TYPE_ATTACHMENT) {
                if (att_len < 1000) {
                    att_mime[att_len++] = m;
                }
            }
        }
        ZMAIL_PARSER_MIME_WALK_END;

        parser->text_mime_count = text_len;
        parser->text_mime_list = (zmail_mime_t **) zmpool_calloc(imp, text_len + 1, (sizeof(zmail_mime_t *)));
        for (i = 0; i < text_len; i++) {
            parser->text_mime_list[i] = text_mime[i];
        }
        text_mime[text_len] = 0;

        parser->attachment_mime_count = att_len;
        parser->attachment_mime_list = (zmail_mime_t **) zmpool_calloc(imp, att_len + 1, (sizeof(zmail_mime_t *)));
        for (i = 0; i < att_len; i++) {
            parser->attachment_mime_list[i] = att_mime[i];
        }
        att_mime[att_len] = 0;
    }

    {
        /* similar to the above text-mime, 
         * in addition to the case of alternative, html is preferred */
        ___view_mime_t view_mime[1024];
        int view_len = 0;
        ZMAIL_PARSER_MIME_WALK_BEGIN(mime, m) {
            if (view_len < 1000) {
                ___mime_identify_view_part(m, view_mime, &view_len);
            }
        }
        ZMAIL_PARSER_MIME_WALK_END;
        parser->view_mime_count = view_len;
        parser->view_mime_list = (zmail_mime_t **) zmpool_calloc(imp, view_len + 1, sizeof(zmail_mime_t *));
        for (i = 0; i < view_len; i++) {
            parser->view_mime_list[i] = view_mime[i].self;
        }
        parser->view_mime_list[i] = 0;
    }

    return 0;
}
