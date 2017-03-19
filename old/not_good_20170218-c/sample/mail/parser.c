/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-11
 * ================================
 */

#include "libzc.h"
#include <ctype.h>

static int save_att_tnef(ztnef_parser_t * parser, ztnef_mime_t * mime, int i)
{
    int ret;
    char *sname, *p;
    char tmpname[256];

    sname = mime->filename_rd;
    if (ZEMPTY(sname)) {
        sprintf(tmpname, "att/tnef_unknown_%d.dat", i);
    } else {
        snprintf(tmpname, 255, "att/tnef_%s", sname);
        p = tmpname + 4;
        while (*p) {
            char ch = *p;
            if ((ch == '?') || (ch == '<') || (ch == '>')) {
                *p++ = ' ';
                continue;
            }
            if ((ch == '"') || (ch == '\'') || (ch == '|')) {
                *p++ = ' ';
                continue;
            }
            if ((ch == '/') || (ch == '\\') || (ch == '*')) {
                *p++ = ' ';
                continue;
            }
            p++;
        }
    }
    ret = ztnef_parser_get_mime_body(parser, mime, &p);
    printf("save tnef_attachment %s\n", tmpname);
    if (ret < 0) {
        printf("tnef_parser_get_mime_body: error\n");
    } else if (zfile_put_contents(tmpname, p, ret) < 0) {
        printf("tnef_parser_get_mime_body: save %m\n");
    }

    return 0;
}

static int save_att(zmail_parser_t * parser, zmail_mime_t * mime, int i)
{
    char *sname, *p;
    char tmpname[256];

    sname = mime->name_rd;
    if (ZEMPTY(sname)) {
        sname = mime->filename_rd;
    }
    if (ZEMPTY(sname)) {
        sprintf(tmpname, "att/unknown_%d.dat", i);
    } else {
        snprintf(tmpname, 255, "att/%s", sname);
        p = tmpname + 4;
        while (*p) {
            char ch = *p;
            if ((ch == '?') || (ch == '<') || (ch == '>')) {
                *p++ = ' ';
                continue;
            }
            if ((ch == '"') || (ch == '\'') || (ch == '|')) {
                *p++ = ' ';
                continue;
            }
            if ((ch == '/') || (ch == '\\') || (ch == '*')) {
                *p++ = ' ';
                continue;
            }
            p++;
        }
    }
    zbuf_t *out;
    int ret;
    out = zbuf_create(mime->body_len * 2);
    ret = zmail_parser_decode_mime_body(parser, mime, out);

    printf("save attachment %s\n", tmpname);
    if (ret < 0) {
        printf("mail_parser_decode_mime_body: error\n");
    } else if (zfile_put_contents(tmpname, ZBUF_DATA(out), ret) < 0) {
        printf("mail_parser_decode_mime_body: save %m\n");
    }

    if (mime->is_tnef) {
        int j = 0;
        ztnef_parser_t *tnef_parser;
        tnef_parser = ztnef_parser_create(ZBUF_DATA(out), ret);

        if (ztnef_parser_run(tnef_parser) < 0) {
            printf("can not decode tnef");
        }
        for (j = 0; j < tnef_parser->attachment_mime_count; j++) {
            save_att_tnef(tnef_parser, tnef_parser->attachment_mime_list[j], j + 1);
        }
        ztnef_parser_free(tnef_parser);
    }
    zbuf_free(out);
    return 0;
}

static int save_all_attachments(zmail_parser_t * parser)
{
    int i;
    for (i = 0; i < parser->attachment_mime_count; i++) {
        save_att(parser, parser->attachment_mime_list[i], i);
    }

    return 0;
}

int main(int argc, char **argv)
{
    zmail_parser_t *parser;
    char *eml_fn;
    zmmap_reader_t reader;

    eml_fn = argv[1];
    zmmap_reader_init(&reader, eml_fn);

    zmpool_t *mp = zmpool_create_grow_pool();
    parser = zmail_parser_create_mpool(mp, reader.data, reader.len);
    zmail_parser_run(parser);

    zmail_parser_show(parser);

    //save_all_attachments(parser);

    zmail_parser_free(parser);

    zmmap_reader_fini(&reader);

    return 0;
}
