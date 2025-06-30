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

// 邮件解析
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

    // 邮件地址
    class mail_address
    {
    public:
        inline mail_address() {}
        inline ~mail_address() {}

    public:
        // 姓名, 原始字段形如: =?GB2312?=xxxxxx?= 等等
        std::string name_;
        // 邮件地址
        std::string mail_;
        // 姓名, 解码,字符转码后
        std::string name_utf8_;
    };

    // 邮件解析后的节点(附件/正文等等)
    class mime_node
    {
        friend mail_parser_running_context;
        friend mail_parser;

    private:
        mime_node(mail_parser &parser);

    public:
        ~mime_node();
        // Content-Type
        inline const std::string &get_content_type() { return content_type_; }
        // Content-Transfer-Encoding
        const std::string &get_encoding();
        // Content-ID
        const std::string &get_content_id();
        // Boundary
        inline const std::string &get_boundary()
        {
            return boundary_;
        }
        // 字符集
        inline const std::string &get_charset()
        {
            return charset_;
        }
        // Content-Disposition, inline/attachment
        const std::string &get_disposition();
        // name_, 原始字段
        inline const std::string &get_name()
        {
            return name_;
        }
        // name_, 解码转码后
        const std::string &get_name_utf8();
        // filename, 原始字段
        const std::string &get_filename();
        // filename, 解码转码后
        const std::string &get_filename_utf8();
        const std::string &get_filename2231(bool *with_charset_flag = 0);
        // 应该显示的 name_
        const std::string &get_show_name();
        // 解码后的附件内容
        const std::string get_decoded_content();
        // 解码后, 并转码为UTF8的内容, 用于 文本类节点(text/plain, text/html)
        const std::string get_decoded_content_utf8();

        // 节点头的地址
        inline const char *get_header_data()
        {
            return parser_.mail_data_ + header_offset_;
        }

        // 节点头偏移
        inline int64_t get_header_offset()
        {
            return header_offset_;
        }

        // 节点头的大小
        inline int64_t get_header_size()
        {
            return header_size_;
        }

        // 节点正文地址
        inline const char *get_body_data()
        {
            return parser_.mail_data_ + body_offset_;
        }

        // 节点正文偏移
        inline int64_t get_body_offset()
        {
            return body_offset_;
        }

        // 节点正文大小
        inline int64_t get_body_size()
        {
            return body_size_;
        }

        // 是否 tnef 类附件, winmail.dat
        inline bool is_tnef()
        {
            return is_tnef_;
        }

        // 计算 section, 一般用于imap服务器开发
        const std::string &get_imap_section();

        // 下一个节点
        mime_node *get_next()
        {
            return next_;
        }

        // 第一个子节点
        mime_node *get_child()
        {
            return child_head_;
        }

        // 父节点
        mime_node *get_parent()
        {
            return parent_;
        }

        // 原始节点头, 按行分解
        inline const std::vector<size_data> get_raw_header_line_vector()
        {
            return raw_header_lines_;
        }

        // 根据名字, 获取原始节点行, sn 是序号
        int64_t get_raw_header_line_ptr(const char *header_name, int64_t header_name_len, char **result, int64_t sn = 0);

        // 如上, 把行存到 result
        bool get_raw_header_line(const char *header_name, std::string &result, int64_t sn = 0);

        // 如上
        std::string get_raw_header_line(const char *header_name, int64_t sn = 0);

        // 如上, 只返回值, 不包括名字
        int64_t get_header_line_value(const char *header_name, std::string &result, int64_t sn = 0);

        // 如上
        std::string get_header_line_value(const char *header_name, int64_t sn = 0);

    private:
        void _section_walk(const std::string &parent_setion, int idx);

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
    // 邮件头,行最长长度限制
    static const int64_t header_line_max_length = 1024000;

    // 字符集转码
    static std::string charset_convert(const char *from_charset, const char *data, int64_t size);
    inline static std::string charset_convert(const char *from_charset, const std::string &data)
    {
        return charset_convert(from_charset, data.c_str(), data.size());
    }

    // 解码 RFC2231
    static std::string decode_2231(const char *src_charset_def, const char *in_line, int64_t in_len, bool with_charset_flag = true);
    inline static std::string decode_2231(const char *src_charset_def, const std::string &in, bool with_charset_flag = true)
    {
        return decode_2231(src_charset_def, in.c_str(), in.size(), with_charset_flag);
    }

    // 解析 Date 字段 为 unix 时间戳
    static int64_t decode_date(const char *str);
    inline static int64_t decode_date(const std::string &str)
    {
        return decode_date(str.c_str());
    }

    // 反解邮件头逻辑行
    static std::string header_line_unescape(const char *in_line, int64_t in_len);
    inline static std::string header_line_unescape(const std::string &in)
    {
        return header_line_unescape(in.c_str(), in.size());
    }

    // 获取 邮件头值对的 值,
    static int64_t header_line_get_first_token(const char *line, int64_t in_len, char **val);
    static std::string header_line_get_first_token(const char *line, int64_t in_len);
    inline static std::string header_line_get_first_token(const std::string &line)
    {
        return header_line_get_first_token(line.c_str(), line.size());
    }

    // 解析邮件头行为节点
    static std::vector<header_line_node> header_line_to_node_vector(const char *in_line, int64_t in_len, int64_t max_count = 102400);
    inline static std::vector<header_line_node> header_line_to_node_vector(const std::string &line, int64_t max_count = 102400)
    {
        return header_line_to_node_vector(line.c_str(), line.size(), max_count);
    }

    // 转码邮件头行为 UTF8
    static std::string header_line_get_utf8(const char *src_charset_def, const char *in_line, int64_t in_len = -1);
    inline static std::string header_line_get_utf8(const char *src_charset_def, const std::string &line)
    {
        return header_line_get_utf8(src_charset_def, line.c_str(), line.size());
    }

    // 解析邮件头行为 邮件地址, 如 From, To, Cc, Bcc
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

    // 获取 邮件头行的值的参数
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
    // 根据从数据流解析邮件
    static mail_parser *create_from_data(const char *mail_data, int64_t mail_data_len, const char *default_charset = nullptr);
    inline static mail_parser *create_from_data(const std::string &mail_data, const char *default_charset = nullptr)
    {
        return create_from_data(mail_data.c_str(), mail_data.size(), default_charset);
    }

    // 根据文件名解析邮件
    static mail_parser *create_from_file(const char *pathname, const char *default_charset = nullptr);
    inline static mail_parser *create_from_file(const std::string &pathname, const char *default_charset = nullptr)
    {
        return create_from_file(pathname.c_str(), default_charset);
    }

private:
    mail_parser();

public:
    ~mail_parser();

    // 邮件数据的地址
    inline const char *get_mail_data()
    {
        return mail_data_;
    }

    // 邮件大小
    inline int64_t get_mail_size()
    {
        return mail_size_;
    }

    // 邮件头地址(同: 邮件数据的地址)
    inline const char *get_header_data()
    {
        return mail_data_ + top_mime_->header_offset_;
    }

    // 邮件头偏移(实际一定是 0)
    inline int64_t get_header_offset()
    {
        return top_mime_->header_offset_;
    }

    // 邮件头大小
    inline int64_t get_header_size()
    {
        return top_mime_->header_size_;
    }

    // MIME结构的正文地址
    inline const char *get_body_data()
    {
        return mail_data_ + top_mime_->body_offset_;
    }

    // MIME结构的正文偏移
    inline int64_t get_body_offset()
    {
        return top_mime_->body_offset_;
    }

    // MIME结构的正文大小
    inline int64_t get_body_size()
    {
        return top_mime_->body_size_;
    }

    // 主题, 原始主题
    const std::string &get_subject();

    // 主题
    const std::string &get_subject_utf8();

    // Message-ID
    const std::string &get_message_id();

    // 日期
    const std::string &get_date();

    // 日期的unix时间戳
    int64_t get_date_unix();

    // 通过 Received 字段计算信件的时间戳(只是估计, 用于没有Date字段的情况)
    int64_t get_date_unix_by_received();

    // 发件人
    const mail_address &get_from();
    const mail_address &get_from_utf8();

    // Sender
    const mail_address &get_sender();

    // To
    const mail_address &get_reply_to();

    // Receipt
    const mail_address &get_receipt();

    // In-Reply-To
    const std::string &get_in_reply_to();

    // To
    const std::vector<mail_address> &get_to();
    const std::vector<mail_address> &get_to_utf8();

    // Cc
    const std::vector<mail_address> &get_cc();
    const std::vector<mail_address> &get_cc_utf8();

    // Bcc
    const std::vector<mail_address> &get_bcc();
    const std::vector<mail_address> &get_bcc_utf8();

    // 顶级 MIME 节点
    inline const mime_node *get_top_mime()
    {
        return top_mime_;
    }

    // 全部 MIME 节点
    inline const std::vector<mime_node *> &get_all_mimes()
    {
        return all_mimes_;
    }

    // 可显示的文本类节点
    const std::vector<mime_node *> &get_text_mimes();

    // 如上, 且优先显示的节点
    const std::vector<mime_node *> &get_show_mimes();

    // 附件
    const std::vector<mime_node *> &get_attachment_mimes();

    // References
    const std::vector<std::string> &get_references();

    // n == 0: first, n == -1: last
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
    int64_t date_unix_{-1};
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

// tnef/winmail.dat 类型文件的解析
// 常见于outlook发送的信件
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

// 邮件生成
/**
 * @brief 邮件构建器类，用于构建符合MIME标准的邮件
 */
class mail_builder
{
public:
    /**
     * @brief 附件结构体定义
     */
    struct attachment
    {
        inline attachment() {}
        inline ~attachment() {}
        std::string filename;      // 附件文件名
        std::string content_data;  // 附件内容数据
        std::string content_type;  // 附件内容类型
        std::string content_id;    // 附件内容ID
        bool inline_image_{false}; // 是否为内联图片
    };

    /**
     * @brief 邮件地址结构体定义
     */
    class mail_address
    {
    public:
        static mail_address getFromCmdString(const std::string &str);
        static mail_address getFromDefaultConfig(const std::string &arg, const std::string &defaultValue = "");
        static std::vector<mail_address> getVectorFromDefaultConfig(const std::string &arg);
        static std::vector<mail_address> getVectorFromString(const std::string &str);
        static std::vector<mail_address> getVectorFromPlain(const std::string &text);
        //
        static std::string mime_encode(const std::string &name_, const std::string &mail_);
        inline static std::string mime_encode(const mail_address &addr) { return mime_encode(addr.name_, addr.mail_); }
        //
        inline mail_address() {}
        inline mail_address(const std::string &name_, const std::string &mail_)
        {
            this->name_ = name_;
            this->mail_ = mail_;
        }
        inline ~mail_address() {}
        std::string mime_encode();
        std::string &getDisplayName();
        json *toJson();
        //
        std::string name_; // 名称
        std::string mail_; // 邮件地址
    };

    static std::string encode_header_line(const std::string &line);

public:
    mail_builder();
    ~mail_builder();

    // 邮件构建相关方法
    std::string build();                                                  // 构建完整的邮件内容并返回
    void set_priority(bool tf = true) { priority_ = (tf ? 1 : -1); }      // 设置邮件优先级
    void set_message_id(const std::string &message_id);                   // 设置邮件Message-ID
    void set_references(const std::string &references);                   // 设置邮件References头
    void set_date(const std::string &date);                               // 设置邮件日期
    void set_subject(const std::string &subject);                         // 设置邮件主题
    void set_receipt(const std::string &name_, const std::string &mail_); // 设置回执地址
    inline void set_receipt(const mail_address &addr) { set_receipt(addr.name_, addr.mail_); }
    void set_sender(const std::string &name_, const std::string &mail_); // 设置发件人
    inline void set_sender(const mail_address &addr) { set_sender(addr.name_, addr.mail_); }
    void set_from(const std::string &name_, const std::string &mail_); // 设置From地址
    inline void set_from(const mail_address &addr) { set_from(addr.name_, addr.mail_); }
    void add_to(const std::string &name_, const std::string &mail_); // 添加收件人
    inline void add_to(const mail_address &addr) { add_to(addr.name_, addr.mail_); }
    void add_cc(const std::string &name_, const std::string &mail_); // 添加抄送人
    inline void add_cc(const mail_address &addr) { add_cc(addr.name_, addr.mail_); }
    void add_bcc(const std::string &name_, const std::string &mail_); // 添加密送人
    inline void add_bcc(const mail_address &addr) { add_bcc(addr.name_, addr.mail_); }
    void set_in_reply_to(const std::string &in_reply_to); // 设置In-Reply-To头
    inline void set_reply_to(const mail_address &addr) { set_reply_to(addr.name_, addr.mail_); }
    void set_reply_to(const std::string &name_, const std::string &mail_); // 设置Reply-To地址
    inline void set_reply_to(const std::string &reply_to) { set_reply_to("", reply_to); }
    void set_priority(int priority);                                   // 设置邮件优先级(数值)
    void add_header(const std::string &key, const std::string &value); // 添加自定义邮件头
    void add_header(const std::string &line);                          // 添加完整的邮件头行

    // 邮件内容相关方法
    void set_html_body(const char *html, int len = -1); // 设置HTML正文内容
    inline void set_html_body(const std::string &html) { set_html_body(html.c_str(), (int)html.size()); }
    void set_plain_body(const char *html, int len = -1); // 设置纯文本正文内容
    inline void set_plain_body(const std::string &html) { set_plain_body(html.c_str(), (int)html.size()); }
    void add_attachment(const attachment &info); // 添加附件

public:
    // 邮件头字段
    std::string in_reply_to_;              // In-Reply-To头
    std::string message_id_;               // Message-ID头
    std::string references_;               // References头
    std::string date_;                     // Date头
    std::string subject_;                  // Subject头
    mail_address reply_to_;                // Reply-To地址
    mail_address receipt_;                 // 回执地址
    mail_address sender_;                  // Sender地址
    mail_address from_;                    // From地址
    std::list<mail_address> tos_;          // To收件人列表
    std::list<mail_address> ccs_;          // Cc抄送人列表
    std::list<mail_address> bccs_;         // Bcc密送人列表
    std::list<std::string> extra_headers_; // 额外自定义邮件头列表

    // 邮件内容
    std::string html_body_;               // HTML正文内容
    std::string plain_body_;              // 纯文本正文内容
    std::list<attachment> inline_images_; // 内联图片列表
    std::list<attachment> atts_;          // 附件列表
    int priority_{-1};                    // 邮件优先级

protected:
    // 构建邮件头的内部方法
    void build_header_date();                                             // 构建Date头
    void build_header_subject();                                          // 构建Subject头
    void build_header_message_id();                                       // 构建Message-ID头
    void build_header_references();                                       // 构建References头
    void build_header_in_reply_to();                                      // 构建In-Reply-To头
    void build_header_reply_to();                                         // 构建Reply-To头
    void build_header_priority();                                         // 构建Priority头
    void build_header_from();                                             // 构建From头
    void build_header_tcb(const char *key, std::list<mail_address> &tcb); // 构建To/Cc/Bcc头
    void build_header_sender();                                           // 构建Sender头
    void build_header_receipt();                                          // 构建Receipt头
    void build_header_extra();                                            // 构建额外自定义头
    void build_header_mime();                                             // 构建MIME版本头
    void build_header_mailer();                                           // 构建X-Mailer头
    void build_header();                                                  // 构建完整的邮件头

    // 构建邮件正文的内部方法
    void build_body_mixed();                  // 构建混合类型正文
    void build_body_related();                // 构建相关类型正文
    void build_body_html();                   // 构建HTML正文
    void build_body_att_one(attachment &att); // 构建单个附件
    void build_body();                        // 构建完整的邮件正文

    std::string boundaray_mixed_;   // 混合类型boundary
    std::string boundaray_related_; // 相关类型boundary

protected:
    // 数据追加方法
    mail_builder &append_data(const std::string &data);        // 追加字符串数据
    mail_builder &append_data(const char *data, int len = -1); // 追加字符数据
    mail_builder &append_data(int ch);                         // 追加单个字符

    std::string result_string_; // 构建结果字符串
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_MAIL___
