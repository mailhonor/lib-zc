/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-21
 * ================================
 */

#include "libzc.h"

typedef struct ___mime_list_t ___mime_list_t;
struct ___mime_list_t {
    ztnef_mime_t *head;
    ztnef_mime_t *tail;
};

static int mime_list_add(ztnef_parser_t * parser, ___mime_list_t * mime_list)
{
    ztnef_mime_t *mime = (ztnef_mime_t *) zmpool_calloc(parser->mpool, 1,
                                                        sizeof(ztnef_mime_t));

    ZMLINK_APPEND(mime_list->head, mime_list->tail, mime, all_last, all_next);
    return 0;
}

static int mime_list_pop(___mime_list_t * mime_list, ztnef_mime_t ** mime)
{
    *mime = mime_list->tail;

    ZMLINK_DETACH(mime_list->head, mime_list->tail, mime_list->tail, all_last, all_next);

    if (*mime) {
        return 1;
    }
    return 0;
}

/* ################################################################## */
static void free_one_mime(ztnef_parser_t * parser, ztnef_mime_t * mime)
{
#define _FR(a)  {zmpool_free(parser->mpool, mime->a);}
    _FR(type);
    _FR(filename);
    _FR(filename_rd);
    _FR(content_id);
#undef _FR
    zmpool_free(parser->mpool, mime);
}

/* ################################################################## */

#define TNEF_SIGNATURE 			 0x223e9f78
#define TNEF_LVL_MESSAGE 		 0x01
#define TNEF_LVL_ATTACHMENT 		 0x02

#define TNEF_STRING 			 0x00010000
#define TNEF_TEXT 			 0x00020000
#define TNEF_BYTE 			 0x00060000
#define TNEF_WORD 			 0x00070000
#define TNEF_DWORD 		 	 0x00080000

#define TNEF_ASUBJECT 			 0x8004|TNEF_DWORD
#define TNEF_AMCLASS 			 0x8008|TNEF_WORD
#define TNEF_BODYTEXT	 		 0x800c|TNEF_TEXT
#define TNEF_ATTACHDATA 		 0x800f|TNEF_BYTE
#define TNEF_AFILENAME 			 0x8010|TNEF_STRING
#define TNEF_ARENDDATA 			 0x9002|TNEF_BYTE
#define TNEF_AGRIDIATTRS 		 0x9005|TNEF_BYTE
#define TNEF_AVERSION 			 0x9006|TNEF_DWORD

#define TNEF_GRIDI_NULL 			 0x0001
#define TNEF_GRIDI_SHORT 		 0x0002
#define TNEF_GRIDI_INT 			 0x0003
#define TNEF_GRIDI_FLOAT 		 0x0004
#define TNEF_GRIDI_DOUBLE 		 0x0005
#define TNEF_GRIDI_CURRENCY 		 0x0006
#define TNEF_GRIDI_APPTIME 		 0x0007
#define TNEF_GRIDI_ERROR 		 0x000a
#define TNEF_GRIDI_BOOLEAN 		 0x000b
#define TNEF_GRIDI_OBJECT 		 0x000d
#define TNEF_GRIDI_INT8BYTE 		 0x0014
#define TNEF_GRIDI_STRING 		 0x001e
#define TNEF_GRIDI_UNICODE_STRING 	 0x001f
#define TNEF_GRIDI_SYSTIME 		 0x0040
#define TNEF_GRIDI_CLSID 		 0x0048
#define TNEF_GRIDI_BINARY 		 0x0102

#define TNEF_GRIDI_ATTACH_MIME_TAG 	 0x370E
#define TNEF_GRIDI_ATTACH_LONG_FILENAME 	 0x3707
#define TNEF_GRIDI_ATTACH_DATA 		 0x3701
#define TNEF_GRIDI_ATTACH_CID 		 0x3712

static int ___mime_decode_tnef(ztnef_parser_t * parser, ___mime_list_t * mime_list);

#define ___LEFT(parser) 	((parser)->tnef_size - ((parser)->tnef_pos - (parser)->tnef_data))

static inline int tnef_geti8(ztnef_parser_t * parser)
{
    int v;
    unsigned char *p;

    if (___LEFT(parser) < 1) {
        return -1;
    }

    p = (unsigned char *)(parser->tnef_pos);
    v = p[0];

    parser->tnef_pos += 1;

    return v;
}

static inline int tnef_geti16(ztnef_parser_t * parser)
{
    int v;
    unsigned char *p;

    if (___LEFT(parser) < 2) {
        return -1;
    }

    p = (unsigned char *)(parser->tnef_pos);
    v = p[0] + (p[1] << 8);

    parser->tnef_pos += 2;

    return v;
}

static inline int tnef_geti32(ztnef_parser_t * parser)
{
    int v;
    unsigned char *p;

    if (___LEFT(parser) < 4) {
        return -1;
    }

    p = (unsigned char *)(parser->tnef_pos);
    v = p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);

    parser->tnef_pos += 4;

    return v;
}

static inline int tnef_getx(ztnef_parser_t * parser, int value_len, char **value)
{
    if (___LEFT(parser) < value_len) {
        return -1;
    }

    *value = parser->tnef_pos;

    parser->tnef_pos += value_len;

    return 0;
}

static int tnef_decode_fragment(ztnef_parser_t * parser, int *attribute, char **value, int *value_len)
{
    if ((*attribute = tnef_geti32(parser)) == -1) {
        return -1;
    }

    if ((*value_len = tnef_geti32(parser)) == -1) {
        return -1;
    }

    if (tnef_getx(parser, *value_len, value) == -1) {
        return -1;
    }

    if (tnef_geti16(parser) == -1) {
        return -1;
    }

    return 0;
}

static int tnef_decode_message(ztnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);

    return ret;
}

static int extract_gridi_attrs(ztnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int att_type, att_name;
    char *val;
    int val_len;
    ztnef_mime_t *cmime;
    ztnef_parser_t parser2;

    cmime = mime_list->tail;

    /* number of attributes */
    if (tnef_geti32(parser) == -1) {
        return -1;
    }
    while (___LEFT(parser) > 0) {
        val = 0;
        val_len = 0;
        att_type = tnef_geti16(parser);
        att_name = tnef_geti16(parser);
        switch (att_type) {
        case TNEF_GRIDI_SHORT:
            if (tnef_getx(parser, 2, &val) == -1) {
                return -1;
            }
            break;
        case TNEF_GRIDI_INT:
        case TNEF_GRIDI_BOOLEAN:
        case TNEF_GRIDI_FLOAT:
            if (tnef_getx(parser, 4, &val) == -1) {
                return -1;
            }
            break;

        case TNEF_GRIDI_DOUBLE:
        case TNEF_GRIDI_SYSTIME:
            if (tnef_getx(parser, 8, &val) == -1) {
                return -1;
            }
            break;

        case TNEF_GRIDI_STRING:
        case TNEF_GRIDI_UNICODE_STRING:
        case TNEF_GRIDI_BINARY:
        case TNEF_GRIDI_OBJECT:
            {
                int num_vals = tnef_geti32(parser), i, length, buflen;
                if (num_vals == -1) {
                    return -1;
                }
                for (i = 0; i < num_vals; i++)  // usually just 1
                {
                    length = tnef_geti32(parser);
                    if (length == -1) {
                        return -1;
                    }
                    buflen = length + ((4 - (length % 4)) % 4); // pad to next 4 byte boundary
                    if (tnef_getx(parser, buflen, &val) == -1) {
                        return -1;
                    }
                    val_len = length;
                }
            }
            break;

        default:
            break;
        }
        switch (att_name) {
        case TNEF_GRIDI_ATTACH_LONG_FILENAME:  // used in preference to AFILENAME value
            if (val && cmime) {
                if (cmime->filename) {
                    zmpool_free(parser->mpool, cmime->filename);
                }
                cmime->filename = zmpool_memdup(parser->mpool, val, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_MIME_TAG:   // Is this ever set, and what is format?
            if (val && cmime && (!cmime->type)) {
                cmime->type = zmpool_memdup(parser->mpool, val, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_DATA:
            memcpy(&parser2, parser, sizeof(ztnef_parser_t));
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
            if (tnef_getx(&parser2, 16, &val) == -1) {
                return -1;
            }
            cmime = 0;
            if (mime_list_pop(mime_list, &cmime)) {
                if (cmime) {
                    free_one_mime(parser, cmime);
                }
            }
            cmime = 0;

            parser2.data_orignal = parser->data_orignal;
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
            if (___mime_decode_tnef(&parser2, mime_list) == -1) {
                return -1;
            }
            break;
        case TNEF_GRIDI_ATTACH_CID:
            if (val && cmime && (!cmime->content_id)) {
                cmime->content_id = zmpool_memdup(parser->mpool, val, val_len);
            }
            break;

        default:
            break;
        }

    }

    return 0;
}

static int tnef_decode_attachment(ztnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;
    ztnef_mime_t *cmime;
    ztnef_parser_t parser2;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);
    if (ret < 0) {
        return -1;
    }

    cmime = mime_list->tail;
    switch (attribute) {
    case TNEF_ARENDDATA:
        mime_list_add(parser, mime_list);
        break;
    case TNEF_AFILENAME:
        if (cmime && (!cmime->filename)) {
            cmime->filename = zmpool_memdup(parser->mpool, val, val_len);
        }
        break;
    case TNEF_ATTACHDATA:
        if (cmime) {
            cmime->body_len = val_len;
            cmime->body_offset = val - (parser->data_orignal);
        }
        break;
    case TNEF_AGRIDIATTRS:
        memcpy(&parser2, parser, sizeof(ztnef_parser_t));
        parser2.tnef_data = val;
        parser2.tnef_pos = val;
        parser2.tnef_size = val_len;
        if (extract_gridi_attrs(&parser2, mime_list) == -1) {
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int ___mime_decode_tnef(ztnef_parser_t * parser, ___mime_list_t * mime_list)
{
    int ret;
    int signature, type;

    signature = tnef_geti32(parser);
    if (signature != TNEF_SIGNATURE) {
        return -1;
    }
    tnef_geti16(parser);

    while (___LEFT(parser)) {
        type = tnef_geti8(parser);
        ret = 0;
        if (type == TNEF_LVL_MESSAGE) {
            ret = tnef_decode_message(parser, mime_list);
        } else if (type == TNEF_LVL_ATTACHMENT) {
            ret = tnef_decode_attachment(parser, mime_list);
        } else {
            return -1;
        }
        if (ret < 0) {
            return -1;
        }
    }

    return 0;
}

/* ################################################################## */

ztnef_parser_t *ztnef_parser_create_mpool(zmpool_t * imp, char *tnef_data, int tnef_data_len)
{
    ztnef_parser_t *parser;

    parser = (ztnef_parser_t *) zmpool_calloc(imp, 1, sizeof(ztnef_parser_t));
    parser->mpool = imp;

    parser->data_orignal = tnef_data;
    parser->tnef_data = tnef_data;
    parser->tnef_pos = tnef_data;
    parser->tnef_size = tnef_data_len;

    ztnef_parser_set_default_charset(parser, "GB18030", "UTF-8");

    return parser;
}

int ztnef_parser_run(ztnef_parser_t * parser)
{
    int ret;
    ___mime_list_t mime_list;
    ztnef_mime_t *m;
    int count;
    zmail_parser_t mail_parser;

    /* init one mail_parser for filename'iconv */
    memset(&mail_parser, 0, sizeof(zmail_parser_t));
    zmail_parser_set_default_charset(&mail_parser, parser->default_src_charset, parser->default_dest_charset);
    mail_parser.mpool = parser->mpool;

    /* mime chain */
    memset(&mime_list, 0, sizeof(___mime_list_t));
    mime_list.head = 0;
    mime_list.tail = 0;

    ret = ___mime_decode_tnef(parser, &mime_list);
    if (ret < 0) {
        return ret;
    }

    count = 0;
    for (m = mime_list.head; m; m = m->all_next) {
        count++;
        if ((m->filename) && (*(m->filename))) {
            zmail_parser_header_value_decode_dup(&mail_parser, m->filename, strlen(m->filename), &(m->filename_rd));
        }
        /* format */
#define _FR(a) { if((!(m->a))) { m->a = zmpool_strdup(parser->mpool, ""); }}
        _FR(type);
        _FR(filename);
        _FR(filename_rd);
        _FR(content_id);
#undef _FR
    }

    /* to list */
    parser->attachment_mime_count = count;
    parser->attachment_mime_list = (ztnef_mime_t **) zcalloc(count + 1, sizeof(ztnef_mime_t *));
    count = 0;
    for (m = mime_list.head; m; m = m->all_next) {
        parser->attachment_mime_list[count] = m;
        count++;
    }
    parser->attachment_mime_list[count] = 0;
    return 0;
}

int ztnef_parser_set_default_charset(ztnef_parser_t * parser, char *default_src_charset, char *default_dest_charset)
{
    if (!ZEMPTY(default_src_charset)) {
        strncpy(parser->default_src_charset, default_src_charset, 31);
    }
    if (!ZEMPTY(default_dest_charset)) {
        strncpy(parser->default_dest_charset, default_dest_charset, 31);
    }

    return 0;
}

/* ################################################################## */
ztnef_parser_t *ztnef_parser_create(char *mail_data, int mail_data_len)
{
    return ztnef_parser_create_mpool(0, mail_data, mail_data_len);
}

void ztnef_parser_free(ztnef_parser_t * parser)
{
    if (parser->mpool) {
        /* The free action is invalid when mpool is not null */
        return;
    }

    /* free mime */
    int i;
    for (i = 0; i < parser->attachment_mime_count; i++) {
        free_one_mime(parser, parser->attachment_mime_list[i]);
    }
    zmpool_free(parser->mpool, parser->attachment_mime_list);

    /* self */
    zmpool_free(parser->mpool, parser);
}

/* ################################################################## */
int ztnef_parser_get_mime_body(ztnef_parser_t * parser, ztnef_mime_t * mime, char **out_ptr)
{
    *out_ptr = parser->tnef_data + mime->body_offset;
    return mime->body_len;
}
