/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-04-28
 * ================================
 */

#include "zc.h"
#include "mime.h"

struct ztnef_mime_t {
    char *type;
    char *filename;
    char *filename_utf8;
    char *show_name;
    char *content_id;
    int body_offset;
    int body_len;
    ztnef_t *parser;
    unsigned char filename_utf8_flag:1;
    unsigned char show_name_flag:1;
};

struct ztnef_t {
    char *src_charset_def;
    zvector_t *all_mimes;
    char *data_orignal;
    char *tnef_data;
    char *tnef_pos;
    int tnef_size;
    zmmap_reader_t fmmap;
    unsigned char fmmap_flag:1;
};

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

static ztnef_mime_t *ztnef_mime_create(ztnef_t *parser);
static void ztnef_mime_free(ztnef_mime_t *mime);
static int ___mime_decode_tnef(ztnef_t * parser, zvector_t *mime_list);

#define ___LEFT(parser) 	((parser)->tnef_size - ((parser)->tnef_pos - (parser)->tnef_data))

static inline int tnef_geti8(ztnef_t * parser)
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

static inline int tnef_geti16(ztnef_t * parser)
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

static inline int tnef_geti32(ztnef_t * parser)
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

static inline int tnef_getx(ztnef_t * parser, int value_len, char **value)
{
    if (___LEFT(parser) < value_len) {
        return -1;
    }

    *value = parser->tnef_pos;

    parser->tnef_pos += value_len;

    return 0;
}

static int tnef_decode_fragment(ztnef_t * parser, int *attribute, char **value, int *value_len)
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

static int tnef_decode_message(ztnef_t * parser, zvector_t *mime_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);

    return ret;
}

static int extract_mapi_attrs(ztnef_t * parser, zvector_t *mime_list)
{
    int att_type, att_name;
    char *val;
    int val_len;
    ztnef_mime_t *cmime;
    ztnef_t parser2;

    if (zvector_len(mime_list)==0) {
        cmime = 0;
    } else {
        cmime = (ztnef_mime_t *)(zvector_data(mime_list)[zvector_len(mime_list)-1]);
    }

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
                zfree(cmime->filename);
                cmime->filename = zmemdupnull(val, val_len);
                zmail_clear_null_inner(cmime->filename, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_MIME_TAG:   // Is this ever set, and what is format?
            if (val && cmime && (zempty(cmime->type))) {
                zfree(cmime->type);
                cmime->type = zmemdupnull(val, val_len);
                zmail_clear_null_inner(cmime->type, val_len);
            }
            break;

        case TNEF_GRIDI_ATTACH_DATA:
            memset(&parser2, 0, sizeof(ztnef_t));
            parser2.src_charset_def = cmime->parser->src_charset_def;
            parser2.data_orignal = parser->data_orignal;
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
            if (tnef_getx(&parser2, 16, &val) == -1) {
                return -1;
            }
            if (zbuf_len(mime_list)) {
                cmime = (ztnef_mime_t *)(zvector_data(mime_list)[zvector_len(mime_list)-1]);
                zvector_truncate(mime_list, zvector_len(mime_list)-1);
                ztnef_mime_free(cmime);
            }
            cmime = 0;

#if  0
            parser2.data_orignal = parser->data_orignal;
            parser2.tnef_data = val;
            parser2.tnef_pos = val;
            parser2.tnef_size = val_len;
#endif
            if (___mime_decode_tnef(&parser2, mime_list) == -1) {
                return -1;
            }
            break;
        case TNEF_GRIDI_ATTACH_CID:
            if (val && cmime && (zempty(cmime->content_id))) {
                zfree(cmime->content_id);
                cmime->content_id = zmemdupnull(val, val_len);
                zmail_clear_null_inner(cmime->content_id, val_len);
            }
            break;

        default:
            break;
        }

    }

    return 0;
}

static int tnef_decode_attachment(ztnef_t * parser, zvector_t *mime_list)
{
    int ret;
    int attribute;
    char *val;
    int val_len;
    ztnef_mime_t *cmime;
    ztnef_t parser2;

    ret = tnef_decode_fragment(parser, &attribute, &val, &val_len);
    if (ret < 0) {
        return -1;
    }

    if (zvector_len(mime_list)==0) {
        cmime = 0;
    } else {
        cmime = (ztnef_mime_t *)(zvector_data(mime_list)[zvector_len(mime_list)-1]);
    }

    switch (attribute) {
    case TNEF_ARENDDATA:
        zvector_push(mime_list, ztnef_mime_create(parser));
        break;
    case TNEF_AFILENAME:
        if (cmime && (zempty(cmime->filename))) {
            zfree(cmime->filename);
            cmime->filename = zmemdupnull(val, val_len);
            zmail_clear_null_inner(cmime->filename, val_len);
        }
        break;
    case TNEF_ATTACHDATA:
        if (cmime) {
            cmime->body_len = val_len;
            cmime->body_offset = val - (parser->data_orignal);
        }
        break;
    case TNEF_AGRIDIATTRS:
        parser2.data_orignal = parser->data_orignal;
        parser2.tnef_data = val;
        parser2.tnef_pos = val;
        parser2.tnef_size = val_len;
        if (extract_mapi_attrs(&parser2, mime_list) == -1) {
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int ___mime_decode_tnef(ztnef_t * parser, zvector_t *mime_list)
{
    int ret;
    int signature, type;

    signature = tnef_geti32(parser);
    if (signature != TNEF_SIGNATURE) {
        return -1;
        if (parser->data_orignal == parser->tnef_data) {
            return -1;
        }
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
static ztnef_mime_t *ztnef_mime_create(ztnef_t *parser)
{
    ztnef_mime_t *mime = (ztnef_mime_t *)zcalloc(1, sizeof(ztnef_mime_t));
    mime->type = zblank_buffer;
    mime->filename = zblank_buffer;
    mime->filename_utf8 = zblank_buffer;
    mime->show_name = zblank_buffer;
    mime->content_id = zblank_buffer;
    mime->parser = parser;
    return mime;
}

static void ztnef_mime_free(ztnef_mime_t *mime)
{
    if(!mime) {
        return;
    }
    zfree(mime->type);
    zfree(mime->filename);
    zfree(mime->filename_utf8);
    zfree(mime->content_id);
    zfree(mime);
}

const char *ztnef_mime_get_type(ztnef_mime_t *mime)
{
    return mime->type;
}

const char *ztnef_mime_get_show_name(ztnef_mime_t *mime)
{
    if (!mime->show_name_flag) {
        if (!mime->filename_utf8_flag) {
            ztnef_mime_get_filename_utf8(mime);
        }
        char *n = mime->filename_utf8;
        if (zempty(n)) {
            n = mime->filename;
        }
        mime->show_name = n;
        mime->show_name_flag = 1;
    }
    return mime->show_name;
}

const char *ztnef_mime_get_filename(ztnef_mime_t *mime)
{
    return mime->filename;
}

const char *ztnef_mime_get_filename_utf8(ztnef_mime_t *mime)
{
    if (!mime->filename_utf8_flag) {
        if (!zempty(mime->filename)) {
            zbuf_t *tmpbf = zbuf_create(128);
            zmime_header_line_get_utf8(mime->parser->src_charset_def, mime->filename, -1, tmpbf);
            mime->filename_utf8 = zmemdupnull(zbuf_data(tmpbf), zbuf_len(tmpbf));
            zbuf_free(tmpbf);
        }
        mime->filename_utf8_flag = 1;
    }
    return mime->filename_utf8;
}

const char *ztnef_mime_get_content_id(ztnef_mime_t *mime)
{
    return mime->content_id;
}

int ztnef_mime_get_body_offset(ztnef_mime_t *mime)
{
    return mime->body_offset;
}

int ztnef_mime_get_body_len(ztnef_mime_t *mime)
{
    return mime->body_len;
}

/* ################################################################## */
ztnef_t *ztnef_create_parser()
{
    ztnef_t *parser = (ztnef_t *)zcalloc(1, sizeof(ztnef_t));
    return parser;
}

void ztnef_free(ztnef_t *parser)
{
    if (!parser) {
        return;
    }
    zfree(parser->src_charset_def);

    ZVECTOR_WALK_BEGIN(parser->all_mimes, ztnef_mime_t *, mime) {
        ztnef_mime_free(mime);
    } ZVECTOR_WALK_END;

    zfree(parser);
}

void ztnef_set_default_charset(ztnef_t *parser, const char *charset)
{
    zfree(parser->src_charset_def);
    parser->src_charset_def = zstrdup(charset);
}

void ztnef_parse_from_data(ztnef_t *parser, const char *tnef_data, int tnef_data_len)
{
    parser->data_orignal = (char *)(void *)tnef_data;
    parser->tnef_data = (char *)(void *)tnef_data;
    parser->tnef_pos = (char *)(void *)tnef_data;
    parser->tnef_size = tnef_data_len;

    parser->all_mimes = zvector_create(-1);
    if (___mime_decode_tnef(parser, parser->all_mimes) < 0) {
    }
}

zbool_t ztnef_parse_from_filename(ztnef_t *parser, const char *filename)
{
    if (zmmap_reader_init(&(parser->fmmap), filename) < 0) {
        return 0;
    }
    parser->fmmap_flag = 1;
    ztnef_parse_from_data(parser, parser->fmmap.data, parser->fmmap.len);
    return 1;
}

const char *ztnef_get_data(ztnef_t *parser)
{
    return parser->tnef_data;
}

int ztnef_get_len(ztnef_t *parser)
{
    return parser->tnef_size;
}

const zvector_t *ztnef_get_all_mimes(ztnef_t *parser)
{
    return parser->all_mimes;
}


void ztnef_debug_show(ztnef_t *parser)
{
    const char *fmt = "%15s: %s\n";
    int i = 0;
    ZVECTOR_WALK_BEGIN(parser->all_mimes, ztnef_mime_t *, m) {
        i++;
        printf("\n");
        char buf[128];
        sprintf(buf, "Mime (%d)", i);
        printf(fmt, buf, ztnef_mime_get_type(m));
        printf(fmt, "Content-Type", ztnef_mime_get_type(m));
        printf(fmt, "filename", ztnef_mime_get_filename(m));
        printf(fmt, "filename_utf8", ztnef_mime_get_filename_utf8(m));
        printf(fmt, "Content-ID", ztnef_mime_get_content_id(m));
        sprintf(buf, "%d", ztnef_mime_get_body_offset(m));
        printf(fmt, "body_offset", buf);
        sprintf(buf, "%d", ztnef_mime_get_body_len(m));
        printf(fmt, "body_len", buf);
    } ZVECTOR_WALK_END;
}
