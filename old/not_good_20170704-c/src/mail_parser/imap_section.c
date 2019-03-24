/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-04-28
 * ================================
 */

#include "zc.h"

static inline int ___child_count(zmime_t * mime)
{
    int count;

    count = 0;
    for (mime = mime->child; mime; mime = mime->next) {
        count++;
    }

    return count;
}

void ___mime_section(zmail_t * parser, zmime_t * mime, char *section)
{
    zmpool_t *mp = parser->mpool;
    int i, k, count;
    char nsection[10240], intbuf[16];
    zmime_t *cm, *fm;
    zargv_t *zav;
    ZSTACK_BUF(zb, 10240);

    mime->imap_section = zmpool_strdup(mp, section);

    zav = zargv_create(1);
    zargv_split_append(zav, section, ".");
    count = ___child_count(mime);
    fm = mime->child;
    cm = fm;
    for (i = 0; i < count; i++, cm = cm->next) {
        if (!strcmp(section, "")) {
            sprintf(nsection, "%d", i + 1);
        } else if ((ZARGV_LEN(zav) > 1) && (strncasecmp(fm->type, "multipart/", 11))) {
            zbuf_reset(zb);
            for (k = 0; k < ZARGV_LEN(zav) - 1; k++) {
                zbuf_strcat(zb, ZARGV_ARGV(zav)[k]);
                zbuf_strcat(zb, ".");
            }
            sprintf(intbuf, "%d", i + 1);
            zbuf_strcat(zb, intbuf);
            strcpy(nsection, ZBUF_DATA(zb));
        } else {
            if (!strcmp(ZARGV_ARGV(zav)[ZARGV_LEN(zav) - 1], "0")) {
                zbuf_reset(zb);
                for (k = 0; k < ZARGV_LEN(zav) - 1; k++) {
                    zbuf_strcat(zb, ZARGV_ARGV(zav)[k]);
                    zbuf_strcat(zb, ".");
                }
                sprintf(intbuf, "%d", i + 1);
                zbuf_strcat(zb, intbuf);
                strcpy(nsection, ZBUF_DATA(zb));
            } else {
                strcpy(nsection, section);
            }
        }
        if (___child_count(cm)) {
            if (!((!strncasecmp(cm->type, "message/", 8)) && (!strncasecmp(cm->child->type, "multipart/", 11)))) {
                strcat(nsection, ".0");
            }
        }
        ___mime_section(parser, cm, nsection);
    }
    zargv_free(zav);
}

void zmail_set_imap_section(zmail_t * parser)
{
    if (parser->section_flag) {
        return;
    }
    parser->section_flag = 1;
    ___mime_section(parser, parser->top_mime, "0");
}
