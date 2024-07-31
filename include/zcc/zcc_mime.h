/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_MAIL___
#define ZCC_LIB_INCLUDE_MAIL___

#include <functional>
#include <tuple>
#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

class mail_parser_running_context;

enum mail_header_encode_type
{
    mail_header_encode_base64 = 1,
    mail_header_encode_qp,
    mail_header_encode_none,
};

class mail_parser
{
    friend mail_parser_running_context;

public:
    class header_line_node
    {
    public:
        inline header_line_node() {}
        inline ~header_line_node() {}
        void reset();

    public:
        std::string charset_;
        std::string data_;
        mail_header_encode_type encode_type_{mail_header_encode_none};
    };

    class mail_address
    {
    public:
        inline mail_address() {}
        inline ~mail_address() {}

    public:
        std::string name_;
        std::string mail_;
        std::string name_utf8_;
    };
    class mime_node
    {
        friend mail_parser_running_context;
        friend mail_parser;

    public:
        mime_node(mail_parser &parser);
        ~mime_node();

        inline const std::string &get_content_type() { return content_type_; }
        const std::string &get_encoding();
        const std::string &get_content_id();
        inline const std::string &get_boundary()
        {
            return boundary_;
        }
        inline const std::string &get_charset()
        {
            return charset_;
        }
        const std::string &get_disposition();

        inline const std::string &get_name()
        {
            return name_;
        }
        const std::string &get_name_utf8();
        const std::string &get_filename();
        const std::string &get_filename_utf8();
        const std::string &get_filename2231(bool *with_charset_flag = 0);
        const std::string &get_show_name();
        const std::string get_decoded_content();
        const std::string get_decoded_content_utf8();

        inline const char *get_header_data()
        {
            return parser_.mail_data_ + header_offset_;
        }

        inline int64_t get_header_offset()
        {
            return header_offset_;
        }

        inline int64_t get_header_size()
        {
            return header_size_;
        }

        inline const char *get_body_data()
        {
            return parser_.mail_data_ + body_offset_;
        }

        inline int64_t get_body_offset()
        {
            return body_offset_;
        }

        inline int64_t get_body_size()
        {
            return body_size_;
        }

        inline bool is_tnef()
        {
            return is_tnef_;
        }
        const std::string &get_imap_section();

        mime_node *get_next()
        {
            return next_;
        }

        mime_node *get_child()
        {
            return child_head_;
        }

        mime_node *get_parent()
        {
            return parent_;
        }
        //
        inline const std::vector<size_data> get_raw_header_line_vector()
        {
            return raw_header_lines_;
        }
        int64_t get_raw_header_line_ptr(const char *header_name, int64_t header_name_len, char **result, int64_t sn = 0);
        bool get_raw_header_line(const char *header_name, std::string &result, int64_t sn = 0);
        std::string get_raw_header_line(const char *header_name, int64_t sn = 0);
        int64_t get_header_line_value(const char *header_name, std::string &result, int64_t sn = 0);
        std::string get_header_line_value(const char *header_name, int64_t sn = 0);

    private:
        mail_parser &parser_;
        std::string content_type_;
        std::string content_id_;
        std::string boundary_;
        std::string charset_;
        std::string name_;
        std::string name_utf8_;
        std::string encoding_;
        std::string disposition_;
        std::string filename_;
        std::string filename2231_;
        std::string filename_utf8_;
        std::string *show_name_{nullptr};
        std::string imap_section_;
        int64_t header_offset_{0};
        int64_t header_size_{0};
        int64_t body_offset_{0};
        int64_t body_size_{0};
        bool is_tnef_{false};
        bool filename2231_with_charset_flag_{false};
        //
        std::vector<size_data> raw_header_lines_;
        char mime_type_;
        bool is_multipart_{false};
        //
        mime_node *parent_{nullptr};
        mime_node *prev_{nullptr};
        mime_node *next_{nullptr};
        mime_node *child_head_{nullptr};
        mime_node *child_tail_{nullptr};
        //
        bool content_type_flag_{false};
        bool encoding_flag_{false};
        bool disposition_flag_{false};
        bool name_utf8_flag_{false};
        bool filename_utf8_flag_{false};
        bool filename2231_flag_{false};
        bool content_id_flag_{false};
    };

public:
    //
    static const int64_t header_line_max_length = 1024000;
    //
    static std::string charset_convert(const char *from_charset, const char *data, int64_t size);
    inline static std::string charset_convert(const char *from_charset, const std::string &data)
    {
        return charset_convert(from_charset, data.c_str(), data.size());
    }
    //
    static std::string decode_2231(const char *src_charset_def, const char *in_line, int64_t in_len, bool with_charset_flag = true);
    inline static std::string decode_2231(const char *src_charset_def, const std::string &in, bool with_charset_flag = true)
    {
        return decode_2231(src_charset_def, in.c_str(), in.size(), with_charset_flag);
    }
    //
    static int64_t decode_date(const char *str);
    inline static int64_t decode_date(const std::string &str)
    {
        return decode_date(str.c_str());
    }

    //
    static std::string header_line_unescape(const char *in_line, int64_t in_len);
    inline static std::string header_line_unescape(const std::string &in)
    {
        return header_line_unescape(in.c_str(), in.size());
    }
    //
    static int64_t header_line_get_first_token(const char *line, int64_t in_len, char **val);
    static std::string header_line_get_first_token(const char *line, int64_t in_len);
    inline static std::string header_line_get_first_token(const std::string &line)
    {
        return header_line_get_first_token(line.c_str(), line.size());
    }
    //
    static std::vector<header_line_node> header_line_to_node_vector(const char *in_line, int64_t in_len, int64_t max_count = 102400);
    inline static std::vector<header_line_node> header_line_to_node_vector(const std::string &line, int64_t max_count = 102400)
    {
        return header_line_to_node_vector(line.c_str(), line.size(), max_count);
    }
    //
    static std::string header_line_get_utf8(const char *src_charset_def, const char *in_line, int64_t in_len = -1);
    inline static std::string header_line_get_utf8(const char *src_charset_def, const std::string &line)
    {
        return header_line_get_utf8(src_charset_def, line.c_str(), line.size());
    }
    //
    static std::vector<mail_address> header_line_get_address_vector(const char *src_charset_def, const char *in_str, int64_t in_len);
    inline static std::vector<mail_address> header_line_get_address_vector(const char *src_charset_def, const std::string &str)
    {
        return header_line_get_address_vector(src_charset_def, str.c_str(), str.size());
    }
    static std::vector<mail_address> header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int64_t in_len);
    inline static std::vector<mail_address> header_line_get_address_vector_utf8(const char *src_charset_def, const std::string &str)
    {
        return header_line_get_address_vector_utf8(src_charset_def, str.c_str(), str.size());
    }
    //
    static void header_line_get_params(const char *in_line, int64_t in_len, std::string &val, std::vector<std::tuple<std::string, std::string>> &params);
    inline static void header_line_get_params(const std::string &line, std::string &val, std::vector<std::tuple<std::string, std::string>> &params)
    {
        header_line_get_params(line.c_str(), line.size(), val, params);
    }
    static void header_line_get_params(const char *in_line, int64_t in_len, std::string &val, dict &params);
    inline static void header_line_get_params(const std::string &line, std::string &val, dict &params)
    {
        header_line_get_params(line.c_str(), line.size(), val, params);
    }

public:
    static mail_parser *create_from_data(const char *mail_data, int64_t mail_data_len, const char *default_charset = nullptr);
    inline static mail_parser *create_from_data(const std::string &mail_data, const char *default_charset = nullptr)
    {
        return create_from_data(mail_data.c_str(), mail_data.size(), default_charset);
    }
    static mail_parser *create_from_file(const char *pathname, const char *default_charset = nullptr);
    inline static mail_parser *create_from_file(const std::string &pathname, const char *default_charset = nullptr)
    {
        return create_from_file(pathname.c_str(), default_charset);
    }

private:
    mail_parser();

public:
    ~mail_parser();
    inline const char *get_mail_data()
    {
        return mail_data_;
    }

    inline int64_t get_mail_size()
    {
        return mail_size_;
    }

    inline const char *get_header_data()
    {
        return mail_data_ + top_mime_->header_offset_;
    }

    inline int64_t get_header_offset()
    {
        return top_mime_->header_offset_;
    }

    inline int64_t get_header_size()
    {
        return top_mime_->header_size_;
    }

    inline const char *get_body_data()
    {
        return mail_data_ + top_mime_->body_offset_;
    }

    inline int64_t get_body_offset()
    {
        return top_mime_->body_offset_;
    }

    inline int64_t get_body_size()
    {
        return top_mime_->body_size_;
    }
    const std::string &get_subject();
    const std::string &get_subject_utf8();
    const std::string &get_message_id();
    const std::string &get_date();
    int64_t get_date_unix();
    int64_t get_date_unix_by_received();
    const mail_address &get_from();
    const mail_address &get_from_utf8();
    const mail_address &get_sender();
    const mail_address &get_reply_to();
    const mail_address &get_receipt();
    const std::string &get_in_reply_to();
    const std::vector<mail_address> &get_to();
    const std::vector<mail_address> &get_to_utf8();
    const std::vector<mail_address> &get_cc();
    const std::vector<mail_address> &get_cc_utf8();
    const std::vector<mail_address> &get_bcc();
    const std::vector<mail_address> &get_bcc_utf8();
    inline const mime_node *get_top_mime()
    {
        return top_mime_;
    }
    inline const std::vector<mime_node *> &get_all_mimes()
    {
        return all_mimes_;
    }
    const std::vector<mime_node *> &get_text_mimes();
    const std::vector<mime_node *> &get_show_mimes();
    const std::vector<mime_node *> &get_attachment_mimes();
    const std::vector<std::string> &get_references();

    /* n == 0: first, n == -1: last */
    inline const std::vector<size_data> get_raw_header_line_vector()
    {
        return top_mime_->get_raw_header_line_vector();
    }
    bool get_raw_header_line(const char *header_name, std::string &result, int64_t sn = 0)
    {
        return top_mime_->get_raw_header_line(header_name, result, sn);
    }
    std::string get_raw_header_line(const char *header_name, int64_t sn = 0)
    {
        return top_mime_->get_raw_header_line(header_name, sn);
    }
    inline int64_t get_header_line_value(const char *header_name, std::string &result, int64_t sn = 0)
    {
        return top_mime_->get_header_line_value(header_name, result, sn);
    }
    inline std::string get_header_line_value(const char *header_name, int64_t sn = 0)
    {
        return top_mime_->get_header_line_value(header_name, sn);
    }
    void debug_show();

    //
private:
    void classify();
    int64_t classify_mime_identify_type(mime_node *mime);
    void classify_mime_identify_view_part(mime_node *mime, void *view_list, int64_t *view_len);
    void imap_section();

private:
    const char *mail_data_{nullptr};
    int64_t mail_size_{0};
    std::string default_charset_;
    mmap_reader reader_;
    //
    std::string subject_;
    std::string subject_utf8_;
    std::string message_id_;
    std::string date_;
    int64_t date_unix_;
    mail_address from_;
    mail_address sender_;
    mail_address reply_to_;
    mail_address receipt_;
    std::string in_reply_to_;
    std::vector<mail_address> to_;
    std::vector<mail_address> cc_;
    std::vector<mail_address> bcc_;
    std::vector<std::string> references_;
    std::string imap_section_;
    //
    mime_node *top_mime_{nullptr};
    std::vector<mime_node *> all_mimes_;
    std::vector<mime_node *> text_mimes_;
    std::vector<mime_node *> show_mimes_;
    std::vector<mime_node *> attachment_mimes_;
    //
    bool mmap_reader_mode_{false};
    bool subject_flag_{false};
    bool subject_utf8_flag_{false};
    bool message_id_flag_{false};
    bool date_flag_{false};
    bool date_unix_flag_{false};
    char from_flag_{0};
    char sender_flag_{0};
    char reply_to_flag_{0};
    char receipt_flag_{0};
    bool in_reply_to_flag_{false};
    char to_flag_{0};
    char cc_flag_{0};
    char bcc_flag_{0};
    bool references_flag_{false};
    bool classify_flag_{false};
    bool imap_section_flag_{false};
};

class tnef_parser
{
    friend class tnef_parser_running_context;

public:
    class mime_node
    {
        friend class tnef_parser_running_context;
        friend class tnef_parser;

    public:
        mime_node(tnef_parser &parser);
        ~mime_node();
        inline const std::string get_content_type()
        {
            return content_type_;
        }
        inline const std::string &get_content_id()
        {
            return content_id_;
        }
        inline const std::string &get_charset()
        {
            return parser_.charset_;
        }
        inline bool is_attachment()
        {
            return is_att_;
        }
        inline const std::string &get_filename()
        {
            return filename_;
        }
        const std::string &get_show_name();
        const std::string &get_filename_utf8();

        inline const char *get_body_data()
        {
            return body_data_;
        }
        inline int64_t get_body_size()
        {
            return body_size_;
        }

    private:
        tnef_parser &parser_;
        std::string content_type_;
        std::string filename_;
        std::string filename_utf8_;
        std::string content_id_;
        const char *body_data_;
        int64_t body_size_;
        //
        bool is_att_{false};
        bool is_szMAPI_UNICODE_STRING_{false};
        bool filename_utf8_flag_{false};
        bool body_need_delete_{false};
    };

public:
    static tnef_parser *create_from_data(const char *mail_data, int64_t mail_data_len, const char *default_charset = nullptr);
    inline static tnef_parser *create_from_data(const std::string &mail_data, const char *default_charset = nullptr)
    {
        return create_from_data(mail_data.c_str(), mail_data.size(), default_charset);
    }
    static tnef_parser *create_from_file(const char *pathname, const char *default_charset = nullptr);
    inline static tnef_parser *create_from_file(const std::string &pathname, const char *default_charset = nullptr)
    {
        return create_from_file(pathname.c_str(), default_charset);
    }

private:
    tnef_parser();
    void parse();

public:
    ~tnef_parser();
    inline const char *get_data()
    {
        return tnef_data_;
    }
    inline int64_t get_size()
    {
        return tnef_size_;
    }
    inline const std::vector<mime_node *> &get_all_mimes()
    {
        return all_mimes_;
    }
    inline const std::vector<mime_node *> &get_text_mimes()
    {
        return text_mimes_;
    }
    inline const std::vector<mime_node *> &get_attachment_mimes()
    {
        return attachment_mimes;
    }
    void debug_show();

private:
    std::string default_charset_;
    std::vector<mime_node *> all_mimes_;
    std::vector<mime_node *> text_mimes_;
    std::vector<mime_node *> attachment_mimes;
    const char *tnef_data_{nullptr};
    int64_t tnef_size_;
    int codepage_;
    std::string charset_;
    mmap_reader reader_;
    unsigned char fmmap_flag : 1;
    bool mmap_reader_mode_{false};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_MAIL___
