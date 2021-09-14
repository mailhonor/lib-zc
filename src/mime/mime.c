/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-10
 * ================================
 */

#include "zc.h"
#include "mime.h"

#if 1
#define _DEV_DEUBG 1
#endif

typedef struct boundary_line_t boundary_line_t;
struct boundary_line_t {
    int offset;
    int len;
    int part_offset;
};

typedef struct mail_parser_node_t mail_parser_node_t;
struct mail_parser_node_t {
    zmime_t *cmime;
    int bls_idx;
    int bls_len;
    char *mail_data;
    char *mail_pos;
    int mail_size;
};

typedef struct mail_parser_context_t mail_parser_context_t;
struct mail_parser_context_t {
    mail_parser_node_t *cnode;
    zvector_t *node_vec;
    char *part_mail_data;
    int part_mail_size;
    boundary_line_t *bls_ptr;
    int bls_idx;
    int bls_len;
    char tmp_buf_prefix[6];
    char tmp_buf[zvar_mime_header_line_max_length];
    char tmp_buf_suffix[6];
};

/* ################################################################## */
inline __attribute__((always_inline)) char *my_find_boundary_prefix(char *s, int len)
{
    char *p;
#if 0
    while(len>3) {
        p = memchr(s, '\n', len);
        if (!p) {
            break;
        }
        if ((p[1] != '-') || (p[2]!='-')) {
            len = len - (p - s) - 1;
            s = p + 1;
            p = 0;
        } else {
            break;
        }
    }
    return p;
#else
    while(len>3) {
        p = memchr(s, '\n', len);
        if (!p) {
            break;
        }
        if (p[2] != '-' ) {
            len = len - (p - s) - 2;
            s = p + 2;
            p = 0;
            continue;
        }
        if (p[1] != '-') {
            if (p[1] != '\n') {
                len = len - (p - s) - 3;
                s = p + 3;
                p = 0;
                continue;
            }
            if (p[3] == '-') {
                p++;
                break;
            }
            len = len - (p - s) - 3;
            s = p + 3;
            p = 0;
            continue;
        }
        break;
    }
    return p;
#endif
}

static int ___get_header_line(mail_parser_node_t * cnode, char **ptr)
{
    char *pbegin = cnode->mail_pos;
    char *pend = cnode->mail_data + cnode->mail_size;
    char *ps, *p;
    int len = 0;

    *ptr = pbegin;
    if (pbegin >= pend) {
        return 0;
    }
    if (pbegin[0] == '\n') {
        cnode->mail_pos += 1;
        return 0;
    }
    if (pend > pbegin) {
        if ((pbegin[0] == '\r') && (pbegin[1] == '\n')) {
            cnode->mail_pos += 2;
            return 0;
        }
    }

    ps = pbegin;
    while (pend > ps) {
        p = (char *)memchr(ps, '\n', pend - ps);
        if ((!p) || (p + 1 == pend)) {
            /* not found or to end */
            len = pend - pbegin;
            break;
        }
        if ((p[1] == ' ') || (p[1] == '\t')) {
            ps = p + 1;
            continue;
        }
        len = p - pbegin + 1;
        break;
    }

    cnode->mail_pos += len;
    return len;
}

/* ################################################################## */
static void deal_content_type(zmail_t * parser, zmime_t * cmime, char *buf, int len)
{
    if (!ZEMPTY(cmime->type)) {
        return;
    }
    int blen = 0;
    zmime_header_line_decode_content_type_inner(parser, buf, len, &(cmime->type), &(cmime->boundary), &blen, &(cmime->charset), &(cmime->name));
    cmime->boundary_len = blen;
    zstr_tolower(cmime->type);
    cmime->type_flag = 1;
}

/* ################################################################## */
static zbool_t zmail_decode_mime_read_header_line(zmail_t *parser, mail_parser_context_t *ctx)
{
    mail_parser_node_t *cnode = ctx->cnode;
    zmime_t *cmime = cnode->cmime, *pmime = cmime->parent;
    char *line, *buf = ctx->tmp_buf;
    int llen, safe_llen;
    safe_llen = llen = ___get_header_line(cnode, &line);
    if (llen == 0) {
        cmime->header_len = cnode->mail_pos - cnode->mail_data;
        cmime->body_offset = cnode->mail_pos - parser->mail_data;

        if ((cmime->type[0] != 'm') || (cmime->type[1] != 'u') || (memcmp(cmime->type, "multipart/", 10))) {
            cmime->is_multipart = 0;
        } else {
            cmime->is_multipart = 1;
        }
        if (pmime) {
            if (pmime->child_head == 0) {
                pmime->child_head = cmime;
                pmime->child_tail = cmime;
            } else {
                pmime->child_tail->next = cmime;
                cmime->prev = pmime->child_tail;
                pmime->child_tail = cmime;
            }
        }
        return 0;
    }
    if (safe_llen > zvar_mime_header_line_max_length) {
        safe_llen = zvar_mime_header_line_max_length - 2;
    }
    if (1) {
        zsize_data_t *sd = (zsize_data_t *)zmpool_malloc(parser->mpool, sizeof(zsize_data_t));
        sd->size = llen;
        sd->data = line;
        zvector_push(&(cmime->raw_header_lines), sd);
    }

    if (!(cmime->type_flag)) {
        if ((llen > 12) && (line[7]=='-') && (line[12]==':') && (!strncasecmp(line, "Content-Type:", 13))) {
            int rlen = zmime_raw_header_line_unescape_inner(parser, line, safe_llen, buf, zvar_mime_header_line_max_length);
            deal_content_type(parser, cmime, buf + 13, rlen - 13);
        }
    }
    return 1;
}

static void zmail_decode_mime_prepare_node(zmail_t *parser, mail_parser_context_t *ctx)
{
    if (parser->top_mime) {
        if (ctx->part_mail_size < 1) {
            return;
        }

        char *data = ctx->part_mail_data;
        int i, ch, len = ctx->part_mail_size;
        for (i=0;i<len;i++) {
            ch = data[i];
            if ((ch =='\r')||(ch == '\n') || (ch==' ') || (ch == '\t')) {
                continue;
            }
            break;
        }
        if (i == len) {
            return;
        }
    } else {
        ctx->cnode = 0;
    }
    zmime_t *nmime = zmime_create(parser);
    if (ctx->cnode && ctx->cnode->cmime) {
        nmime->parent = ctx->cnode->cmime;
    }
    zvector_push(parser->all_mimes, nmime);
    mail_parser_node_t * nnode = (mail_parser_node_t *)zcalloc(1, sizeof(mail_parser_node_t));
    nnode->cmime = nmime;
    nnode->bls_idx = ctx->bls_idx;
    nnode->bls_len = ctx->bls_len;
    nnode->mail_data = ctx->part_mail_data;
    nnode->mail_pos = nnode->mail_data;
    nnode->mail_size = ctx->part_mail_size;
    zvector_push(ctx->node_vec, nnode);
}

static int zmail_decode_mime_get_all_boundary(zmail_t *parser, boundary_line_t **_bls_ptr)
{
    int bls_size = 100, bls_len = 0, len;
    boundary_line_t *bls, *bls_ptr = (boundary_line_t *)zcalloc((bls_size+1), sizeof(boundary_line_t));
    char *ps = parser->mail_data, *pend = parser->mail_data + parser->mail_size, *p;

    if (parser->mail_size > 0) {
        if (ps[0] == '\n') {
            return 0;
        }
    }
    if (parser->mail_size > 1) {
        if ((ps[0] == '\r') && (ps[1] == '\n')) {
            return 0;
        }
    }

    while(ps < pend) {
        len = pend - ps;
        if (len < 3) {
            break;
        }
        if (ps[1]!='-') {
            p = memchr(ps+1, '\n', len-1);
            if (p) {
                ps = p + 1;
            } else {
                ps += len;
            }
            continue;
        }
        if (ps[0]!='-') {
            p = memchr(ps, '\n', len);
            if (p) {
                ps = p + 1;
            } else {
                ps += len;
            }
            continue;
        }

        if (bls_len == bls_size) {
            bls_size *= 2;
            bls_ptr = (boundary_line_t *)zrealloc(bls_ptr, (bls_size+1)*sizeof(boundary_line_t));
        }
        bls = bls_ptr + bls_len;
        bls_len ++;
        p = memchr(ps, '\n', len);
        if (p) {
            len = p - ps + 1;
        }
        bls->offset = (ps - parser->mail_data);
        bls->len = len;
        bls->part_offset = (ps + len - parser->mail_data);
        ps += len;
    }
    *_bls_ptr = bls_ptr;
    return bls_len;
}

static mail_parser_context_t *zmail_decode_mime_prepare_context(zmail_t *parser)
{
    mail_parser_context_t *ctx = (mail_parser_context_t *)zcalloc(sizeof(mail_parser_context_t), 1);

    ctx->node_vec = zvector_create(32); /* mail_parser_node_t */

    ctx->bls_idx = 0;
    ctx->bls_len = zmail_decode_mime_get_all_boundary(parser, &(ctx->bls_ptr));
    ctx->part_mail_data = parser->mail_data;
    ctx->part_mail_size = parser->mail_size;

    zmail_decode_mime_prepare_node(parser,  ctx);

    parser->top_mime = ((mail_parser_node_t*)(zvector_data(ctx->node_vec)[0]))->cmime;

    return ctx;
}

static void zmail_decode_mime_free_context(mail_parser_context_t *ctx)
{
    zfree(ctx->bls_ptr);
    zvector_free(ctx->node_vec);
    zfree(ctx);
}

int zmail_decode_mime_inner(zmail_t * parser)
{
    mail_parser_context_t *ctx = zmail_decode_mime_prepare_context(parser);
    mail_parser_node_t *cnode = 0, *cnode_last = 0;
    while (zvector_len(ctx->node_vec)) {
        if (cnode_last) {
            zfree(cnode_last);
        }
        cnode_last = cnode = ctx->cnode = (mail_parser_node_t *)(zvector_data(ctx->node_vec)[zvector_len(ctx->node_vec)-1]);
        int new_len = zvector_len(ctx->node_vec) - 1;
        zvector_truncate(ctx->node_vec, new_len);
        zmime_t *cmime = cnode->cmime; 

        /* header */
        cmime->header_offset = cnode->mail_data - parser->mail_data;
        while (zmail_decode_mime_read_header_line(parser, ctx));
        if (cmime->type_flag == 0) {
            cmime->type = zmpool_strdup(parser->mpool, "text/plain");
            cmime->type_flag = 1;
        }
        /* body */
        cmime->body_len = cnode->mail_data + cnode->mail_size - parser->mail_data - cmime->body_offset;
        if (!cmime->is_multipart) {
            continue;
        }
        if (cnode->bls_len == 0) {
            ctx->bls_idx = -1;
            ctx->bls_len = 0;
            ctx->part_mail_data = parser->mail_data+cmime->body_offset;
            ctx->part_mail_size = cmime->body_len;
            zmail_decode_mime_prepare_node(parser, ctx);
            continue;
        }

        boundary_line_t *bls, *bls1, *bls2;
        bls1 = 0;
        bls2 = 0;
        int count = 0;
        for (int bls_idx = 0; bls_idx < cnode->bls_len; bls_idx++) {
            bls = ctx->bls_ptr + (cnode->bls_idx + bls_idx);
            int len = bls->len - 2;
            char *boundary = parser->mail_data + bls->offset + 2;

            if ((cmime->boundary_len > len) || (!ZSTR_N_EQ(cmime->boundary, boundary, cmime->boundary_len))) {
                continue;
            }
            count ++;
            bls1 = bls2;
            bls2 = bls;
            if (bls1 && bls2) {
                int i1 = (bls1 - ctx->bls_ptr) + 1;
                int i2 = (bls2 - ctx->bls_ptr);

                if (i1 <= i2){
                    ctx->bls_idx= i1;
                    ctx->bls_len = i2 - i1 + 1;
                } else {
                    ctx->bls_idx= -1;
                    ctx->bls_len = 0;
                }
                ctx->part_mail_data = parser->mail_data + bls1->part_offset;
                ctx->part_mail_size = bls2->offset - bls1->part_offset;
                zmail_decode_mime_prepare_node(parser, ctx);
            }
        }
        if (count == 0) {
            continue;
        }
        int len = bls2->len - 2;
        char *boundary = parser->mail_data + bls2->offset + 2;
        if (len - 2 >= cmime->boundary_len) {
            if ((boundary[len-2] == '-') && (boundary[len-1] == '-')) {
                continue;
            }
        }

#if 1
        ctx->bls_idx = (bls2 - ctx->bls_ptr) + 1;
        ctx->bls_len = cnode->bls_idx + cnode->bls_len - ctx->bls_idx;

        if (ctx->bls_len < 0) {
            ctx->bls_idx = -1;
            ctx->bls_len = 0;
        }
#else
        ctx->bls_idx = -1;
        ctx->bls_len = 0;
#endif

        ctx->part_mail_data = parser->mail_data + bls2->part_offset;
        ctx->part_mail_size = cnode->mail_data + cnode->mail_size - (parser->mail_data + bls2->part_offset);
        zmail_decode_mime_prepare_node(parser, ctx);
    }
    zfree(cnode_last);
    zmail_decode_mime_free_context(ctx);

    ZVECTOR_WALK_BEGIN(parser->all_mimes, zmime_t *, cmime) {
        int idx = cmime->body_offset + cmime->body_len;
        if ((cmime->body_len > 0) && (parser->mail_data[idx - 1] == '\n')) { idx--; cmime->body_len--; }
        if ((cmime->body_len > 0) && (parser->mail_data[idx - 1] == '\r')) { cmime->body_len--; }
    } ZVECTOR_WALK_END;

    return 0;
}
