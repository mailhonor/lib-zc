/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-11
 * ================================
 */

#include "zc.h"
#include <ctype.h>

static void ___usage()
{
    zprintf("USAGE: %s [ -loop 1000 ] [--onlymime ] eml_filename\n", zvar_progname);
    exit(1);
}

static char hunman_buf[100];
static char *hunman_size2(long a)
{
    char buf[300], *p = buf, ch;
    int len, m, i;
    int tl = 0;

    hunman_buf[0] = 0;
    zsprintf(buf, "%ld", a);
    len = strlen(buf);
    m = len % 3;

    while (1) {
        for (i = 0; i < m; i++) {
            ch = *p++;
            if (ch == '\0') {
                goto over;
            }
            hunman_buf[tl++] = ch;
        }
        hunman_buf[tl++] = ',';
        m = 3;
    }

  over:
    hunman_buf[tl] = 0;
    len = strlen(hunman_buf);
    if (len > 0) {
        if (hunman_buf[len - 1] == ',') {
            hunman_buf[len - 1] = 0;
        }
    }
    if (hunman_buf[0] == ',') {
        return hunman_buf + 1;
    }

    return hunman_buf;
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    char *eml_fn = 0, *eml_data;
    int times = 1000, i, eml_size;
    zbuf_t *eml_data_buf = zbuf_create(102400);
    long t;

    times = zconfig_get_int(zvar_default_config, "loop", 1000);
    int onlymime = zconfig_get_bool(zvar_default_config, "onlymime", 0);

    if (zvar_main_redundant_argc == 0) {
        ___usage();
    }
    eml_fn = zvar_main_redundant_argv[0];

    zfile_get_contents_sample(eml_fn, eml_data_buf);
    eml_data = zbuf_data(eml_data_buf);
    eml_size = zbuf_len(eml_data_buf);

    zprintf("eml     : %s\n", eml_fn);
    zprintf("size    : %d(bytes)\n", eml_size);
    zprintf("loop    : %d\n", times);
    zprintf("total   : %s(bytes)\n", hunman_size2((long)eml_size * times));

    t = ztimeout_set_millisecond(0);
    for (i = 0; i < times; i++) {
        zmail_t *mp = zmail_create_parser_from_data(eml_data, eml_size, "");
        if (!onlymime) {
            zmail_get_message_id(mp);
            zmail_get_subject(mp);
            zmail_get_subject_utf8(mp);
            zmail_get_date(mp);
            zmail_get_in_reply_to(mp);
            zmail_get_date_unix(mp);
            zmail_get_from(mp);
            zmail_get_from_utf8(mp);
            zmail_get_sender(mp);
            zmail_get_reply_to(mp);
            zmail_get_receipt(mp);
            zmail_get_to(mp); /* zmime_address_t* */
            zmail_get_to_utf8(mp);
            zmail_get_cc(mp);
            zmail_get_cc_utf8(mp);
            zmail_get_bcc(mp);
            zmail_get_bcc_utf8(mp);
            zmail_get_references(mp);
            zmail_get_top_mime(mp);
            zmail_get_all_mimes(mp); /* zmime_t* */
            zmail_get_text_mimes(mp);
            zmail_get_show_mimes(mp);
            zmail_get_attachment_mimes(mp);
        }
        zmail_free(mp);
    }
    t = ztimeout_set_millisecond(0) - t;

    zprintf("elapse  : %ld.%03ld(second)\n", t / 1000, t % 1000);
    zprintf("%%second : %s(bytes)\n", hunman_size2((long)(((long)eml_size * times) / ((1.0 * t) / 1000))));

    return 0;
}
