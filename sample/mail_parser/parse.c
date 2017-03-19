/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-11
 * ================================
 */

#include "zc.h"

static int enable_tnef = 0;
static int enable_mpool = 0;
static int enable_att = 0;
static int enable_json = 0;

static void ___usage(char *parameter)
{
    printf("USAGE: %s -f eml_filename [ eml_filename2 ... ] [ -tnef ] [ -mpool ] [-att ] [-json] \n", zvar_progname);
    exit(1);
}

static int save_att_tnef(ztnef_t * parser, ztnef_mime_t * mime, int i)
{
    char *sname, *p;
    char tmpname[256];

    sname = mime->filename_utf8;
    if (ZEMPTY(sname)) {
        sprintf(tmpname, "atts/tnef_unknown_%d.dat", i);
    } else {
        snprintf(tmpname, 255, "atts/tnef_%s", sname);
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
    if (zfile_put_contents(tmpname, ztnef_parser_data(parser) + ztnef_mime_body_offset(mime), ztnef_mime_body_size(mime)) < 0) {
        printf("tnef_parser_get_mime_body: save %m\n");
    }

    return 0;
}

static int save_att(zmail_t * parser, zmime_t * mime, int i)
{
    char *sname, *p;
    char tmpname[256];

    sname = mime->name_utf8;
    if (ZEMPTY(sname)) {
        sname = mime->filename_utf8;
    }
    if (ZEMPTY(sname)) {
        sprintf(tmpname, "atts/unknown_%d.dat", i);
    } else {
        snprintf(tmpname, 255, "atts/%s", sname);
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
    ret = zmime_decoded_content_utf8(mime, out);

    printf("save attachment %s\n", tmpname);
    if (ret < 0) {
        printf("mail_decode_mime_body: error\n");
    } else if (zfile_put_contents(tmpname, ZBUF_DATA(out), ret) < 0) {
        printf("mail_decode_mime_body: save %m\n");
    }

    if (enable_tnef && zmime_is_tnef(mime)) {
        int j = 0;
        ztnef_t *tnef_parser;
        ztnef_mime_t *m;
        tnef_parser = ztnef_parser_create(ZBUF_DATA(out), ret);

        ZVECTOR_WALK_BEGIN(ztnef_all_mimes(tnef_parser), m) {
            save_att_tnef(tnef_parser, m, j + 1);
        } ZVECTOR_WALK_END;
        ztnef_parser_free(tnef_parser);
    }
    zbuf_free(out);
    return 0;
}

static int save_all_attachments(zmail_t * parser)
{
    int i = 0;
    zmime_t *m;
    ZVECTOR_WALK_BEGIN(parser->attachment_mimes, m) {
        i++;
        save_att(parser, m, i);
    } ZVECTOR_WALK_END;

    return 0;
}

static void do_parse(char *eml_fn)
{
    zmmap_reader_t reader;
    
    if (zmmap_reader_init(&reader, eml_fn) < 0) {
        printf("ERR: open %s (%m)\n", eml_fn);
        exit(1);
    }

    zmpool_t *mp = 0;
    if (enable_mpool) {
        mp = zmpool_create_greedy_pool();
    }

    zmail_t *parser = zmail_parser_create_MPOOL(mp, reader.data, reader.len);
    zmail_parser_run(parser);

    if (enable_json) {
        zbuf_t *json = zbuf_create(102400);
        zmail_parser_show_json(parser, json);
        fwrite(ZBUF_DATA(json), 1, ZBUF_LEN(json), stdout);
        printf("\n");
        zbuf_free(json);
    } else {
        zmail_parser_show(parser);
    }

    if (enable_att) {
        save_all_attachments(parser);
    }

    zmail_parser_free(parser);
    zmmap_reader_fini(&reader);

    if (enable_mpool) {
        zmpool_free_pool(mp);
    }
}

int main(int argc, char **argv)
{
    zvar_progname = argv[0];
    zvector_t *fn_vec = zvector_create(128);
    char *eml_fn;

    ZPARAMETER_BEGIN() {
        if (!strcmp(optname, "-tnef")) {
            enable_tnef = 1;
            opti+=1;
            continue;
        }
        if (!strcmp(optname, "-mpool")) {
            enable_mpool = 1;
            opti+=1;
            continue;
        }
        if (!strcmp(optname, "-att")) {
            enable_att = 1;
            opti+=1;
            continue;
        }
        if (!strcmp(optname, "-json")) {
            enable_json = 1;
            opti+=1;
            continue;
        }
        if (!optval) {
            ___usage(0);
        }
        if (!strcmp(optname, "-f")) {
            if (optval_count < 1) {
                ___usage(optname);
            }
            int i;
            for (i=0;i<optval_count;i++) {
                zvector_add(fn_vec, argv[opti + 1 + i]);
            }
            opti += 1 + optval_count;
            continue;
        }
    }
    ZPARAMETER_END;

    if (ZVECTOR_LEN(fn_vec) < 1) {
        ___usage(0);
    }

    ZVECTOR_WALK_BEGIN(fn_vec, eml_fn) {
        do_parse(eml_fn);
    } ZVECTOR_WALK_END;

    zvector_free(fn_vec);

    return 0;
}
