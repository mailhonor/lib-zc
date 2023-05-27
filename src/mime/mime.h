/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

void zvector_init_mpool(zvector_t *v, int size, zmpool_t *mpool);
void zargv_init_mpool(zargv_t *argvp, int size, zmpool_t *mpool);

typedef struct zbuf_node_t zbuf_node_t;
struct zbuf_node_t {
    zbuf_node_t *prev;
    zbuf_node_t *next;
    zbuf_t buf;
};


struct zmime_t {
    unsigned int type_flag:1;
    unsigned int encoding_flag:1;
    unsigned int disposition_flag:1;
    unsigned int show_name_flag:1;
    unsigned int name_flag:1;
    unsigned int name_utf8_flag:1;
    unsigned int filename_flag:1;
    unsigned int filename2231_flag:1;
    unsigned int filename_utf8_flag:1;
    unsigned int content_id_flag:1;
    unsigned int filename2231_with_charset:1;
    unsigned int is_tnef:1;
    unsigned int is_multipart:1; /* inner */
    unsigned int mime_type:3; /* inner */
    unsigned int parser_stage_inner:4; /* inner */

    short int boundary_len;

    char *type;
    char *encoding;
    char *charset;
    char *disposition;
    char *show_name;
    char *name;
    char *name_utf8;
    char *filename;
    char *filename2231;
    char *filename_utf8;
    char *content_id;
    char *boundary;
    int header_offset;
    int header_len;
    int body_offset;
    int body_len;

    /* mime original header-logic-line */
    zvector_t raw_header_lines; /* size_data_t * */
    void * raw_header_lines_mpool_ptr;

    /* relationship */
    zmime_t *parent;
    zmime_t *child_head;
    zmime_t *child_tail;
    zmime_t *next;
    zmime_t *prev;

    /* */
    zmail_t * parser;

    /* mime proto, for imapd */
    char *imap_section;
};

struct zmail_t {
    unsigned short int subject_flag:1;
    unsigned short int subject_utf8_flag:1;
    unsigned short int date_flag:1;
    unsigned short int date_unix_flag:1;
    unsigned short int message_id_flag:1;
    unsigned short int in_reply_to_flag:1;
    unsigned short int from_flag:2;
    unsigned short int sender_flag:2;
    unsigned short int reply_to_flag:2;
    unsigned short int to_flag:2;
    unsigned short int cc_flag:2;
    unsigned short int bcc_flag:2;
    unsigned short int receipt_flag:2;
    unsigned short int references_flag:2;
    unsigned short int classify_flag:2;
    unsigned short int section_flag:2;
    char *subject;
    char *subject_utf8;
    char *date;
    long date_unix;
    zmime_address_t *from;
    zmime_address_t *sender;
    zmime_address_t *reply_to;
    zmime_address_t *receipt;
    zvector_t *to; /* zmime_address_t * */
    zvector_t *cc;
    zvector_t *bcc;
    char *in_reply_to;
    char *message_id;
    zargv_t *references;

    /* mime-tree */
    zmime_t *top_mime;

    /* all-mime-std::list */
    zvector_t *all_mimes; /* zmime_t * */

    /* text(plain,html) type mime-list except for attachment */
    zvector_t *text_mimes; 

    /* similar to the above, 
     * in addition to the case of alternative, html is preferred */
    zvector_t *show_mimes;

    /* attachment(and background-image) type mime-list */
    zvector_t *attachment_mimes;

    /* option */
    char *src_charset_def;

    /* email data */
    char *mail_data;
    int mail_size;
    zmmap_reader_t *fmmap;

    /* greedy mpool */
    zmpool_t *mpool;

    /* cache */
    zbuf_node_t *zbuf_cache_head;
    zbuf_node_t *zbuf_cache_tail;
};

zmime_t *zmime_create(zmail_t *parser);
void zmime_free(zmime_t *mime);
int zmail_decode_mime_inner(zmail_t * parser);

char *zmail_clear_null_inner(const void *data, int size);

zbuf_t *zmail_zbuf_cache_require(zmail_t *parser, int len);
void zmail_zbuf_cache_release(zmail_t *parser, zbuf_t *bf);
void zmail_zbuf_cache_release_all(zmail_t *parser);

int zmime_raw_header_line_unescape_inner(zmail_t *parser, const char *data, int size, char *dest, int dest_size);

int zmime_get_raw_header_line_ptr(zmime_t *mime, const char *header_name, char **result, int sn);
int zmail_decode_mime_inner(zmail_t * parser);
zvector_t *zmime_header_line_get_address_vector_inner(zmail_t *parser, const char *in_str, int in_len);

void zmime_header_line_get_utf8_inner(zmail_t *parser, const char *in_line, int in_len, zbuf_t *result);
void zmime_header_line_decode_content_type_inner(zmail_t *parser, const char *data, int len, char **_value, char **boundary, int *boundary_len, char **charset, char **name);
void zmime_header_line_decode_content_disposition_inner(zmail_t *parser, const char *data, int len, char **_value, char **filename, char **filename_2231, int *filename_2231_with_charset_flag);
void zmime_header_line_decode_content_transfer_encoding_inner(zmail_t *parser, const char *data, int len, char **_value);
