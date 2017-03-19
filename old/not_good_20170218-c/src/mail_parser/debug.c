/*
 * ================================
 * eli960@123.com
 * http://www.mailhonor.com/
 * 2015-12-15
 * ================================
 */

#include "libzc.h"

#include <time.h>

static char *fmt = "%-15s %s\n";
static char *fmt_ld = "%-15s %ld\n";
static char *fmt_none = "%-15s\n";
static char *fmt_addr = "%-15s %c %s\n";

static void addr_list_show(char *hid, zmail_addr_t * ma)
{
    if (!ma) {
        printf(fmt_none, hid);
        return;
    }

    for (; ma; ma = ma->next) {
        printf(fmt_addr, hid, '/', ma->mail);
        printf(fmt_addr, "", '|', ma->name);
        printf(fmt_addr, "", '\\', ma->name_rd);
    }
}

static void addr_show(char *hid, zmail_addr_t * ma)
{
    if (!ma) {
        printf(fmt_none, hid);
        return;
    }
    printf(fmt_addr, hid, '/', ma->mail);
    printf(fmt_addr, "", '|', ma->name);
    printf(fmt_addr, "", '\\', ma->name_rd);
}

static void references_show(char *hid, zmail_references_t * references)
{
    int i = 0;

    if (!references) {
        printf(fmt_none, hid);
        return;
    }

    for (; references; references = references->next) {
        i++;
        printf(fmt, i == 1 ? hid : "", references->message_id);
    }
}

static void all_mime_show(char *hid, zmail_mime_t * mime)
{
    zmail_mime_t *m;
    char buf[128];

    ZMAIL_PARSER_MIME_WALK_BEGIN(mime, m) {
        printf(fmt_addr, hid, '/', m->type);
        printf(fmt_addr, "", '|', m->disposition);
        printf(fmt_addr, "", '|', m->name);
        printf(fmt_addr, "", '|', m->name_rd);
        printf(fmt_addr, "", '|', m->filename);
        printf(fmt_addr, "", '|', m->filename_star);
        printf(fmt_addr, "", '|', m->filename_rd);
        printf(fmt_addr, "", '|', m->content_id);
        printf(fmt_addr, "", '|', m->encoding);
        printf(fmt_addr, "", '|', m->boundary);
        sprintf(buf, "%d,%d,%d,%d", m->header_offset, m->header_len, m->body_offset, m->body_len);
        printf(fmt_addr, "", '\\', buf);
    }
    ZMAIL_PARSER_MIME_WALK_END;
}

static void attachments_show(char *hid, zmail_mime_t ** mime, int count)
{
    int i;
    if (count == 0) {
        printf(fmt, hid, "not found");
        return;
    }

    for (i = 0; i < count; i++) {
        printf(fmt_ld, hid, (i + 1));
        printf(fmt_addr, "", '/', mime[i]->name);
        printf(fmt_addr, "", '|', mime[i]->name_rd);
        printf(fmt_addr, "", '|', mime[i]->filename);
        printf(fmt_addr, "", '|', mime[i]->filename_star);
        printf(fmt_addr, "", '\\', mime[i]->filename_rd);
    }
}

static void text_show(zmail_parser_t * parser, char *hid, zmail_mime_t ** mime_limit, int count)
{
    int i;
    int len;
    zmail_mime_t *mime;
    zbuf_t *zout;

    zout = zbuf_create(10240);
    for (i = 0; i < count; i++) {
        mime = mime_limit[i];
        printf(fmt, hid, mime->type);
        zbuf_reset(zout);
        len = zmail_parser_decode_text_mime_body(parser, mime, zout);
        if (len < 0) {
            printf("found error\n");
            continue;
        }
        printf(fmt, "", ZBUF_DATA(zout));
    }
    zbuf_free(zout);
}

void zmail_parser_show(zmail_parser_t * parser)
{
    printf(fmt, "Date:", parser->date);
    printf(fmt_ld, "Date_unix:", parser->date_unix);

    printf(fmt, "Subject:", parser->subject);
    printf(fmt, "Subject_rd:", parser->subject_rd);

    addr_show("From:", parser->from);

    addr_list_show("To:", parser->to);
    addr_list_show("Cc:", parser->cc);
    addr_list_show("Bcc:", parser->bcc);

    printf(fmt, "Message-ID:", parser->message_id);

    addr_show("Sender:", parser->sender);
    addr_show("Reply_to:", parser->reply_to);
    addr_show("Receipt:", parser->receipt);
    references_show("References:", parser->references);
    printf(fmt, "In-Reply-To:", parser->in_reply_to);

    all_mime_show("Mime:", parser->mime);
    attachments_show("Attachment:", parser->attachment_mime_list, parser->attachment_mime_count);

    text_show(parser, "Text:", parser->text_mime_list, parser->text_mime_count);
}

/* ################################################################## */
static void ___json_text_escape(char *str, int len, zbuf_t * result)
{
    int i;
    char ch;

    if (len < 0) {
        if (!str) {
            len = 0;
        } else {
            len = strlen(str);
        }
    }

    for (i = 0; i < len; i++) {
        ch = str[i];
        if (ch == '\0') {
            zbuf_strcat(result, "\\0");
            continue;
        }
        if (ch == '\\') {
            zbuf_strcat(result, "\\\\");
            continue;
        }
        if (ch == '"') {
            zbuf_strcat(result, "\\\"");
            continue;
        }
        if (ch == '\r') {
            zbuf_strcat(result, "\\r");
            continue;
        }
        if (ch == '\n') {
            zbuf_strcat(result, "\\n");
            continue;
        }
        zbuf_put(result, ch);
    }
}

static void ___json_add_kv(int first, char *key, char *value, zbuf_t * result)
{
    if (!first) {
        zbuf_strcat(result, ",");
    }
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":\"");
    ___json_text_escape(value, strlen(value), result);
    zbuf_strcat(result, "\"");
}

static void ___json_add_addr(int first, char *key, zmail_addr_t * ma, zbuf_t * result)
{
    if (!ma) {
        return;
    }
    if (!first) {
        zbuf_strcat(result, ",");
    }
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":{");
    ___json_add_kv(1, "mail", ma->mail, result);
    ___json_add_kv(0, "name", ma->name, result);
    ___json_add_kv(0, "name_rd", ma->name_rd, result);
    zbuf_strcat(result, "}");
}

static void ___json_add_addr_list(int first, char *key, zmail_addr_t * ma, zbuf_t * result)
{
    int f2 = 1;

    if (!ma) {
        return;
    }
    if (!first) {
        zbuf_strcat(result, ",");
    }
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":[");
    for (; ma; ma = ma->next) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        f2 = 0;
        zbuf_strcat(result, "{");
        ___json_add_kv(1, "mail", ma->mail, result);
        ___json_add_kv(0, "name", ma->name, result);
        ___json_add_kv(0, "name_rd", ma->name_rd, result);
        zbuf_strcat(result, "}");
    }
    zbuf_strcat(result, "]");
}

static void ___json_add_references(int first, char *key, zmail_references_t * references, zbuf_t * result)
{
    int f2 = 1;

    if (!references) {
        return;
    }
    if (!first) {
        zbuf_strcat(result, ",");
    }

    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":[");
    for (; references; references = references->next) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        f2 = 0;
        zbuf_strcat(result, "\"");
        ___json_text_escape(references->message_id, -1, result);
        zbuf_strcat(result, "\"");
    }
    zbuf_strcat(result, "]");
}

static void ___json_add_mime(int first, char *key, zmail_mime_t * mime, zbuf_t * result)
{
    int f2 = 1;
    zmail_mime_t *m;
    char buf[128];

    if (!mime) {
        return;
    }
    if (!first) {
        zbuf_strcat(result, ",");
    }

    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":[");
    ZMAIL_PARSER_MIME_WALK_BEGIN(mime, m) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        f2 = 0;
        zbuf_strcat(result, "{");
        ___json_add_kv(1, "type", m->type, result);
        ___json_add_kv(0, "disposition", m->disposition, result);
        ___json_add_kv(0, "name", m->name, result);
        ___json_add_kv(0, "name_rd", m->name_rd, result);
        ___json_add_kv(0, "filename", m->filename, result);
        ___json_add_kv(0, "filename_star", m->filename_star, result);
        ___json_add_kv(0, "filename_rd", m->filename_rd, result);
        ___json_add_kv(0, "content_id", m->content_id, result);
        ___json_add_kv(0, "encoding", m->encoding, result);
        ___json_add_kv(0, "boundary", m->boundary, result);
        sprintf(buf, "%d,%d,%d,%d", m->header_offset, m->header_len, m->body_offset, m->body_len);
        ___json_add_kv(0, "offset", buf, result);
        zbuf_strcat(result, "}");
    }
    ZMAIL_PARSER_MIME_WALK_END;
    zbuf_strcat(result, "]");
}

static void ___json_add_text(zmail_parser_t * parser, int first, char *key, zmail_mime_t ** mime_limit, int count, zbuf_t * result)
{
    int i;
    int len;
    zmail_mime_t *mime;
    int f2 = 1;
    zbuf_t *zout;

    if (!count) {
        return;
    }
    zout = zbuf_create(10240);
    if (!first) {
        zbuf_strcat(result, ",");
    }

    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":[");
    for (i = 0; i < count; i++) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        mime = mime_limit[i];
        zbuf_reset(zout);
        len = zmail_parser_decode_text_mime_body(parser, mime, zout);
        if (len < 0) {
            continue;
        }
        zbuf_strcat(result, "{");
        ___json_add_kv(1, "type", mime->type, result);
        ___json_add_kv(0, "content", ZBUF_DATA(zout), result);
        zbuf_strcat(result, "}");
        f2 = 0;
    }
    zbuf_strcat(result, "]");
    zbuf_free(zout);
}

void zmail_parser_show_json(zmail_parser_t * parser, zbuf_t * result)
{
    zbuf_strcat(result, "{");

    ___json_add_kv(1, "date", parser->date, result);
    ___json_add_kv(0, "subject", parser->subject, result);
    ___json_add_kv(0, "subject_rd", parser->subject_rd, result);

    ___json_add_addr(0, "from", parser->from, result);
    ___json_add_addr_list(0, "to", parser->to, result);
    ___json_add_addr_list(0, "cc", parser->cc, result);
    ___json_add_addr_list(0, "bcc", parser->bcc, result);

    ___json_add_kv(0, "message_id", parser->message_id, result);

    ___json_add_addr(0, "sender", parser->sender, result);
    ___json_add_addr(0, "reply_to", parser->reply_to, result);
    ___json_add_addr(0, "receipt", parser->receipt, result);

    ___json_add_references(0, "references", parser->references, result);
    ___json_add_kv(0, "in_reply_to", parser->in_reply_to, result);

    ___json_add_mime(0, "mime", parser->mime, result);

    ___json_add_text(parser, 0, "text", parser->text_mime_list, parser->text_mime_count, result);

    zbuf_strcat(result, "}");
}
