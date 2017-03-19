/*
 * ================================
 * eli960@123.com
 * http://www.mailhonor.com/
 * 2015-12-15
 * ================================
 */

#include "zc.h"

#include <time.h>

static char *fmt = "%-15s %s\n";
static char *fmt_ld = "%-15s %ld\n";
static char *fmt_none = "%-15s\n";
static char *fmt_addr = "%-15s %c %s\n";

static void addr_list_show(char *hid, const zvector_t *vec)
{
    zmime_address_t *addr;

    if ((!vec) || (!ZVECTOR_LEN(vec))) {
        printf(fmt_none, hid);
        return;
    }

    ZVECTOR_WALK_BEGIN(vec, addr) {
        printf(fmt_addr, hid, '/', addr->address);
        printf(fmt_addr, "", '|', addr->name);
        printf(fmt_addr, "", '\\', addr->name_utf8);
    } ZVECTOR_WALK_END;
}

static void addr_show(char *hid, const zmime_address_t * ma)
{
    if (!ma) {
        printf(fmt_none, hid);
        return;
    }
    printf(fmt_addr, hid, '/', ma->address);
    printf(fmt_addr, "", '|', ma->name);
    printf(fmt_addr, "", '\\', ma->name_utf8);
}

static void references_show(char *hid, const zvector_t * references)
{
    char *mid;
    int i = 0;

    if ((!references) || (!ZVECTOR_LEN(references))) {
        printf(fmt_none, hid);
        return;
    }

    ZVECTOR_WALK_BEGIN(references, mid) {
        i++;
        printf(fmt, i == 1 ? hid : "", mid);
    } ZVECTOR_WALK_END;
}

static void all_mime_show(char *hid, const zvector_t *vec)
{
    zmime_t *m;
    int e;
    char buf[128];

    ZVECTOR_WALK_BEGIN(vec, m) {
        printf(fmt_addr, hid, '/', zmime_type(m));
        printf(fmt_addr, "", '|', zmime_disposition(m));
        printf(fmt_addr, "", '|', zmime_name(m));
        printf(fmt_addr, "", '|', zmime_name_utf8(m));
        printf(fmt_addr, "", '|', zmime_filename(m));
        printf(fmt_addr, "", '|', zmime_filename2231(m, &e));
        printf(fmt_addr, "", '|', zmime_filename_utf8(m));
        printf(fmt_addr, "", '|', zmime_content_id(m));
        printf(fmt_addr, "", '|', zmime_encoding(m));
        printf(fmt_addr, "", '|', zmime_boundary(m));
        sprintf(buf, "%zd,%zd,%zd,%zd",zmime_header_offset(m), zmime_header_size(m)
                , zmime_body_offset(m), zmime_body_size(m));
        printf(fmt_addr, "", '\\', buf);
    } ZVECTOR_WALK_END;
}

static void attachments_show(char *hid, const zvector_t *vec)
{
    int i = 0, e;
    zmime_t *m;
    if ((!vec)||(!ZVECTOR_LEN(vec))) {
        printf(fmt, hid, "not found");
        return;
    }

    ZVECTOR_WALK_BEGIN(vec, m) {
        i++;
        printf(fmt_ld, hid, (i + 1));
        printf(fmt_addr, "", '/', zmime_name(m));
        printf(fmt_addr, "", '|', zmime_name_utf8(m));
        printf(fmt_addr, "", '|', zmime_filename(m));
        printf(fmt_addr, "", '|', zmime_filename2231(m, &e));
        printf(fmt_addr, "", '\\', zmime_filename_utf8(m));
    } ZVECTOR_WALK_END;
}

static void text_show(const char *hid, const zvector_t *vec)
{
    int len;
    zmime_t *mime;
    zbuf_t *zout;

    if ((!vec)||(!ZVECTOR_LEN(vec))) {
        return;
    }
    zout = zbuf_create(10240);
    ZVECTOR_WALK_BEGIN(vec, mime) {
        printf(fmt, hid, zmime_type(mime));
        zbuf_reset(zout);
        len = zmime_decoded_content_utf8(mime, zout);
        if (len < 0) {
            printf("found error\n");
            continue;
        }
        printf(fmt, "", ZBUF_DATA(zout));
    } ZVECTOR_WALK_END;
    zbuf_free(zout);
}

void zmail_parser_show(zmail_t * parser)
{
    printf(fmt, "Date:", zmail_date(parser));
    printf(fmt_ld, "Date_unix:", zmail_date_unix(parser));

    printf(fmt, "Subject:", zmail_subject(parser));
    printf(fmt, "Subject_utf8:", zmail_subject_utf8(parser));

    addr_show("From:", zmail_from_utf8(parser));

    addr_list_show("To:", zmail_to_utf8(parser));
    addr_list_show("Cc:", zmail_cc_utf8(parser));
    addr_list_show("Bcc:", zmail_bcc_utf8(parser));

    printf(fmt, "Message-ID:", zmail_message_id(parser));

    addr_show("Sender:", zmail_sender(parser));
    addr_show("Reply_to:", zmail_reply_to(parser));
    addr_show("Receipt:", zmail_receipt(parser));
    references_show("References:", zmail_references(parser));
    printf(fmt, "In-Reply-To:", zmail_in_reply_to(parser));

    all_mime_show("Mime:", zmail_all_mimes(parser));
    attachments_show("Attachment:", zmail_attachment_mimes(parser));

    text_show("Text:", zmail_text_mimes(parser));
}

/* ################################################################## */
static void ___json_text_escape(const char *str, int len, zbuf_t * result)
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

static void ___json_add_kv(int first, const char *key, const char *value, zbuf_t * result)
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

static void ___json_add_addr(int first, const char *key, const zmime_address_t * ma, zbuf_t * result)
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
    ___json_add_kv(1, "mail", ma->address, result);
    ___json_add_kv(0, "name", ma->name, result);
    ___json_add_kv(0, "name_utf8", ma->name_utf8, result);
    zbuf_strcat(result, "}");
}

static void ___json_add_addr_list(int first, const char *key, const zvector_t *vec, zbuf_t * result)
{
    int f2 = 1;
    zmime_address_t *ma;

    if ((!vec)||(!ZVECTOR_LEN(vec))) {
        return;
    }
    if (!first) {
        zbuf_strcat(result, ",");
    }
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":[");
    ZVECTOR_WALK_BEGIN(vec, ma) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        f2 = 0;
        zbuf_strcat(result, "{");
        ___json_add_kv(1, "mail", ma->address, result);
        ___json_add_kv(0, "name", ma->name, result);
        ___json_add_kv(0, "name_utf8", ma->name_utf8, result);
        zbuf_strcat(result, "}");
    } ZVECTOR_WALK_END;
    zbuf_strcat(result, "]");
}

static void ___json_add_references(int first, char *key, zvector_t * references, zbuf_t * result)
{
    int f2 = 1;
    char *mid;

    if ((!references)||(!ZVECTOR_LEN(references))) {
        return;
    }
    if (!first) {
        zbuf_strcat(result, ",");
    }

    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":[");
    ZVECTOR_WALK_BEGIN(references, mid) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        f2 = 0;
        zbuf_strcat(result, "\"");
        ___json_text_escape(mid, -1, result);
        zbuf_strcat(result, "\"");
    } ZVECTOR_WALK_END;
    zbuf_strcat(result, "]");
}

static void ___json_add_mime(int first, char *key, zvector_t *vec, zbuf_t * result)
{
    int f2 = 1, e;
    zmime_t *m;
    char buf[128];

    if ((!vec) || (!ZVECTOR_LEN(vec))) {
        return;
    }
    if (!first) {
        zbuf_strcat(result, ",");
    }

    zbuf_strcat(result, "\"");
    zbuf_strcat(result, key);
    zbuf_strcat(result, "\"");
    zbuf_strcat(result, ":[");
    ZVECTOR_WALK_BEGIN(vec, m) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        f2 = 0;
        zbuf_strcat(result, "{");
        ___json_add_kv(1, "type", zmime_type(m), result);
        ___json_add_kv(0, "disposition", zmime_disposition(m), result);
        ___json_add_kv(0, "name", zmime_name(m), result);
        ___json_add_kv(0, "name_utf8", zmime_name_utf8(m), result);
        ___json_add_kv(0, "filename", zmime_filename(m), result);
        ___json_add_kv(0, "filename_star", zmime_filename2231(m, &e), result);
        ___json_add_kv(0, "filename_utf8", zmime_filename_utf8(m), result);
        ___json_add_kv(0, "content_id", zmime_content_id(m), result);
        ___json_add_kv(0, "encoding", zmime_encoding(m), result);
        ___json_add_kv(0, "boundary", zmime_boundary(m), result);
        sprintf(buf, "%zd,%zd,%zd,%zd", zmime_header_offset(m), zmime_header_size(m)
                , zmime_body_offset(m), zmime_body_size(m));
        ___json_add_kv(0, "offset", buf, result);
        zbuf_strcat(result, "}");
    } ZVECTOR_WALK_END;
    zbuf_strcat(result, "]");
}

static void ___json_add_text(int first, char *key, zvector_t *vec, zbuf_t * result)
{
    int len;
    zmime_t *mime;
    int f2 = 1;
    zbuf_t *zout;

    if ((!vec)||(!ZVECTOR_LEN(vec))) {
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
    ZVECTOR_WALK_BEGIN(vec, mime) {
        if (!f2) {
            zbuf_strcat(result, ",");
        }
        zbuf_reset(zout);
        len = zmime_decoded_content_utf8(mime, zout);
        if (len < 0) {
            continue;
        }
        zbuf_strcat(result, "{");
        ___json_add_kv(1, "type", zmime_type(mime), result);
        ___json_add_kv(0, "content", ZBUF_DATA(zout), result);
        zbuf_strcat(result, "}");
        f2 = 0;
    } ZVECTOR_WALK_END;
    zbuf_strcat(result, "]");
    zbuf_free(zout);
}

void zmail_parser_show_json(zmail_t * parser, zbuf_t * result)
{
    zbuf_strcat(result, "{");

    ___json_add_kv(1, "date", zmail_date(parser), result);
    ___json_add_kv(0, "subject", zmail_subject(parser), result);
    ___json_add_kv(0, "subject_utf8", zmail_subject_utf8(parser), result);

    ___json_add_addr(0, "from", zmail_from(parser), result);
    ___json_add_addr_list(0, "to", zmail_to(parser), result);
    ___json_add_addr_list(0, "cc", zmail_cc(parser), result);
    ___json_add_addr_list(0, "bcc", zmail_bcc(parser), result);

    ___json_add_kv(0, "message_id", zmail_message_id(parser), result);

    ___json_add_addr(0, "sender", zmail_sender(parser), result);
    ___json_add_addr(0, "reply_to", zmail_reply_to(parser), result);
    ___json_add_addr(0, "receipt", zmail_receipt(parser), result);

    ___json_add_references(0, "references", (zvector_t *)(zmail_references(parser)), result);
    ___json_add_kv(0, "in_reply_to", zmail_in_reply_to(parser), result);

    ___json_add_mime(0, "mime", (zvector_t *)zmail_all_mimes(parser), result);

    ___json_add_text(0, "text", (zvector_t *)zmail_text_mimes(parser), result);

    zbuf_strcat(result, "}");
}
