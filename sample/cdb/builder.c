/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2019-01-23
 * ================================
 */

#include "zc.h"
const char *postifx_config_name_val[] = {
    "biff", "yes",
    "debug_peer_level", "2",
    "debug_peer_list", "",
    "export_environment", "TZ MAIL_CONFIG",
    "ignore_mx_lookup_error", "no",
    "import_environment", "MAIL_CONFIG MAIL_DEBUG MAIL_LOGTAG TZ XAUTHORITY DISPLAY",
    "line_length_limit", "2048",
    "lmtp_lhlo_timeout", "300s",
    "lmtp_quit_timeout", "300s",
    "lmtp_rset_timeout", "20s",
    "lmtp_skip_quit_response", "no",
    "mime_nesting_limit", "100",
    "mynetworks", "127.0.0.0/8 192.168.188.0/24 119.61.7.0/24 ",
    "mynetworks_style", "subnet",
    "qmqpd_timeout", "300s",
    "queue_minfree", "0",
    "setgid_group", "postdrop",
    "smtp_helo_timeout", "300s",
    "smtp_host_lookup", "dns",
    "smtp_line_length_limit", "990",
    "smtp_mx_session_limit", "2",
    "smtp_never_send_ehlo", "no",
    "smtp_quit_timeout", "300s",
    "smtp_rset_timeout", "20s",
    "smtp_skip_5xx_greeting", "yes",
    "smtp_skip_quit_response", "yes",
    "smtp_tls_CAfile", "",
    "smtp_tls_loglevel", "0",
    "smtp_tls_per_site", "",
    "smtp_use_tls", "no",
    "smtpd_error_sleep_time", "1s",
    "smtpd_helo_required", "no",
    "smtpd_history_flush_threshold", "100",
    "smtpd_proxy_filter", "",
    "smtpd_proxy_timeout", "100s",
    "smtpd_soft_error_limit", "10",
    "smtpd_timeout", "300s",
    "smtpd_tls_CAfile", "",
    "smtpd_tls_loglevel", "0",
    "smtpd_use_tls", "no",
    "trigger_timeout", "10s",
    "verp_delimiter_filter", "-",
    0, 0 };

int main(int argc, char **argv)
{
    zcdb_builder_t *builder = zcdb_builder_create();
    printf("USAGE: %s configfile db\n", argv[0]);
    if (argc == 3) {
        FILE *fp = fopen(argv[1], "rb");
        char buf[10240+1], *p, *q;
        while(fgets(buf, 10240, fp)) {
            p = ztrim(buf);
            if (strlen(p) == 0) {
                continue;
            }
            q = strchr(p, '=');
            if (!q) {
                continue;
            }
            *q++ = 0;
            p = ztrim(p);
            zcdb_builder_update(builder, p, -1, q, -1);
        }
        fclose(fp);
        if (zcdb_builder_build(builder, argv[2]) < 1) {
            printf("ERROR: can not build zcdb to %s\n", argv[2]);
            exit(1);
        }
        printf("OK %s\n", argv[2]);
    } else {
        for (const char **pp = postifx_config_name_val; *pp; pp++) {
            const char *key = *pp++;
            const char *val = *pp;
            zcdb_builder_update(builder, key, -1, val, -1);
            /* 或 */
            zcdb_builder_update(builder, key, strlen(key), val, strlen(val));
        }
        if (zcdb_builder_build(builder, "./postfix_conf.zcdb") < 1) {
            printf("ERROR: can not build zcdb to postfix_conf.zcdb\n");
            exit(1);
        }
        printf("OK postfix_conf.zcdb\n");
    }
    zcdb_builder_free(builder);
    return 0;
}
