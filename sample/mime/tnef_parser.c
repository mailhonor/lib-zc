/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-11
 * ================================
 */

#include "zc.h"

static int enable_att = 0;

static char *name_char_validate(char *abc)
{
    char key[] = "?<>\"'|/\\*", *p = abc, ch;

    while((ch=*p)) {
        if (strchr(key, ch)) {
            *p = ' ';
        }
        p++;
    }
    return abc;

}

static int save_att(ztnef_t * parser, ztnef_mime_t * mime, int i)
{
    const char *sname;
    char tmpname[256];

    sname = ztnef_mime_get_show_name(mime);
    if (zempty(sname)) {
        sprintf(tmpname, "atts/unknown_%d.dat", i);
    } else {
        snprintf(tmpname, 255, "atts/%s", sname);
        name_char_validate(tmpname+5);
    }

    printf("save attachment %s\n", tmpname);
    if (!zfile_put_contents(tmpname, ztnef_mime_get_body_data(mime), ztnef_mime_get_body_len(mime))) {
        printf("ERR decode_mime_body: save %m\n");
    }

    return 0;
}

static int save_all_attachments(ztnef_t *parser)
{
    int i = 0;
    const zvector_t *allm = ztnef_get_all_mimes(parser);
    ZVECTOR_WALK_BEGIN(allm, ztnef_mime_t *, m) {
        i++;
        save_att(parser, m, i);
    } ZVECTOR_WALK_END;
    return 0;
}

static void do_parse(char *eml_fn)
{
    ztnef_t *parser = ztnef_create_parser_from_pathname(eml_fn, "");
    if (parser == 0) {
        printf("ERR open %s (%m)\n", eml_fn);
        exit(1);
    }
    ztnef_debug_show(parser);

    if (enable_att) {
        save_all_attachments(parser);
    }

    ztnef_free(parser);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    enable_att = zconfig_get_bool(zvar_default_config, "att", 0);

    if (zvar_main_redundant_argc == 0) {
        printf("USAGE: %s [--att] tnef_fn...\n", zvar_progname);
        exit(0);
    }
    for (int i = 0; i < zvar_main_redundant_argc; i++) {
        do_parse(zvar_main_redundant_argv[i]);
    }

    return 0;
}
