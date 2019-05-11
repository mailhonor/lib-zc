
/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-01-13
 * ================================
 */

#include "zc.h"
#include "mime.h"

static void _debug_show_addr(zmail_t *parser, const char *h, const zmime_address_t *addr)
{
    const char *fmt = "%15s: %s\n";
    zbuf_t *tmpstr = zmail_zbuf_cache_require(parser, 128); 
    zmail_get_raw_header_line(parser, h, tmpstr, 0);
    printf(fmt, h, zbuf_data(tmpstr));
    printf(fmt, "", addr->address);
    printf(fmt, "", addr->name);
    printf(fmt, "", addr->name_utf8);
    zmail_zbuf_cache_release(parser, tmpstr);
}

static void _debug_show_addr_vector(zmail_t *parser, const char *h, const zvector_t *address_vec)
{
    const char *fmt = "%15s: %s\n";
    char nh[256];
    int i = 0;

    zbuf_t *tmpstr = zmail_zbuf_cache_require(parser, 128); 
    zmail_get_raw_header_line(parser, h, tmpstr, 0);
    printf(fmt, h, zbuf_data(tmpstr));
    
    ZVECTOR_WALK_BEGIN(address_vec, zmime_address_t *, addr) {
        if (i == 0) {
            sprintf(nh, "%s (1)", h);
        } else {
            sprintf(nh, "(%d)", i+1);
        }
        i ++;
        printf(fmt, nh, addr->address);
        printf(fmt, "", addr->name);
        printf(fmt, "", addr->name_utf8);
    } ZVECTOR_WALK_END;
    zmail_zbuf_cache_release(parser, tmpstr);
}

void zmail_debug_show(zmail_t *parser)
{
    const char *fmt = "%15s: %s\n";
    int i = 0;
    zbuf_t *tmpstr = zmail_zbuf_cache_require(parser, 128); 

    printf(fmt, "Date", zmail_get_date(parser));
    printf("%15s: %ld\n","", zmail_get_date_unix(parser));
    printf("\n");
    printf(fmt, "Subject", zmail_get_subject(parser));
    printf(fmt, "", zmail_get_subject_utf8(parser));
    printf("\n");
    _debug_show_addr(parser, "From", zmail_get_from_utf8(parser));
    printf("\n");
    _debug_show_addr(parser, "Sender", zmail_get_sender(parser));
    printf("\n");
    _debug_show_addr(parser, "Reply-To", zmail_get_reply_to(parser));
    printf("\n");
    _debug_show_addr(parser, "Disposition-Notification-To", zmail_get_receipt(parser));
    printf("\n");
    printf(fmt, "In-Reply-To", zmail_get_in_reply_to(parser));
    printf("\n");
    _debug_show_addr_vector(parser, "To", zmail_get_to_utf8(parser));
    printf("\n");
    _debug_show_addr_vector(parser, "Cc", zmail_get_cc_utf8(parser));
    printf("\n");
    _debug_show_addr_vector(parser, "Bcc", zmail_get_bcc_utf8(parser));
    printf("\n");
    printf(fmt, "Message-Id", zmail_get_message_id(parser));

    {
        i = 0;
        printf("\n");
        zmail_get_raw_header_line(parser, "References", tmpstr, 0);
        printf(fmt, "References", zbuf_data(tmpstr));
        const zargv_t *references = zmail_get_references(parser);
        ZARGV_WALK_BEGIN(references, r) {
            if (i==0) {
                printf(fmt, "References", r);
            } else {
                printf(fmt, "", r);
            }
            i++;
        } ZARGV_WALK_END;
    }

    const zvector_t *allm = zmail_get_all_mimes(parser);
    i = 0;
    ZVECTOR_WALK_BEGIN(allm, zmime_t *, m) {
        printf("\n");
        char buf[128];
        sprintf(buf, "Mime (%d)", i+1);
        printf(fmt, buf, zmime_get_type(m));
        zmime_get_raw_header_line(m, "Content-Type", tmpstr, 0);
        printf(fmt, "Content-Type", zbuf_data(tmpstr));
        zmime_get_raw_header_line(m, "Content-Transfer-Encoding", tmpstr, 0);
        printf(fmt, "Content-Transfer-Encoding", zbuf_data(tmpstr));
        zmime_get_raw_header_line(m, "Content-Disposition", tmpstr, 0);
        printf(fmt, "Content-Disposition", zbuf_data(tmpstr));
        printf(fmt, "disposition", zmime_get_disposition(m));
        printf(fmt, "name", zmime_get_name(m));
        printf(fmt, "name_utf8", zmime_get_name_utf8(m));
        printf(fmt, "filename", zmime_get_filename(m));
        printf(fmt, "filename2231", zmime_get_filename2231(m, 0));
        printf(fmt, "filename_utf8", zmime_get_filename_utf8(m));
        printf(fmt, "content_id", zmime_get_content_id(m));
        printf(fmt, "encoding", zmime_get_encoding(m));
        printf(fmt, "boundary", zmime_get_boundary(m));
        printf(fmt, "section", zmime_get_imap_section(m));
        sprintf(buf, "%d,%d,%d,%d", zmime_get_header_offset(m), zmime_get_header_len(m), zmime_get_body_offset(m), zmime_get_body_len(m));
        printf(fmt, "", buf);
        i++;
    } ZVECTOR_WALK_END;

    i = 0;
    const zvector_t *textm = zmail_get_text_mimes(parser);
    ZVECTOR_WALK_BEGIN(textm, zmime_t *, m) {
        printf("\n");
        zmime_get_decoded_content_utf8(m, tmpstr);
        printf(fmt, zmime_get_type(m),  zbuf_data(tmpstr));
    } ZVECTOR_WALK_END;

    zmail_zbuf_cache_release(parser, tmpstr);
}
