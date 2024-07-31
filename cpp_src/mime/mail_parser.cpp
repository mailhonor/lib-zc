/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

void mail_parser_decode_mime_inner(mail_parser &parser);

static std::string trim_dup(const void *ptr, int64_t n, bool token_mode = false)
{
    std::string r;
    if (n == 0)
    {
        return r;
    }
    const unsigned char *ps = (const unsigned char *)ptr, *pe = ps + n;
    for (; ps < pe; ps++)
    {
        if (token_mode)
        {
            if ((*ps == '<') || (*ps == '>'))
            {
                continue;
            }
        }
        if (((*ps) < 33) || ((*ps) == '"') || ((*ps) == '\''))
        {
            continue;
        }
        break;
    }
    pe--;
    for (; ps <= pe; pe--)
    {
        if (token_mode)
        {
            if ((*pe == '<') || (*pe == '>'))
            {
                continue;
            }
        }
        if (((*pe) < 33) || ((*pe) == '"') || ((*pe) == '\''))
        {
            continue;
        }
        break;
    }
    pe++;
    r.append((const char *)ps, pe - ps);
    return r;
}

inline static std::string trim_dup(const std::string &n, bool token_mode = false)
{
    return trim_dup(n.c_str(), n.size(), token_mode);
}

int64_t mail_parser::mime_node::get_raw_header_line_ptr(const char *header_name, int64_t header_name_len, char **result, int64_t sn)
{
    int64_t firstch, nn = 0;
    int64_t name_len;
    size_data *sdn = 0, *sd;

    if (header_name_len > 0)
    {
        name_len = header_name_len;
    }
    else
    {
        name_len = std::strlen(header_name);
        if (name_len == 0)
        {
            return -1;
        }
        if (header_name[name_len - 1] == ':')
        {
            name_len--;
        }
    }
    if (name_len < 1)
    {
        return -1;
    }

    firstch = toupper(header_name[0]);
    for (auto it = raw_header_lines_.begin(); it != raw_header_lines_.end(); it++)
    {
        sd = &(*it);
        if (sd->size < name_len)
        {
            continue;
        }
        const char *data = (const char *)(sd->data);
        if (data[name_len] != ':')
        {
            continue;
        }
        if (firstch != toupper(data[0]))
        {
            continue;
        }
        if (zcc::strncasecmp(data, header_name, name_len))
        {
            continue;
        }
        if (nn == sn)
        {
            *result = sd->data;
            return sd->size;
        }
        nn++;
        sdn = sd;
    }
    if (!sdn)
    {
        return -1;
    }
    if (sn != -1)
    {
        return -1;
    }

    *result = sdn->data;
    return sdn->size;
}

bool mail_parser::mime_node::get_raw_header_line(const char *header_name, std::string &result, int64_t sn)
{
    result.clear();
    char *vp;
    int64_t vlen = get_raw_header_line_ptr(header_name, -1, &vp, sn);
    if (vlen > 0)
    {
        result.append(vp, vlen);
    }
    return (vlen > 0);
}

std::string mail_parser::mime_node::get_raw_header_line(const char *header_name, int64_t sn)
{
    std::string r;
    get_raw_header_line(header_name, r, sn);
    return r;
}

int64_t mail_parser::mime_node::get_header_line_value(const char *header_name, std::string &result, int64_t n)
{
    result.clear();
    int64_t len = strlen(header_name);
    if (len < 1)
    {
        return -1;
    }
    if (header_name[len - 1] == ':')
    {
        len--;
    }
    if (len < 1)
    {
        return -1;
    }
    char *vp;
    int64_t vlen = get_raw_header_line_ptr(header_name, len, &vp, n);
    if (vlen < 0)
    {
        return vlen;
    }
    if (vlen > 0)
    {
        std::string r = header_line_unescape(vp + (len + 1), vlen - (len + 1));
        char *start;
        int64_t rlen = skip(r.c_str(), r.size(), " \t\r\n", 0, &start);
        if (rlen > 0)
        {
            result.append(start, rlen);
        }
    }
    return result.size();
}

std::string mail_parser::mime_node::get_header_line_value(const char *header_name, int64_t sn)
{
    std::string r;
    get_header_line_value(header_name, r, sn);
    return r;
}

const std::string &mail_parser::mime_node::get_encoding()
{
    if (encoding_flag_)
    {
        return encoding_;
    }
    encoding_flag_ = true;
    std::string tmpbf;
    get_header_line_value("Content-Transfer-Encoding:", tmpbf, 0);
    if (!tmpbf.empty())
    {
        std::string val;
        std::vector<std::tuple<std::string, std::string>> params;
        header_line_get_params(tmpbf, val, params);
        encoding_ = val;
        tolower(encoding_);
    }
    return encoding_;
}

const std::string &mail_parser::mime_node::get_content_id()
{
    if (!content_id_flag_)
    {
        content_id_flag_ = true;
        std::string tmpbf;
        get_header_line_value("Content-Id:", tmpbf, 0);
        if (!tmpbf.empty())
        {
            char *v;
            int64_t len = header_line_get_first_token(tmpbf.c_str(), tmpbf.size(), &v);
            if (len > 0)
            {
                content_id_ = trim_dup(v, len, true);
            }
        }
    }
    return content_id_;
}
const std::string &mail_parser::mime_node::get_disposition()
{
    if (disposition_flag_)
    {
        return disposition_;
    }
    disposition_flag_ = true;

    std::string tmpbf;
    get_header_line_value("Content-Disposition:", tmpbf, 0);
    if (tmpbf.empty())
    {
        return disposition_;
    }
    std::string val;
    std::vector<std::tuple<std::string, std::string>> params;
    header_line_get_params(tmpbf, val, params);
    disposition_ = val;
    tolower(disposition_);

    bool flag_2231 = false;
    for (auto it = params.begin(); it != params.end(); it++)
    {
        auto a = *it;
        const std::string &key = std::get<0>(*it);
        const std::string &val = std::get<1>(*it);
        int64_t key_len = key.size();
        int64_t value_len = val.size();
        if (key == "filename")
        {
            filename_ = val;
        }
        else if ((key_len > 8) && (!std::strncmp(key.c_str(), "filename*", 9)))
        {
            filename2231_.append(val);
            const char *value = val.c_str();
            if (!flag_2231)
            {
                flag_2231 = true;
                if (key_len == 9)
                {
                    int64_t count = 0;
                    for (int64_t i = 0; i < value_len; i++)
                    {
                        if (((unsigned char *)value)[i] == '\'')
                        {
                            count++;
                        }
                    }
                    if (count > 1)
                    {
                        filename2231_with_charset_flag_ = true;
                    }
                    else
                    {
                        filename2231_with_charset_flag_ = false;
                    }
                }
                else if (key_len == 10)
                {
                    filename2231_with_charset_flag_ = false;
                }
                else if (key_len == 11)
                {
                    filename2231_with_charset_flag_ = true;
                }
            }
        }
    }
    return disposition_;
}

const std::string &mail_parser::mime_node::get_show_name()
{
    if (!show_name_)
    {
        const std::string *n = &(get_filename_utf8());
        if (n->empty())
        {
            n = &(get_name_utf8());
        }
        show_name_ = (std::string *)n;
    }
    return *show_name_;
}

const std::string &mail_parser::mime_node::get_name_utf8()
{
    if (name_utf8_flag_)
    {
        return name_utf8_;
    }
    name_utf8_flag_ = true;
    if (!name_.empty())
    {
        std::string r = header_line_get_utf8(parser_.default_charset_.c_str(), name_);
        name_utf8_ = trim_dup(r);
    }
    return name_utf8_;
}

const std::string &mail_parser::mime_node::get_filename()
{
    if (!disposition_flag_)
    {
        get_disposition();
    }
    return filename_;
}

const std::string &mail_parser::mime_node::get_filename_utf8()
{
    if (filename_utf8_flag_)
    {
        return filename_utf8_;
    }
    filename_utf8_flag_ = true;
    if (!disposition_flag_)
    {
        get_disposition();
    }
    std::string uname;
    if (!filename2231_.empty())
    {
        uname = decode_2231(parser_.default_charset_.c_str(), filename2231_, filename2231_with_charset_flag_);
    }
    else if (!filename_.empty())
    {
        uname = header_line_get_utf8(parser_.default_charset_.c_str(), filename_);
    }
    filename_utf8_ = trim_dup(uname);
    return filename_utf8_;
}

const std::string &mail_parser::mime_node::get_filename2231(bool *with_charset_flag)
{
    if (filename2231_flag_)
    {
        if (with_charset_flag)
        {
            *with_charset_flag = filename2231_with_charset_flag_;
        }
        return filename2231_;
    }
    filename2231_flag_ = true;
    if (!disposition_flag_)
    {
        get_disposition();
    }
    if (with_charset_flag)
    {
        *with_charset_flag = filename2231_with_charset_flag_;
    }
    return filename2231_;
}

const std::string &mail_parser::mime_node::get_imap_section()
{
    if (!parser_.imap_section_flag_)
    {
        parser_.imap_section();
    }
    return imap_section_;
}

const std::string mail_parser::mime_node::get_decoded_content()
{
    const char *in_src = parser_.mail_data_ + body_offset_;
    int64_t in_len = body_size_;
    const std::string &enc = get_encoding();

    if (!enc.empty())
    {
        if (enc == "base64")
        {
            return base64_decode(in_src, in_len);
        }
        else if (enc == "quoted-printable")
        {
            return qp_decode_2045(in_src, in_len);
        }
    }
    return std::string(in_src, in_len);
}

const std::string mail_parser::mime_node::get_decoded_content_utf8()
{
    std::string content = get_decoded_content();
    const std::string &cha = get_charset();

    return charset_convert(cha.empty() ? parser_.default_charset_.c_str() : cha.c_str(), content);
}

/* ################################################################## */

mail_parser::mail_parser()
{
}

mail_parser::~mail_parser()
{
    for (auto it = all_mimes_.begin(); it != all_mimes_.end(); it++)
    {
        delete *it;
    }
}

mail_parser *mail_parser::create_from_data(const char *mail_data, int64_t mail_data_len, const char *default_charset)
{
    mail_parser *mp = new mail_parser();
    mp->mail_data_ = mail_data;
    mp->mail_size_ = mail_data_len;
    mp->default_charset_ = (default_charset ? default_charset : "");
    mail_parser_decode_mime_inner(*mp);
    return mp;
}

mail_parser *mail_parser::create_from_file(const char *pathname, const char *default_charset)
{
    mail_parser *mp = new mail_parser();
    if (mp->reader_.open(pathname) < 1)
    {
        delete mp;
        return nullptr;
    }
    mp->mmap_reader_mode_ = true;
    mp->mail_data_ = mp->reader_.data_;
    mp->mail_size_ = mp->reader_.len_;
    mp->default_charset_ = (default_charset ? default_charset : "");
    mail_parser_decode_mime_inner(*mp);
    return mp;
}

const std::string &mail_parser::get_message_id()
{
    if (!message_id_flag_)
    {
        message_id_flag_ = true;
        std::string tmpbf;
        top_mime_->get_header_line_value("Message-ID:", tmpbf, 0);
        if (!tmpbf.empty())
        {
            char *v;
            int64_t len = header_line_get_first_token(tmpbf.c_str(), tmpbf.size(), &v);
            if (len > 0)
            {
                message_id_ = trim_dup(v, len, true);
            }
        }
    }
    return message_id_;
}

const std::string &mail_parser::get_subject()
{
    if (!subject_flag_)
    {
        subject_flag_ = true;
        std::string tmpbf = top_mime_->get_header_line_value("Subject:");
        if (!tmpbf.empty())
        {
            subject_ = trim_dup(tmpbf);
        }
    }
    return subject_;
}

const std::string &mail_parser::get_subject_utf8()
{
    if (!subject_flag_)
    {
        get_subject();
    }
    if (!subject_utf8_flag_)
    {
        subject_utf8_flag_ = true;
        if (!subject_.empty())
        {
            subject_utf8_ = trim_dup(header_line_get_utf8(default_charset_.c_str(), subject_));
        }
    }
    return subject_utf8_;
}

const std::string &mail_parser::get_date()
{
    if (!date_flag_)
    {
        date_flag_ = true;
        std::string tmpbf;
        top_mime_->get_header_line_value("Date:", tmpbf, 0);
        if (!tmpbf.empty())
        {
            char *v;
            int64_t len = header_line_get_first_token(tmpbf.c_str(), tmpbf.size(), &v);
            if (len > 0)
            {
                date_ = trim_dup(v, len, true);
            }
        }
    }
    return date_;
}

int64_t mail_parser::get_date_unix()
{
    if (!date_flag_)
    {
        get_date();
    }
    if (!date_unix_flag_)
    {
        date_unix_flag_ = true;
        date_unix_ = decode_date(date_);
    }
    return date_unix_;
}

int64_t mail_parser::get_date_unix_by_received()
{
    int64_t t = -1;
    char buf[64];
    auto &raw_header_lines = top_mime_->raw_header_lines_;
    for (auto it = raw_header_lines.rbegin(); it != raw_header_lines.rend(); it++)
    {
        size_data *sd = &(*it);
        const unsigned char *data = (const unsigned char *)(sd->data);
        int64_t len = sd->size;
        if (len < 64)
        {
            continue;
        }
        if (data[0] != 'R' && data[0] != 'r')
        {
            continue;
        }
        if (data[1] != 'E' && data[1] != 'e')
        {
            continue;
        }
        if (zcc::strncasecmp((const char *)data, "received:", 9))
        {
            continue;
        }
        data += (len - 50);
        len = 50;
        memcpy(buf, data, 50);
        buf[50] = 0;
        const char *ps = buf, *end = ps + 50, *s = 0;
        while (ps < end)
        {
            ps = (char *)std::memchr(ps, ';', end - ps);
            if (!ps)
            {
                break;
            }
            s = ps + 1;
            ps = s;
        }
        if (!s)
        {
            continue;
        }
        t = decode_date(s);
        if (t < 1)
        {
            continue;
        }
        break;
    }
    if (t < 1)
    {
        return -1;
    }
    return t;
}

const mail_parser::mail_address &mail_parser::get_from()
{
#define ___mail_noutf8(a, b, c)                                                                                             \
    if (!a)                                                                                                                 \
    {                                                                                                                       \
        a = 1;                                                                                                              \
        std::string tmpbf = top_mime_->get_header_line_value(b, 0);                                                         \
        if (!tmpbf.empty())                                                                                                 \
        {                                                                                                                   \
            std::vector<mail_parser::mail_address> addrs = header_line_get_address_vector(default_charset_.c_str(), tmpbf); \
            if (!addrs.empty())                                                                                             \
            {                                                                                                               \
                c = addrs[0];                                                                                               \
            }                                                                                                               \
        }                                                                                                                   \
    }                                                                                                                       \
    return c;
    ___mail_noutf8(from_flag_, "From:", from_);
}

const mail_parser::mail_address &mail_parser::get_from_utf8()
{
    if (!from_flag_)
    {
        get_from();
    }
    if (from_flag_ != 2)
    {
        from_flag_ = 2;
        from_.name_utf8_ = trim_dup(header_line_get_utf8(default_charset_.c_str(), from_.name_));
    }
    return from_;
}

const mail_parser::mail_address &mail_parser::get_sender()
{
    ___mail_noutf8(sender_flag_, "Sender:", sender_);
}

const mail_parser::mail_address &mail_parser::get_reply_to()
{
    ___mail_noutf8(reply_to_flag_, "Reply-To:", reply_to_);
}

const mail_parser::mail_address &mail_parser::get_receipt()
{
    ___mail_noutf8(receipt_flag_, "Disposition-Notification-To:", receipt_);
}

const std::string &mail_parser::get_in_reply_to()
{
    if (in_reply_to_flag_)
    {
        return in_reply_to_;
    }
    in_reply_to_flag_ = true;
    std::string tmpbf;
    top_mime_->get_header_line_value("In-Reply-To:", tmpbf, 0);
    if (!tmpbf.empty())
    {
        char *v;
        int64_t len = header_line_get_first_token(tmpbf.c_str(), tmpbf.size(), &v);
        if (len > 0)
        {
            in_reply_to_ = trim_dup(v, len, true);
        }
    }
    return in_reply_to_;
}

const std::vector<mail_parser::mail_address> &mail_parser::get_to()
{
#define ___mail_parser_engine_tcb(tcb_flag, tcbname, tcbnamelen, tcb, utf8_tf)                             \
    std::string line;                                                                                      \
    if (!tcb_flag)                                                                                         \
    {                                                                                                      \
        tcb_flag = 1;                                                                                      \
        auto &raw_header_lines = top_mime_->raw_header_lines_;                                             \
        for (auto it = raw_header_lines.begin(); it != raw_header_lines.end(); it++)                       \
        {                                                                                                  \
            size_data *sdp = &(*it);                                                                       \
            if (sdp->size <= tcbnamelen)                                                                   \
            {                                                                                              \
                continue;                                                                                  \
            }                                                                                              \
            if (zcc::strncasecmp(sdp->data, tcbname, tcbnamelen))                                          \
            {                                                                                              \
                continue;                                                                                  \
            }                                                                                              \
            line.append(",").append(header_line_unescape(sdp->data + tcbnamelen, sdp->size - tcbnamelen)); \
        }                                                                                                  \
        tcb = header_line_get_address_vector(default_charset_.c_str(), line);                              \
    }                                                                                                      \
    if (utf8_tf && (tcb_flag != 2))                                                                        \
    {                                                                                                      \
        tcb_flag = 2;                                                                                      \
        for (auto it = tcb.begin(); it != tcb.end(); it++)                                                 \
        {                                                                                                  \
            mail_address &ma = *it;                                                                        \
            if ((!ma.name_.empty()) && (ma.name_utf8_.empty()))                                            \
            {                                                                                              \
                ma.name_utf8_ = trim_dup(header_line_get_utf8(default_charset_.c_str(), ma.name_));        \
            }                                                                                              \
        }                                                                                                  \
    }                                                                                                      \
    return tcb;
    ___mail_parser_engine_tcb(to_flag_, "To:", 3, to_, 0);
}

const std::vector<mail_parser::mail_address> &mail_parser::get_to_utf8()
{
    ___mail_parser_engine_tcb(to_flag_, "To:", 3, to_, 1);
}

const std::vector<mail_parser::mail_address> &mail_parser::get_cc()
{
    ___mail_parser_engine_tcb(cc_flag_, "Cc:", 3, cc_, 0);
}

const std::vector<mail_parser::mail_address> &mail_parser::get_cc_utf8()
{
    ___mail_parser_engine_tcb(cc_flag_, "Cc:", 3, cc_, 1);
}

const std::vector<mail_parser::mail_address> &mail_parser::get_bcc()
{
    ___mail_parser_engine_tcb(bcc_flag_, "Bcc:", 4, bcc_, 0);
}

const std::vector<mail_parser::mail_address> &mail_parser::get_bcc_utf8()
{
    ___mail_parser_engine_tcb(bcc_flag_, "Bcc:", 4, bcc_, 1);
}

const std::vector<std::string> &mail_parser::get_references()
{
    if (references_flag_)
    {
        return references_;
    }
    references_flag_ = true;
    std::string tmpbf = top_mime_->get_header_line_value("References:");
    if (!tmpbf.empty())
    {
        references_ = split(tmpbf, "<> \t,\r\n", true);
    }
    return references_;
}

const std::vector<mail_parser::mime_node *> &mail_parser::get_text_mimes()
{
    if (!classify_flag_)
    {
        classify();
    }
    return text_mimes_;
}

const std::vector<mail_parser::mime_node *> &mail_parser::get_show_mimes()
{
    if (!classify_flag_)
    {
        classify();
    }
    return show_mimes_;
}
const std::vector<mail_parser::mime_node *> &mail_parser::get_attachment_mimes()
{
    if (!classify_flag_)
    {
        classify();
    }
    return attachment_mimes_;
}

zcc_namespace_end;
