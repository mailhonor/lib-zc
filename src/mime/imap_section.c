/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "zc.h"
#include "mime.h"

static inline int ___child_head_count(zmime_t * m)
{
    int count = 0;
    for (m = m->child_head; m; m = m->next) {
        count++;
    }
    return count;
}

typedef struct recursion_info_t recursion_info_t;
struct recursion_info_t {
    recursion_info_t *prev;
    recursion_info_t *next;
    zmime_t *mime;
    zmime_t *child_head_mime;
    zbuf_t *section_bf;
    zbuf_t *nsection;
    zargv_t *argv;
    int child_head_count;
    int i;
};

void zmail_set_imap_section(zmail_t *parser)
{
    if (parser->section_flag) {
        return;
    }
    parser->section_flag = 1;

    char intbuf[16];
    int child_head_count;
    zmime_t *mime, *cm, *fm;
    char *section;
    zbuf_t *nsection, *top_section_bf;
    zargv_t *argv;
    recursion_info_t *rhead = 0, *rtail = 0, *rnode;
    rnode = (recursion_info_t *)zcalloc(1, sizeof(recursion_info_t));
    ZMLINK_APPEND(rhead, rtail, rnode, prev, next);
    rnode->mime = parser->top_mime;
    rnode->i = -1;
    top_section_bf = rnode->section_bf = zmail_zbuf_cache_require(parser, -1);
    zbuf_puts(rnode->section_bf, "0");

    while(rtail) {
        rnode = rtail;
        mime = rnode->mime;
        section = zbuf_data(rnode->section_bf);
        mime->imap_section = zmpool_memdupnull(parser->mpool, section, zbuf_len(rnode->section_bf));
        child_head_count = ___child_head_count(mime);
        if (child_head_count == 0) {
            ZMLINK_DETACH(rhead, rtail, rnode, prev, next);
            zmail_zbuf_cache_release(parser, rnode->nsection);
            zargv_free(rnode->argv);
            zfree(rnode);
            continue;
        }
        if (rnode->i == -1) {
            rnode->argv = zargv_create(10);
            zargv_split_append(rnode->argv, section, ".");
            rnode->child_head_count = ___child_head_count(mime);
            rnode->child_head_mime = mime->child_head;
            rnode->nsection = zmail_zbuf_cache_require(parser, -1);
        } else {
            rnode->child_head_mime = rnode->child_head_mime->next;
        }
        rnode->i++;
        if (rnode->i == rnode->child_head_count) {
            ZMLINK_DETACH(rhead, rtail, rnode, prev, next);
            zmail_zbuf_cache_release(parser, rnode->nsection);
            zargv_free(rnode->argv);
            zfree(rnode);
            continue;
        }

        argv = rnode->argv;
        fm = mime->child_head;
        cm = rnode->child_head_mime;
        nsection = rnode->nsection;
        if (!strcmp(section, "")) {
            sprintf(intbuf, "%d", rnode->i + 1);
            zbuf_strcpy(nsection, intbuf);
        } else if ((zargv_len(argv)> 1) && (strncasecmp(fm->type, "multipart/", 11))) {
            zbuf_reset(nsection);
            for (int k = 1; k < zargv_len(argv); k++) {
                zbuf_puts(nsection, argv->argv[k-1]);
                ZBUF_PUT(nsection, '.');
            }
            sprintf(intbuf, "%d", rnode->i + 1);
            zbuf_strcat(nsection, intbuf);
        } else {
            if (!strcmp(argv->argv[zargv_len(argv) - 1], "0")) {
                zbuf_reset(nsection);
                for (int k = 1; k < zargv_len(argv); k++) {
                    zbuf_puts(nsection,argv->argv[k-1]);
                    ZBUF_PUT(nsection, '.');
                }
                sprintf(intbuf, "%d", rnode->i + 1);
                zbuf_strcat(nsection, intbuf);
            } else {
                zbuf_strcpy(nsection, section);
            }
        }
        if (cm->child_head) {
            if (!((!strncasecmp(cm->type, "message/", 8))
                        && (!strncasecmp(cm->child_head->type, "multipart/", 11)))) {
                zbuf_strcat(nsection, ".0");
            }
        }
        rnode = (recursion_info_t *)zcalloc(1, sizeof(recursion_info_t));
        ZMLINK_APPEND(rhead, rtail, rnode, prev, next);
        rnode->mime = cm;
        rnode->i = -1;
        rnode->section_bf = nsection;
    }

    // 只有一个 part 且不是 multipart
    mime = parser->top_mime;
    if (mime->child_head == 0) {
        if (strncasecmp(mime->type, "multipart/", 11)) {
            mime->imap_section[0] = '1';
        }
    }

    zmail_zbuf_cache_release(parser, top_section_bf);
}
