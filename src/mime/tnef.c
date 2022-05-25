/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2016-04-28
 * ================================
 */

/* 请注意:
 * 本文件核心代码修改自 https://github.com/verdammelt/tnef/
 */

#include "zc.h"
#include "mime.h"

/* {{{ 外壳 */
struct ztnef_mime_t {
    char *type;
    char *filename;
    char *filename_utf8;
    char *content_id;
    char *body_data;
    int body_len;
    char *body_data2;
    ztnef_t *parser;
    unsigned char is_att:1;
    unsigned char is_szMAPI_UNICODE_STRING:1;
};

typedef struct ztnef_reader_t ztnef_reader_t;
struct ztnef_reader_t {
    char *start;
    char *pos;
    char *end;
};

struct ztnef_t {
    char *src_charset_def;
    zvector_t *all_mimes;
    zvector_t *text_mimes; 
    zvector_t *attachment_mimes;
    char *tnef_data;
    int tnef_size;
    int codepage;
    char *charset;
    zmmap_reader_t fmmap;
    unsigned char fmmap_flag:1;
};

static ztnef_mime_t *ztnef_mime_create(ztnef_t *parser);
static void ztnef_mime_free(ztnef_mime_t *mime);
static int ___mime_decode_tnef(ztnef_t *parser, ztnef_reader_t *reader);

static ztnef_mime_t *ztnef_mime_create(ztnef_t *parser)
{
    ztnef_mime_t *mime = (ztnef_mime_t *)zcalloc(1, sizeof(ztnef_mime_t));
    mime->type = zblank_buffer;
    mime->filename = zblank_buffer;
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
    zfree(mime->body_data2);
    zfree(mime);
}

const char *ztnef_mime_get_type(ztnef_mime_t *mime)
{
    return mime->type;
}

static void ztnef_mime_get_filename_utf8_deal(ztnef_mime_t *mime)
{
    if (mime->filename_utf8) {
        return;
    }
    int len = strlen(mime->filename);
    zbuf_t *result = zbuf_create(len*2+1);
    const char *charset = mime->parser->charset;
    if (mime->is_szMAPI_UNICODE_STRING) {
        charset = "UTF-8";
    }
    if (zempty(charset)) {
        charset = mime->parser->src_charset_def;
    }
    zmime_iconv(charset, mime->filename, len, result);
    mime->filename_utf8 = zmemdupnull(zbuf_data(result), zbuf_len(result));
    zbuf_free(result);
}

const char *ztnef_mime_get_show_name(ztnef_mime_t *mime)
{
    if (mime->filename_utf8 == 0) {
        ztnef_mime_get_filename_utf8_deal(mime);
    }
    return mime->filename_utf8;
}

const char *ztnef_mime_get_filename(ztnef_mime_t *mime)
{
    return mime->filename;
}

const char *ztnef_mime_get_filename_utf8(ztnef_mime_t *mime)
{
    if (mime->filename_utf8 == 0) {
        ztnef_mime_get_filename_utf8_deal(mime);
    }
    return mime->filename_utf8;
}

const char *ztnef_mime_get_content_id(ztnef_mime_t *mime)
{
    return mime->content_id;
}

const char *ztnef_mime_get_body_data(ztnef_mime_t *mime)
{
    return mime->body_data;
}

int ztnef_mime_get_body_len(ztnef_mime_t *mime)
{
    return mime->body_len;
}

const char *ztnef_mime_get_charset(ztnef_mime_t *mime)
{
    if (mime->is_att) {
        return zblank_buffer;
    }
    return mime->parser->charset;
}

void ___ztnef_init_parser(ztnef_t *parser, const char *tnef_data, int tnef_data_len, const char *default_charset)
{
    parser->charset = zblank_buffer;
    parser->tnef_data = (char *)(void *)tnef_data;
    parser->tnef_size = tnef_data_len;
    parser->src_charset_def = zstrdup(zempty(default_charset)?"GB18030":default_charset);

    parser->all_mimes = zvector_create(-1);
    parser->text_mimes = zvector_create(-1);
    parser->attachment_mimes = zvector_create(-1);
    ztnef_reader_t reader;
    reader.start = (char *)(void *)parser->tnef_data;
    reader.pos = reader.start;
    reader.end = reader.pos + parser->tnef_size;
    if (___mime_decode_tnef(parser, &reader) < 0) {
    }
    if (parser->codepage > 0) {
        char buf[128];
        sprintf(buf, "CP%d", parser->codepage);
        parser->charset = zstrdup(buf);
    }
}

ztnef_t * ztnef_create_parser_from_data(const char *tnef_data, int tnef_data_len, const char *default_charset)
{
    ztnef_t *parser = (ztnef_t *)zcalloc(1, sizeof(ztnef_t));
    ___ztnef_init_parser(parser, (void *)tnef_data, tnef_data_len, default_charset);
    return parser;
}

ztnef_t *ztnef_create_parser_from_pathname(const char *pathname, const char *default_charset)
{
    ztnef_t *parser = (ztnef_t *)zcalloc(1, sizeof(ztnef_t));
    if (zmmap_reader_init(&(parser->fmmap), pathname) < 0) {
        zfree(parser);
        return 0;
    }
    parser->fmmap_flag = 1;
    ___ztnef_init_parser(parser, (void *)parser->fmmap.data, parser->fmmap.len, default_charset);
    return parser;
}

void ztnef_free(ztnef_t *parser)
{
    if (!parser) {
        return;
    }
    zfree(parser->src_charset_def);
    zfree(parser->charset);

    ZVECTOR_WALK_BEGIN(parser->all_mimes, ztnef_mime_t *, mime) {
        ztnef_mime_free(mime);
    } ZVECTOR_WALK_END;
    zvector_free(parser->all_mimes);
    zvector_free(parser->text_mimes);
    zvector_free(parser->attachment_mimes);

    zfree(parser);
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

const zvector_t *ztnef_get_text_mimes(ztnef_t *parser)
{
    return parser->text_mimes;
}

const zvector_t *ztnef_get_attachment_mimes(ztnef_t *parser)
{
    return parser->attachment_mimes;
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
        printf(fmt, "filename_utf8", ztnef_mime_get_filename_utf8(m));
        printf(fmt, "Content-ID", ztnef_mime_get_content_id(m));
        printf(fmt, "charset", ztnef_mime_get_charset(m));
        sprintf(buf, "%d", ztnef_mime_get_body_len(m));
        printf(fmt, "body_len", buf);
    } ZVECTOR_WALK_END;
}
/* }}} */

/* {{{ get int */
#define ___LEFT(reader)	(((reader)->pos < (reader)->end)?((reader)->end -(reader)->pos):-1)

static inline int tnef_geti8(ztnef_reader_t *reader)
{
    int v;
    unsigned char *p;

    if (___LEFT(reader) < 1) {
        return -1;
    }

    p = (unsigned char *)(reader->pos);
    v = p[0];

    reader->pos += 1;

    return v;
}

static inline int tnef_geti16(ztnef_reader_t *reader)
{
    int v;
    unsigned char *p;

    if (___LEFT(reader) < 2) {
        return -1;
    }

    p = (unsigned char *)(reader->pos);
    v = p[0] + (p[1] << 8);

    reader->pos += 2;

    return v;
}

static inline int tnef_geti32(ztnef_reader_t *reader)
{
    int v;
    unsigned char *p;

    if (___LEFT(reader) < 4) {
        return -1;
    }

    p = (unsigned char *)(reader->pos);
    v = p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);

    reader->pos += 4;

    return v;
}

static inline int tnef_getx(ztnef_reader_t *reader, int value_len, char **value)
{
    if (___LEFT(reader) < value_len) {
        return -1;
    }

    *value = reader->pos;

    reader->pos += value_len;

    return 0;
}

static inline int tnef_skip(ztnef_reader_t *reader, int len)
{
    if (___LEFT(reader) < len) {
        return -1;
    }

    reader->pos += len;

    return 0;
}

static inline int tnef_pad_to_4byte (int length)
{
    size_t len = length;
    return (int)((len+3) & ~3);
}

/* }}} */

/* {{{ define */
#define TNEF_SIGNATURE                                0x223e9f78

/* level type */
#define LVL_MESSAGE                                   0x1
#define LVL_ATTACHMENT                                0x2

/* type */
#define szTRIPLES                                     0x0000
#define szSTRING                                      0x0001
#define szTEXT                                        0x0002
#define szDATE                                        0x0003
#define szSHORT                                       0x0004
#define szLONG                                        0x0005
#define szBYTE                                        0x0006
#define szWORD                                        0x0007
#define szDWORD                                       0x0008
#define szMAX                                         0x0009

/* name */
#define attOWNER                                      0x0000
#define attSENTFOR                                    0x0001
#define attDELEGATE                                   0x0002
#define attDATESTART                                  0x0006
#define attDATEEND                                    0x0007
#define attAIDOWNER                                   0x0008
#define attREQUESTRES                                 0x0009
#define attFROM                                       0x8000
#define attSUBJECT                                    0x8004
#define attDATESENT                                   0x8005
#define attDATERECD                                   0x8006
#define attMESSAGESTATUS                              0x8007
#define attMESSAGECLASS                               0x8008
#define attMESSAGEID                                  0x8009
#define attPARENTID                                   0x800a
#define attCONVERSATIONID                             0x800b
#define attBODY                                       0x800c
#define attPRIORITY                                   0x800d
#define attATTACHDATA                                 0x800f
#define attATTACHTITLE                                0x8010
#define attATTACHMETAFILE                             0x8011
#define attATTACHCREATEDATE                           0x8012
#define attATTACHMODIFYDATE                           0x8013
#define attDATEMODIFY                                 0x8020
#define attATTACHTRANSPORTFILENAME                    0x9001
#define attATTACHRENDDATA                             0x9002
#define attMAPIPROPS                                  0x9003
#define attRECIPTABLE                                 0x9004
#define attATTACHMENT                                 0x9005
#define attTNEFVERSION                                0x9006
#define attOEMCODEPAGE                                0x9007
#define attORIGNINALMESSAGECLASS                      0x9008

/* mapi type */
#define szMAPI_UNSPECIFIED                            0x0000
#define szMAPI_NULL                                   0x0001
#define szMAPI_SHORT                                  0x0002
#define szMAPI_INT                                    0x0003
#define szMAPI_FLOAT                                  0x0004
#define szMAPI_DOUBLE                                 0x0005
#define szMAPI_CURRENCY                               0x0006
#define szMAPI_APPTIME                                0x0007
#define szMAPI_ERROR                                  0x000a
#define szMAPI_BOOLEAN                                0x000b
#define szMAPI_OBJECT                                 0x000d
#define szMAPI_INT8BYTE                               0x0014
#define szMAPI_STRING                                 0x001e
#define szMAPI_UNICODE_STRING                         0x001f
#define szMAPI_SYSTIME                                0x0040
#define szMAPI_CLSID                                  0x0048
#define szMAPI_BINARY                                 0x0102

#define MULTI_VALUE_FLAG                              0x1000
#define GUID_EXISTS_FLAG                              0x8000

/* mapi names */
#define MAPI_ACKNOWLEDGEMENT_MODE                     0x0001
#define MAPI_ALTERNATE_RECIPIENT_ALLOWED              0x0002
#define MAPI_AUTHORIZING_USERS                        0x0003
#define MAPI_AUTO_FORWARD_COMMENT                     0x0004
#define MAPI_AUTO_FORWARDED                           0x0005
#define MAPI_CONTENT_CONFIDENTIALITY_ALGORITHM_ID     0x0006
#define MAPI_CONTENT_CORRELATOR                       0x0007
#define MAPI_CONTENT_IDENTIFIER                       0x0008
#define MAPI_CONTENT_LENGTH                           0x0009
#define MAPI_CONTENT_RETURN_REQUESTED                 0x000A
#define MAPI_CONVERSATION_KEY                         0x000B
#define MAPI_CONVERSION_EITS                          0x000C
#define MAPI_CONVERSION_WITH_LOSS_PROHIBITED          0x000D
#define MAPI_CONVERTED_EITS                           0x000E
#define MAPI_DEFERRED_DELIVERY_TIME                   0x000F
#define MAPI_DELIVER_TIME                             0x0010
#define MAPI_DISCARD_REASON                           0x0011
#define MAPI_DISCLOSURE_OF_RECIPIENTS                 0x0012
#define MAPI_DL_EXPANSION_HISTORY                     0x0013
#define MAPI_DL_EXPANSION_PROHIBITED                  0x0014
#define MAPI_EXPIRY_TIME                              0x0015
#define MAPI_IMPLICIT_CONVERSION_PROHIBITED           0x0016
#define MAPI_IMPORTANCE                               0x0017
#define MAPI_IPM_ID                                   0x0018
#define MAPI_LATEST_DELIVERY_TIME                     0x0019
#define MAPI_MESSAGE_CLASS                            0x001A
#define MAPI_MESSAGE_DELIVERY_ID                      0x001B
#define MAPI_MESSAGE_SECURITY_LABEL                   0x001E
#define MAPI_OBSOLETED_IPMS                           0x001F
#define MAPI_ORIGINALLY_INTENDED_RECIPIENT_NAME       0x0020
#define MAPI_ORIGINAL_EITS                            0x0021
#define MAPI_ORIGINATOR_CERTIFICATE                   0x0022
#define MAPI_ORIGINATOR_DELIVERY_REPORT_REQUESTED     0x0023
#define MAPI_ORIGINATOR_RETURN_ADDRESS                0x0024
#define MAPI_PARENT_KEY                               0x0025
#define MAPI_PRIORITY                                 0x0026
#define MAPI_ORIGIN_CHECK                             0x0027
#define MAPI_PROOF_OF_SUBMISSION_REQUESTED            0x0028
#define MAPI_READ_RECEIPT_REQUESTED                   0x0029
#define MAPI_RECEIPT_TIME                             0x002A
#define MAPI_RECIPIENT_REASSIGNMENT_PROHIBITED        0x002B
#define MAPI_REDIRECTION_HISTORY                      0x002C
#define MAPI_RELATED_IPMS                             0x002D
#define MAPI_ORIGINAL_SENSITIVITY                     0x002E
#define MAPI_LANGUAGES                                0x002F
#define MAPI_REPLY_TIME                               0x0030
#define MAPI_REPORT_TAG                               0x0031
#define MAPI_REPORT_TIME                              0x0032
#define MAPI_RETURNED_IPM                             0x0033
#define MAPI_SECURITY                                 0x0034
#define MAPI_INCOMPLETE_COPY                          0x0035
#define MAPI_SENSITIVITY                              0x0036
#define MAPI_SUBJECT                                  0x0037
#define MAPI_SUBJECT_IPM                              0x0038
#define MAPI_CLIENT_SUBMIT_TIME                       0x0039
#define MAPI_REPORT_NAME                              0x003A
#define MAPI_SENT_REPRESENTING_SEARCH_KEY             0x003B
#define MAPI_X400_CONTENT_TYPE                        0x003C
#define MAPI_SUBJECT_PREFIX                           0x003D
#define MAPI_NON_RECEIPT_REASON                       0x003E
#define MAPI_RECEIVED_BY_ENTRYID                      0x003F
#define MAPI_RECEIVED_BY_NAME                         0x0040
#define MAPI_SENT_REPRESENTING_ENTRYID                0x0041
#define MAPI_SENT_REPRESENTING_NAME                   0x0042
#define MAPI_RCVD_REPRESENTING_ENTRYID                0x0043
#define MAPI_RCVD_REPRESENTING_NAME                   0x0044
#define MAPI_REPORT_ENTRYID                           0x0045
#define MAPI_READ_RECEIPT_ENTRYID                     0x0046
#define MAPI_MESSAGE_SUBMISSION_ID                    0x0047
#define MAPI_PROVIDER_SUBMIT_TIME                     0x0048
#define MAPI_ORIGINAL_SUBJECT                         0x0049
#define MAPI_DISC_VAL                                 0x004A
#define MAPI_ORIG_MESSAGE_CLASS                       0x004B
#define MAPI_ORIGINAL_AUTHOR_ENTRYID                  0x004C
#define MAPI_ORIGINAL_AUTHOR_NAME                     0x004D
#define MAPI_ORIGINAL_SUBMIT_TIME                     0x004E
#define MAPI_REPLY_RECIPIENT_ENTRIES                  0x004F
#define MAPI_REPLY_RECIPIENT_NAMES                    0x0050
#define MAPI_RECEIVED_BY_SEARCH_KEY                   0x0051
#define MAPI_RCVD_REPRESENTING_SEARCH_KEY             0x0052
#define MAPI_READ_RECEIPT_SEARCH_KEY                  0x0053
#define MAPI_REPORT_SEARCH_KEY                        0x0054
#define MAPI_ORIGINAL_DELIVERY_TIME                   0x0055
#define MAPI_ORIGINAL_AUTHOR_SEARCH_KEY               0x0056
#define MAPI_MESSAGE_TO_ME                            0x0057
#define MAPI_MESSAGE_CC_ME                            0x0058
#define MAPI_MESSAGE_RECIP_ME                         0x0059
#define MAPI_ORIGINAL_SENDER_NAME                     0x005A
#define MAPI_ORIGINAL_SENDER_ENTRYID                  0x005B
#define MAPI_ORIGINAL_SENDER_SEARCH_KEY               0x005C
#define MAPI_ORIGINAL_SENT_REPRESENTING_NAME          0x005D
#define MAPI_ORIGINAL_SENT_REPRESENTING_ENTRYID       0x005E
#define MAPI_ORIGINAL_SENT_REPRESENTING_SEARCH_KEY    0x005F
#define MAPI_START_DATE                               0x0060
#define MAPI_END_DATE                                 0x0061
#define MAPI_OWNER_APPT_ID                            0x0062
#define MAPI_RESPONSE_REQUESTED                       0x0063
#define MAPI_SENT_REPRESENTING_ADDRTYPE               0x0064
#define MAPI_SENT_REPRESENTING_EMAIL_ADDRESS          0x0065
#define MAPI_ORIGINAL_SENDER_ADDRTYPE                 0x0066
#define MAPI_ORIGINAL_SENDER_EMAIL_ADDRESS            0x0067
#define MAPI_ORIGINAL_SENT_REPRESENTING_ADDRTYPE      0x0068
#define MAPI_ORIGINAL_SENT_REPRESENTING_EMAIL_ADDRESS 0x0069
#define MAPI_CONVERSATION_TOPIC                       0x0070
#define MAPI_CONVERSATION_INDEX                       0x0071
#define MAPI_ORIGINAL_DISPLAY_BCC                     0x0072
#define MAPI_ORIGINAL_DISPLAY_CC                      0x0073
#define MAPI_ORIGINAL_DISPLAY_TO                      0x0074
#define MAPI_RECEIVED_BY_ADDRTYPE                     0x0075
#define MAPI_RECEIVED_BY_EMAIL_ADDRESS                0x0076
#define MAPI_RCVD_REPRESENTING_ADDRTYPE               0x0077
#define MAPI_RCVD_REPRESENTING_EMAIL_ADDRESS          0x0078
#define MAPI_ORIGINAL_AUTHOR_ADDRTYPE                 0x0079
#define MAPI_ORIGINAL_AUTHOR_EMAIL_ADDRESS            0x007A
#define MAPI_ORIGINALLY_INTENDED_RECIP_ADDRTYPE       0x007B
#define MAPI_ORIGINALLY_INTENDED_RECIP_EMAIL_ADDRESS  0x007C
#define MAPI_TRANSPORT_MESSAGE_HEADERS                0x007D
#define MAPI_DELEGATION                               0x007E
#define MAPI_TNEF_CORRELATION_KEY                     0x007F
#define MAPI_BODY                                     0x1000
#define MAPI_REPORT_TEXT                              0x1001
#define MAPI_ORIGINATOR_AND_DL_EXPANSION_HISTORY      0x1002
#define MAPI_REPORTING_DL_NAME                        0x1003
#define MAPI_REPORTING_MTA_CERTIFICATE                0x1004
#define MAPI_RTF_SYNC_BODY_CRC                        0x1006
#define MAPI_RTF_SYNC_BODY_COUNT                      0x1007
#define MAPI_RTF_SYNC_BODY_TAG                        0x1008
#define MAPI_RTF_COMPRESSED                           0x1009
#define MAPI_RTF_SYNC_PREFIX_COUNT                    0x1010
#define MAPI_RTF_SYNC_TRAILING_COUNT                  0x1011
#define MAPI_ORIGINALLY_INTENDED_RECIP_ENTRYID        0x1012
#define MAPI_BODY_HTML                                0x1013
#define MAPI_SMTP_MESSAGE_ID                          0x1035
#define MAPI_CONTENT_INTEGRITY_CHECK                  0x0C00
#define MAPI_EXPLICIT_CONVERSION                      0x0C01
#define MAPI_IPM_RETURN_REQUESTED                     0x0C02
#define MAPI_MESSAGE_TOKEN                            0x0C03
#define MAPI_NDR_REASON_CODE                          0x0C04
#define MAPI_NDR_DIAG_CODE                            0x0C05
#define MAPI_NON_RECEIPT_NOTIFICATION_REQUESTED       0x0C06
#define MAPI_DELIVERY_POINT                           0x0C07
#define MAPI_ORIGINATOR_NON_DELIVERY_REPORT_REQUESTED 0x0C08
#define MAPI_ORIGINATOR_REQUESTED_ALTERNATE_RECIPIENT 0x0C09
#define MAPI_PHYSICAL_DELIVERY_BUREAU_FAX_DELIVERY    0x0C0A
#define MAPI_PHYSICAL_DELIVERY_MODE                   0x0C0B
#define MAPI_PHYSICAL_DELIVERY_REPORT_REQUEST         0x0C0C
#define MAPI_PHYSICAL_FORWARDING_ADDRESS              0x0C0D
#define MAPI_PHYSICAL_FORWARDING_ADDRESS_REQUESTED    0x0C0E
#define MAPI_PHYSICAL_FORWARDING_PROHIBITED           0x0C0F
#define MAPI_PHYSICAL_RENDITION_ATTRIBUTES            0x0C10
#define MAPI_PROOF_OF_DELIVERY                        0x0C11
#define MAPI_PROOF_OF_DELIVERY_REQUESTED              0x0C12
#define MAPI_RECIPIENT_CERTIFICATE                    0x0C13
#define MAPI_RECIPIENT_NUMBER_FOR_ADVICE              0x0C14
#define MAPI_RECIPIENT_TYPE                           0x0C15
#define MAPI_REGISTERED_MAIL_TYPE                     0x0C16
#define MAPI_REPLY_REQUESTED                          0x0C17
#define MAPI_REQUESTED_DELIVERY_METHOD                0x0C18
#define MAPI_SENDER_ENTRYID                           0x0C19
#define MAPI_SENDER_NAME                              0x0C1A
#define MAPI_SUPPLEMENTARY_INFO                       0x0C1B
#define MAPI_TYPE_OF_MTS_USER                         0x0C1C
#define MAPI_SENDER_SEARCH_KEY                        0x0C1D
#define MAPI_SENDER_ADDRTYPE                          0x0C1E
#define MAPI_SENDER_EMAIL_ADDRESS                     0x0C1F
#define MAPI_CURRENT_VERSION                          0x0E00
#define MAPI_DELETE_AFTER_SUBMIT                      0x0E01
#define MAPI_DISPLAY_BCC                              0x0E02
#define MAPI_DISPLAY_CC                               0x0E03
#define MAPI_DISPLAY_TO                               0x0E04
#define MAPI_PARENT_DISPLAY                           0x0E05
#define MAPI_MESSAGE_DELIVERY_TIME                    0x0E06
#define MAPI_MESSAGE_FLAGS                            0x0E07
#define MAPI_MESSAGE_SIZE                             0x0E08
#define MAPI_PARENT_ENTRYID                           0x0E09
#define MAPI_SENTMAIL_ENTRYID                         0x0E0A
#define MAPI_CORRELATE                                0x0E0C
#define MAPI_CORRELATE_MTSID                          0x0E0D
#define MAPI_DISCRETE_VALUES                          0x0E0E
#define MAPI_RESPONSIBILITY                           0x0E0F
#define MAPI_SPOOLER_STATUS                           0x0E10
#define MAPI_TRANSPORT_STATUS                         0x0E11
#define MAPI_MESSAGE_RECIPIENTS                       0x0E12
#define MAPI_MESSAGE_ATTACHMENTS                      0x0E13
#define MAPI_SUBMIT_FLAGS                             0x0E14
#define MAPI_RECIPIENT_STATUS                         0x0E15
#define MAPI_TRANSPORT_KEY                            0x0E16
#define MAPI_MSG_STATUS                               0x0E17
#define MAPI_MESSAGE_DOWNLOAD_TIME                    0x0E18
#define MAPI_CREATION_VERSION                         0x0E19
#define MAPI_MODIFY_VERSION                           0x0E1A
#define MAPI_HASATTACH                                0x0E1B
#define MAPI_BODY_CRC                                 0x0E1C
#define MAPI_NORMALIZED_SUBJECT                       0x0E1D
#define MAPI_RTF_IN_SYNC                              0x0E1F
#define MAPI_ATTACH_SIZE                              0x0E20
#define MAPI_ATTACH_NUM                               0x0E21
#define MAPI_PREPROCESS                               0x0E22
#define MAPI_ORIGINATING_MTA_CERTIFICATE              0x0E25
#define MAPI_PROOF_OF_SUBMISSION                      0x0E26
#define MAPI_ENTRYID                                  0x0FFF
#define MAPI_OBJECT_TYPE                              0x0FFE
#define MAPI_ICON                                     0x0FFD
#define MAPI_MINI_ICON                                0x0FFC
#define MAPI_STORE_ENTRYID                            0x0FFB
#define MAPI_STORE_RECORD_KEY                         0x0FFA
#define MAPI_RECORD_KEY                               0x0FF9
#define MAPI_MAPPING_SIGNATURE                        0x0FF8
#define MAPI_ACCESS_LEVEL                             0x0FF7
#define MAPI_INSTANCE_KEY                             0x0FF6
#define MAPI_ROW_TYPE                                 0x0FF5
#define MAPI_ACCESS                                   0x0FF4
#define MAPI_ROWID                                    0x3000
#define MAPI_DISPLAY_NAME                             0x3001
#define MAPI_ADDRTYPE                                 0x3002
#define MAPI_EMAIL_ADDRESS                            0x3003
#define MAPI_COMMENT                                  0x3004
#define MAPI_DEPTH                                    0x3005
#define MAPI_PROVIDER_DISPLAY                         0x3006
#define MAPI_CREATION_TIME                            0x3007
#define MAPI_LAST_MODIFICATION_TIME                   0x3008
#define MAPI_RESOURCE_FLAGS                           0x3009
#define MAPI_PROVIDER_DLL_NAME                        0x300A
#define MAPI_SEARCH_KEY                               0x300B
#define MAPI_PROVIDER_UID                             0x300C
#define MAPI_PROVIDER_ORDINAL                         0x300D
#define MAPI_FORM_VERSION                             0x3301
#define MAPI_FORM_CLSID                               0x3302
#define MAPI_FORM_CONTACT_NAME                        0x3303
#define MAPI_FORM_CATEGORY                            0x3304
#define MAPI_FORM_CATEGORY_SUB                        0x3305
#define MAPI_FORM_HOST_MAP                            0x3306
#define MAPI_FORM_HIDDEN                              0x3307
#define MAPI_FORM_DESIGNER_NAME                       0x3308
#define MAPI_FORM_DESIGNER_GUID                       0x3309
#define MAPI_FORM_MESSAGE_BEHAVIOR                    0x330A
#define MAPI_DEFAULT_STORE                            0x3400
#define MAPI_STORE_SUPPORT_MASK                       0x340D
#define MAPI_STORE_STATE                              0x340E
#define MAPI_IPM_SUBTREE_SEARCH_KEY                   0x3410
#define MAPI_IPM_OUTBOX_SEARCH_KEY                    0x3411
#define MAPI_IPM_WASTEBASKET_SEARCH_KEY               0x3412
#define MAPI_IPM_SENTMAIL_SEARCH_KEY                  0x3413
#define MAPI_MDB_PROVIDER                             0x3414
#define MAPI_RECEIVE_FOLDER_SETTINGS                  0x3415
#define MAPI_VALID_FOLDER_MASK                        0x35DF
#define MAPI_IPM_SUBTREE_ENTRYID                      0x35E0
#define MAPI_IPM_OUTBOX_ENTRYID                       0x35E2
#define MAPI_IPM_WASTEBASKET_ENTRYID                  0x35E3
#define MAPI_IPM_SENTMAIL_ENTRYID                     0x35E4
#define MAPI_VIEWS_ENTRYID                            0x35E5
#define MAPI_COMMON_VIEWS_ENTRYID                     0x35E6
#define MAPI_FINDER_ENTRYID                           0x35E7
#define MAPI_CONTAINER_FLAGS                          0x3600
#define MAPI_FOLDER_TYPE                              0x3601
#define MAPI_CONTENT_COUNT                            0x3602
#define MAPI_CONTENT_UNREAD                           0x3603
#define MAPI_CREATE_TEMPLATES                         0x3604
#define MAPI_DETAILS_TABLE                            0x3605
#define MAPI_SEARCH                                   0x3607
#define MAPI_SELECTABLE                               0x3609
#define MAPI_SUBFOLDERS                               0x360A
#define MAPI_STATUS                                   0x360B
#define MAPI_ANR                                      0x360C
#define MAPI_CONTENTS_SORT_ORDER                      0x360D
#define MAPI_CONTAINER_HIERARCHY                      0x360E
#define MAPI_CONTAINER_CONTENTS                       0x360F
#define MAPI_FOLDER_ASSOCIATED_CONTENTS               0x3610
#define MAPI_DEF_CREATE_DL                            0x3611
#define MAPI_DEF_CREATE_MAILUSER                      0x3612
#define MAPI_CONTAINER_CLASS                          0x3613
#define MAPI_CONTAINER_MODIFY_VERSION                 0x3614
#define MAPI_AB_PROVIDER_ID                           0x3615
#define MAPI_DEFAULT_VIEW_ENTRYID                     0x3616
#define MAPI_ASSOC_CONTENT_COUNT                      0x3617
#define MAPI_ATTACHMENT_X400_PARAMETERS               0x3700
#define MAPI_ATTACH_DATA_OBJ                          0x3701
#define MAPI_ATTACH_ENCODING                          0x3702
#define MAPI_ATTACH_EXTENSION                         0x3703
#define MAPI_ATTACH_FILENAME                          0x3704
#define MAPI_ATTACH_METHOD                            0x3705
#define MAPI_ATTACH_LONG_FILENAME                     0x3707
#define MAPI_ATTACH_PATHNAME                          0x3708
#define MAPI_ATTACH_RENDERING                         0x3709
#define MAPI_ATTACH_TAG                               0x370A
#define MAPI_RENDERING_POSITION                       0x370B
#define MAPI_ATTACH_TRANSPORT_NAME                    0x370C
#define MAPI_ATTACH_LONG_PATHNAME                     0x370D
#define MAPI_ATTACH_MIME_TAG                          0x370E
#define MAPI_ATTACH_ADDITIONAL_INFO                   0x370F
#define MAPI_ATTACH_MIME_SEQUENCE                     0x3710
#define MAPI_ATTACH_CONTENT_ID                        0x3712
#define MAPI_ATTACH_CONTENT_LOCATION                  0x3713
#define MAPI_ATTACH_FLAGS                             0x3714
#define MAPI_DISPLAY_TYPE                             0x3900
#define MAPI_TEMPLATEID                               0x3902
#define MAPI_PRIMARY_CAPABILITY                       0x3904
#define MAPI_7BIT_DISPLAY_NAME                        0x39FF
#define MAPI_ACCOUNT                                  0x3A00
#define MAPI_ALTERNATE_RECIPIENT                      0x3A01
#define MAPI_CALLBACK_TELEPHONE_NUMBER                0x3A02
#define MAPI_CONVERSION_PROHIBITED                    0x3A03
#define MAPI_DISCLOSE_RECIPIENTS                      0x3A04
#define MAPI_GENERATION                               0x3A05
#define MAPI_GIVEN_NAME                               0x3A06
#define MAPI_GOVERNMENT_ID_NUMBER                     0x3A07
#define MAPI_BUSINESS_TELEPHONE_NUMBER                0x3A08
#define MAPI_HOME_TELEPHONE_NUMBER                    0x3A09
#define MAPI_INITIALS                                 0x3A0A
#define MAPI_KEYWORD                                  0x3A0B
#define MAPI_LANGUAGE                                 0x3A0C
#define MAPI_LOCATION                                 0x3A0D
#define MAPI_MAIL_PERMISSION                          0x3A0E
#define MAPI_MHS_COMMON_NAME                          0x3A0F
#define MAPI_ORGANIZATIONAL_ID_NUMBER                 0x3A10
#define MAPI_SURNAME                                  0x3A11
#define MAPI_ORIGINAL_ENTRYID                         0x3A12
#define MAPI_ORIGINAL_DISPLAY_NAME                    0x3A13
#define MAPI_ORIGINAL_SEARCH_KEY                      0x3A14
#define MAPI_POSTAL_ADDRESS                           0x3A15
#define MAPI_COMPANY_NAME                             0x3A16
#define MAPI_TITLE                                    0x3A17
#define MAPI_DEPARTMENT_NAME                          0x3A18
#define MAPI_OFFICE_LOCATION                          0x3A19
#define MAPI_PRIMARY_TELEPHONE_NUMBER                 0x3A1A
#define MAPI_BUSINESS2_TELEPHONE_NUMBER               0x3A1B
#define MAPI_MOBILE_TELEPHONE_NUMBER                  0x3A1C
#define MAPI_RADIO_TELEPHONE_NUMBER                   0x3A1D
#define MAPI_CAR_TELEPHONE_NUMBER                     0x3A1E
#define MAPI_OTHER_TELEPHONE_NUMBER                   0x3A1F
#define MAPI_TRANSMITABLE_DISPLAY_NAME                0x3A20
#define MAPI_PAGER_TELEPHONE_NUMBER                   0x3A21
#define MAPI_USER_CERTIFICATE                         0x3A22
#define MAPI_PRIMARY_FAX_NUMBER                       0x3A23
#define MAPI_BUSINESS_FAX_NUMBER                      0x3A24
#define MAPI_HOME_FAX_NUMBER                          0x3A25
#define MAPI_COUNTRY                                  0x3A26
#define MAPI_LOCALITY                                 0x3A27
#define MAPI_STATE_OR_PROVINCE                        0x3A28
#define MAPI_STREET_ADDRESS                           0x3A29
#define MAPI_POSTAL_CODE                              0x3A2A
#define MAPI_POST_OFFICE_BOX                          0x3A2B
#define MAPI_TELEX_NUMBER                             0x3A2C
#define MAPI_ISDN_NUMBER                              0x3A2D
#define MAPI_ASSISTANT_TELEPHONE_NUMBER               0x3A2E
#define MAPI_HOME2_TELEPHONE_NUMBER                   0x3A2F
#define MAPI_ASSISTANT                                0x3A30
#define MAPI_SEND_RICH_INFO                           0x3A40
#define MAPI_WEDDING_ANNIVERSARY                      0x3A41
#define MAPI_BIRTHDAY                                 0x3A42
#define MAPI_HOBBIES                                  0x3A43
#define MAPI_MIDDLE_NAME                              0x3A44
#define MAPI_DISPLAY_NAME_PREFIX                      0x3A45
#define MAPI_PROFESSION                               0x3A46
#define MAPI_PREFERRED_BY_NAME                        0x3A47
#define MAPI_SPOUSE_NAME                              0x3A48
#define MAPI_COMPUTER_NETWORK_NAME                    0x3A49
#define MAPI_CUSTOMER_ID                              0x3A4A
#define MAPI_TTYTDD_PHONE_NUMBER                      0x3A4B
#define MAPI_FTP_SITE                                 0x3A4C
#define MAPI_GENDER                                   0x3A4D
#define MAPI_MANAGER_NAME                             0x3A4E
#define MAPI_NICKNAME                                 0x3A4F
#define MAPI_PERSONAL_HOME_PAGE                       0x3A50
#define MAPI_BUSINESS_HOME_PAGE                       0x3A51
#define MAPI_CONTACT_VERSION                          0x3A52
#define MAPI_CONTACT_ENTRYIDS                         0x3A53
#define MAPI_CONTACT_ADDRTYPES                        0x3A54
#define MAPI_CONTACT_DEFAULT_ADDRESS_INDEX            0x3A55
#define MAPI_CONTACT_EMAIL_ADDRESSES                  0x3A56
#define MAPI_COMPANY_MAIN_PHONE_NUMBER                0x3A57
#define MAPI_CHILDRENS_NAMES                          0x3A58
#define MAPI_HOME_ADDRESS_CITY                        0x3A59
#define MAPI_HOME_ADDRESS_COUNTRY                     0x3A5A
#define MAPI_HOME_ADDRESS_POSTAL_CODE                 0x3A5B
#define MAPI_HOME_ADDRESS_STATE_OR_PROVINCE           0x3A5C
#define MAPI_HOME_ADDRESS_STREET                      0x3A5D
#define MAPI_HOME_ADDRESS_POST_OFFICE_BOX             0x3A5E
#define MAPI_OTHER_ADDRESS_CITY                       0x3A5F
#define MAPI_OTHER_ADDRESS_COUNTRY                    0x3A60
#define MAPI_OTHER_ADDRESS_POSTAL_CODE                0x3A61
#define MAPI_OTHER_ADDRESS_STATE_OR_PROVINCE          0x3A62
#define MAPI_OTHER_ADDRESS_STREET                     0x3A63
#define MAPI_OTHER_ADDRESS_POST_OFFICE_BOX            0x3A64
#define MAPI_STORE_PROVIDERS                          0x3D00
#define MAPI_AB_PROVIDERS                             0x3D01
#define MAPI_TRANSPORT_PROVIDERS                      0x3D02
#define MAPI_DEFAULT_PROFILE                          0x3D04
#define MAPI_AB_SEARCH_PATH                           0x3D05
#define MAPI_AB_DEFAULT_DIR                           0x3D06
#define MAPI_AB_DEFAULT_PAB                           0x3D07
#define MAPI_FILTERING_HOOKS                          0x3D08
#define MAPI_SERVICE_NAME                             0x3D09
#define MAPI_SERVICE_DLL_NAME                         0x3D0A
#define MAPI_SERVICE_ENTRY_NAME                       0x3D0B
#define MAPI_SERVICE_UID                              0x3D0C
#define MAPI_SERVICE_EXTRA_UIDS                       0x3D0D
#define MAPI_SERVICES                                 0x3D0E
#define MAPI_SERVICE_SUPPORT_FILES                    0x3D0F
#define MAPI_SERVICE_DELETE_FILES                     0x3D10
#define MAPI_AB_SEARCH_PATH_UPDATE                    0x3D11
#define MAPI_PROFILE_NAME                             0x3D12
#define MAPI_IDENTITY_DISPLAY                         0x3E00
#define MAPI_IDENTITY_ENTRYID                         0x3E01
#define MAPI_RESOURCE_METHODS                         0x3E02
#define MAPI_RESOURCE_TYPE                            0x3E03
#define MAPI_STATUS_CODE                              0x3E04
#define MAPI_IDENTITY_SEARCH_KEY                      0x3E05
#define MAPI_OWN_STORE_ENTRYID                        0x3E06
#define MAPI_RESOURCE_PATH                            0x3E07
#define MAPI_STATUS_STRING                            0x3E08
#define MAPI_X400_DEFERRED_DELIVERY_CANCEL            0x3E09
#define MAPI_HEADER_FOLDER_ENTRYID                    0x3E0A
#define MAPI_REMOTE_PROGRESS                          0x3E0B
#define MAPI_REMOTE_PROGRESS_TEXT                     0x3E0C
#define MAPI_REMOTE_VALIDATE_OK                       0x3E0D
#define MAPI_CONTROL_FLAGS                            0x3F00
#define MAPI_CONTROL_STRUCTURE                        0x3F01
#define MAPI_CONTROL_TYPE                             0x3F02
#define MAPI_DELTAX                                   0x3F03
#define MAPI_DELTAY                                   0x3F04
#define MAPI_XPOS                                     0x3F05
#define MAPI_YPOS                                     0x3F06
#define MAPI_CONTROL_ID                               0x3F07
#define MAPI_INITIAL_DETAILS_PANE                     0x3F08
#define MAPI_ID_SECURE_MIN                            0x67F0
#define MAPI_ID_SECURE_MAX                            0x67FF
/* }}} */

/* {{{ tnef */
struct tnef_attrs_t {
    int lvl_type;
    int type;
    int name;
    int vlen;
    char *val;
};
typedef struct tnef_attrs_t tnef_attrs_t;


static inline unsigned int GETINT32(unsigned char *p)
{
    return (unsigned int)((unsigned int)(p)[0] +((unsigned int)(p)[1]<<8) +((unsigned int)(p)[2]<<16) +((unsigned int)(p)[3]<<24));
}

static inline unsigned int GETINT16 (unsigned char* p)
{
    return (unsigned int)((unsigned int)(p)[0]+((unsigned int)(p)[1]<<8));
}

static inline unsigned int GETINT8 (unsigned char *p)
{
    return (unsigned int)(p)[0];
}

static char *tnef_unicode_to_utf8 (int type, char *_buf, int len)
{
    if (type != szMAPI_UNICODE_STRING) {
        return zmemdupnull(_buf, len);
    }
    int i = 0;
    int j = 0;
    unsigned char *buf = (unsigned char *)_buf;
    unsigned char *utf8 = (unsigned char *)zmalloc (3 * len/2 + 1); /* won't get any longer than this */

    if (len > 0) {
        for (i = 0; i < len - 1; i += 2) {
            unsigned int c = GETINT16(buf + i);
            if (c <= 0x007f) {
                utf8[j++] = 0x00 | ((c & 0x007f) >> 0);
            } else if (c < 0x07ff) {
                utf8[j++] = 0xc0 | ((c & 0x07c0) >> 6);
                utf8[j++] = 0x80 | ((c & 0x003f) >> 0);
            } else {
                utf8[j++] = 0xe0 | ((c & 0xf000) >> 12);
                utf8[j++] = 0x80 | ((c & 0x0fc0) >> 6);
                utf8[j++] = 0x80 | ((c & 0x003f) >> 0);
            }
        }
    }

    utf8[j] = '\0';

    return (char *)utf8;
}

static int tnef_get_attr(ztnef_t *parser, ztnef_reader_t *reader,  tnef_attrs_t *attr)
{
    if ((attr->lvl_type = tnef_geti8(reader)) < 0) {
        return -1;
    }

    if ((attr->name = tnef_geti16(reader)) < 0) {
        return -1;
    }

    if ((attr->type = tnef_geti16(reader)) < 0) {
        return -1;
    }

    if ((attr->vlen = tnef_geti32(reader)) < 0) {
        return -1;
    }

    if (tnef_getx(reader, attr->vlen, &(attr->val)) < 0) {
        return -1;
    }

    if (tnef_geti16(reader) < 0) {
        return -1;
    }

    return 0;
}

static const unsigned int rtf_uncompressed_magic = 0x414c454d;
static const unsigned int rtf_compressed_magic =   0x75465a4c;

static int decompress_rtf_data(unsigned char *src, size_t lenc, int lenu, unsigned char *dest)
{
    char rtf_prebuf[] = "{\\rtf1\\ansi\\mac\\deff0\\deftab720{\\fonttbl;}{\\f0\\fnil \\froman \\fswiss \\fmodern \\fscript \\fdecor MS Sans SerifSymbolArialTimes New RomanCourier{\\colortbl\\red0\\green0\\blue0\r\n\\par \\pard\\plain\\f0\\fs20\\b\\i\\u\\tab\\tx";
    size_t rtf_prebuf_len = sizeof(rtf_prebuf) - 1;

    int woff, eoff, roff, rlen;
    int control, cin, cout, i, j, endflag;
    unsigned char dict[4096];

    memset(dict, 0x0, sizeof(dict));
    memcpy(dict, rtf_prebuf, rtf_prebuf_len);

    woff = rtf_prebuf_len;
    eoff = rtf_prebuf_len;

    cout = 0;
    cin = 0;
    endflag = 0;

    while (1) {
        if (endflag){
            break;
        }

        if (cin+1 > lenc) {
            endflag = -1;
            break;
        }

        control = (int)src[cin++];
        for (i=0; i<8; i++) {
            if (endflag) {
                break;
            }

            if (control & (1<<i)) {
                if (cin+2 > lenc) {
                    endflag = -1;
                    break;
                }

                roff = (int)src[cin++];
                rlen = (int)src[cin++];

                roff = (roff<<4) + (rlen>>4);
                rlen = (rlen&0x0f) + 2;

                if (roff == woff) {
                    endflag = 1;
                    break;
                }

                if (cout+rlen > lenu) {
                    endflag = -1;
                    break;
                }

                for (j=0; j<rlen; j++) {
                    dest[cout++] = dict[roff];
                    dict[woff++] = dict[roff++];

                    roff &= 0xfff;
                    woff &= 0xfff;
                    if (eoff < 4096) eoff++;
                }
            } else {
                if (cin+1 > lenc) {
                    endflag = -1;
                    break;
                }

                if (cout+1 > lenu) {
                    endflag = -1;
                    break;
                }

                dest[cout++] = src[cin];
                dict[woff++] = src[cin++];

                woff &= 0xfff;
                if (eoff < 4096) eoff++;
            }
        }
    }
    
    dest[cout] = 0;
    return cout;
}

static int get_rtf_data_from_buf(ztnef_mime_t *m, unsigned char *data, size_t len)
{
    size_t compr_size = 0L;
    size_t uncompr_size = 0L;
    unsigned int magic;
    size_t idx = 0;
    
    if (len < 20) {
        return -1;
    }
    compr_size = GETINT32(data + idx); idx += 4;
    uncompr_size = GETINT32(data + idx); idx += 4;
    magic = GETINT32(data + idx); idx += 4;
    GETINT32 (data + idx); idx += 4;

    if (compr_size+4 != len) {
        return -1;
    }

    if (magic == rtf_uncompressed_magic) {
        if (uncompr_size+4 > len) {
            return -1;
        }
        m->body_data = (char *)data+4;
        m->body_len = uncompr_size;
    } else if (magic == rtf_compressed_magic) {
        if (uncompr_size > 1024 * 1024 *100) {
            return -1;
        }
        unsigned char * up = (unsigned char *)zmalloc(uncompr_size+2);
        if ((m->body_len=decompress_rtf_data(data+idx, len-idx, uncompr_size, up)) < 0) {
            zfree(up);
            return -1;
        } else {
            m->body_data = m->body_data2 = (char *)up;
        }
    } else {
        return -1;
    }

    return 0;
}

static int tnef_decode_extract_mapi_attrs(ztnef_t *parser, ztnef_reader_t *reader, int message_or_attachment)
{
    int i, j, num_properties;
    int type, name, num_values, num_names, nlen, vlen = 0;
    char *vdata;
    ztnef_mime_t *m;

    num_properties = tnef_geti32(reader);
    if (num_properties < 0) {
        return -1;
    }
    for (i=0;i<num_properties;i++) {
        type = tnef_geti16(reader);
        name = tnef_geti16(reader);
        if (name & GUID_EXISTS_FLAG) {
            /* skip guid */
            tnef_skip(reader, 16);
            num_names = tnef_geti32(reader);
            if (num_names < 0) {
                return -1;
            } else if (num_names > 0) {
                for (j=0;j<num_names;j++) {
                    nlen = tnef_geti32(reader);
                    int skip = tnef_pad_to_4byte(nlen);
                    if (skip < 0) {
                        return -1;
                    }
                    tnef_skip(reader, skip);
                }
            } else {
                name = tnef_geti32(reader);
            }
        }
        if ((type & MULTI_VALUE_FLAG) || (type == szMAPI_STRING) || (type == szMAPI_UNICODE_STRING) || (type == szMAPI_OBJECT) || (type == szMAPI_BINARY)) {
            num_values = tnef_geti32(reader);
        } else {
            num_values = 1;
        }
        if (type & MULTI_VALUE_FLAG) {
            type -= MULTI_VALUE_FLAG;
        }

        for (j = 0; j < num_values; j++) {
            vdata = 0;
            switch (type) {
            case szMAPI_SHORT:
                tnef_geti16(reader);
                tnef_geti16(reader);
                break;
            case szMAPI_INT:
            case szMAPI_FLOAT:
            case szMAPI_BOOLEAN:
                tnef_geti32(reader);
                break;
            case szMAPI_SYSTIME:
            case szMAPI_DOUBLE:
            case szMAPI_APPTIME:
            case szMAPI_CURRENCY:
            case szMAPI_INT8BYTE:
                tnef_geti32(reader);
                tnef_geti32(reader);
                break;
            case szMAPI_CLSID:
                tnef_skip(reader, 16);
                break;

            case szMAPI_STRING:
            case szMAPI_UNICODE_STRING:
            case szMAPI_OBJECT:
            case szMAPI_BINARY:
                vlen = tnef_geti32(reader);
                if (vlen < 0) {
                    return -1;
                }
                vdata = reader->pos;
                int skip = tnef_pad_to_4byte(vlen);
                if (skip < 0) {
                    return -1;
                }
                tnef_skip(reader, skip);
                break;
            case szMAPI_NULL:
            case szMAPI_ERROR:
            case szMAPI_UNSPECIFIED:
            default:
                return -1;
            }
            if (message_or_attachment == 1) {
                if ((type == szMAPI_BINARY) && (name == MAPI_BODY_HTML)) {
                    if (vdata) {
                        m = ztnef_mime_create(parser);
                        zvector_push(parser->all_mimes, m);
                        zvector_push(parser->text_mimes, m);
                        m->type = zstrdup("text/html");
                        m->body_data = vdata;
                        m->body_len = vlen;
                    }
                } else if ((type == szMAPI_BINARY) && (name == MAPI_RTF_COMPRESSED)) {
                    if (vdata) {
                        m = ztnef_mime_create(parser);
                        m->type = zstrdup("text/rtf");
                        if (get_rtf_data_from_buf(m, (unsigned char *)vdata, (size_t)vlen) < 0) {
                            ztnef_mime_free(m);
                        } else {
                            zvector_push(parser->all_mimes, m);
                            zvector_push(parser->text_mimes, m);
                        }
                    }
                } else {
                }
            }

            if (message_or_attachment == 0) {
                ztnef_mime_t *m;
                if (vdata && (zvector_len(parser->all_mimes))) {
                    m = (ztnef_mime_t *)(zvector_data(parser->all_mimes)[zvector_len(parser->all_mimes)-1]);
                    switch(name) {
                    case MAPI_ATTACH_LONG_FILENAME:
                        zfree(m->filename);
                        m->filename = zmail_clear_null_inner(tnef_unicode_to_utf8(type, vdata, vlen), -1);
                        m->is_szMAPI_UNICODE_STRING = 0;
                        if (type == szMAPI_UNICODE_STRING){
                            m->is_szMAPI_UNICODE_STRING = 1;
                        }
                        break;
                    case MAPI_ATTACH_DATA_OBJ:
                        m->body_data = vdata;
                        m->body_len = vlen;
                        break;
                    case MAPI_ATTACH_MIME_TAG:
                        zfree(m->type);
                        m->type = tnef_unicode_to_utf8(type, vdata, vlen);
                        break;
                    case MAPI_ATTACH_CONTENT_ID:
                        zfree(m->content_id);
                        m->content_id = tnef_unicode_to_utf8(type, vdata, vlen);
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

static int tnef_decode_attachment(ztnef_t *parser, ztnef_reader_t *reader, tnef_attrs_t *attr)
{
    ztnef_mime_t *m;
    ztnef_reader_t reader2;
    switch(attr->name) {
    case attATTACHRENDDATA:
        m = ztnef_mime_create(parser);
        m->is_att = 1;
        zvector_push(parser->all_mimes, m);
        zvector_push(parser->attachment_mimes, m);
        break;
    case attATTACHMODIFYDATE:
        break;
    case attATTACHMENT:
        if (attr->val + attr->vlen <= reader->end) {
            memset(&reader2, 0, sizeof(ztnef_reader_t));
            reader2.start = attr->val;
            reader2.pos = attr->val;
            reader2.end = attr->val + attr->vlen;
            if (tnef_decode_extract_mapi_attrs(parser, &reader2, 0) < 0) {
            }
        }
        break;
    case attATTACHTITLE:
        if (zvector_len(parser->all_mimes)) {
            m = (ztnef_mime_t *)(zvector_data(parser->all_mimes)[zvector_len(parser->all_mimes)-1]);
            zfree(m->filename);
            m->filename = tnef_unicode_to_utf8(attr->type, attr->val, attr->vlen);
            m->is_szMAPI_UNICODE_STRING = 0;
            if (attr->type == szMAPI_UNICODE_STRING){
                m->is_szMAPI_UNICODE_STRING = 1;
            }
        }
        break;
    case attATTACHDATA:
        if (zvector_len(parser->all_mimes)) {
            m = (ztnef_mime_t *)(zvector_data(parser->all_mimes)[zvector_len(parser->all_mimes)-1]);
            m->body_data = attr->val;
            m->body_len = attr->vlen;
        }
        break;
    case attOEMCODEPAGE:
        break;
    default:
        break;
    }
    return 0;
}

static int tnef_decode_message(ztnef_t *parser, ztnef_reader_t *reader,  tnef_attrs_t *attr)
{
    ztnef_mime_t *m;
    if (attr->name == attBODY) {
        m = ztnef_mime_create(parser);
        zvector_push(parser->all_mimes, m);
        zvector_push(parser->text_mimes, m);
        m->type = zstrdup("text/plain");
        m->body_data = attr->val;
        m->body_len = attr->vlen;
    } else if (attr->name == attMAPIPROPS) {
        if (attr->val + attr->vlen <= reader->end) {
            ztnef_reader_t reader2;
            memset(&reader2, 0, sizeof(ztnef_reader_t));
            reader2.start = attr->val;
            reader2.pos = attr->val;
            reader2.end = attr->val + attr->vlen;
            if (tnef_decode_extract_mapi_attrs(parser, &reader2, 1) < 0) {
            }
        }
    } else if (attr->name == attOEMCODEPAGE) {
        if (attr->vlen >= 4) {
            parser->codepage = GETINT32((unsigned char *)(attr->val));
        }
    } else {
    }
    return 0;
}

static int ___mime_decode_tnef(ztnef_t *parser, ztnef_reader_t *reader)
{
    tnef_attrs_t attr;
    
    /* signature */
    if (tnef_geti32(reader) != TNEF_SIGNATURE) {
        return -1;
    }

    /* key */
    if (tnef_geti16(reader) < 0) {
        return -1;
    }

    /* a series of 'messages' and 'attachments' */
    while (___LEFT(reader)) {
        if (tnef_get_attr(parser, reader, &attr) < 0) {
            return -1;
        }
        if (attr.lvl_type == LVL_MESSAGE) {
            if (tnef_decode_message(parser, reader, &attr)<0) {
                return -1;
            }
        } else if (attr.lvl_type == LVL_ATTACHMENT) {
            if (tnef_decode_attachment(parser, reader, &attr)<0) {
                return -1;
            }
        } else {
            return -1;
        }
    }

    return 0;
}
/* }}} */

/*
 * vim600: fdm=marker
 */
