/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-05-30
 * ================================
 */

#include "./mime.h"
#include "zcc/zcc_json.h"

zcc_namespace_begin;

std::string mail_builder::encode_header_line(const std::string &line)
{
    std::string r;
    r = "=?UTF-8?B?";
    base64_encode(line, r);
    r.append("?=");
    return r;
}
mail_builder::mail_address mail_builder::mail_address::getFromCmdString(const std::string &str)
{
    // 分解 "xxx@a.com:xxx" 或 "xxx@a.com"
    size_t pos = str.find(':');
    if (pos != std::string::npos)
    {
        return mail_builder::mail_address(str.substr(pos + 1), str.substr(0, pos));
    }
    else
    {
        return mail_builder::mail_address("", str);
    }
}

mail_builder::mail_address mail_builder::mail_address::getFromDefaultConfig(const std::string &arg, const std::string &defaultValue)
{
    auto v = var_main_config.get_string(arg);
    if (v.empty())
    {
        return getFromCmdString(defaultValue);
    }
    return getFromCmdString(v);
}

std::vector<mail_builder::mail_address> mail_builder::mail_address::getVectorFromDefaultConfig(const std::string &arg)
{
    std::vector<mail_builder::mail_address> r;
    auto v = var_main_config.get_string(arg);
    if (v.empty())
    {
        return r;
    }
    auto ss = split_and_ignore_empty_token(v, ',');
    for (auto &s : ss)
    {
        r.push_back(getFromCmdString(s));
    }
    return r;
}

std::vector<mail_builder::mail_address> mail_builder::mail_address::getVectorFromString(const std::string &str)
{
    std::vector<mail_builder::mail_address> r;
    auto ss = split_and_ignore_empty_token(str, ',');
    for (auto &s : ss)
    {
        r.push_back(getFromCmdString(s));
    }
    return r;
}

std::vector<mail_builder::mail_address> mail_builder::mail_address::getVectorFromPlain(const std::string &text)
{
    std::vector<mail_builder::mail_address> list;
    const char *ps = text.c_str();
    const char *p1, *p2;

    while (1)
    {
        mail_builder::mail_address ma;
        p1 = std::strchr(ps, '\n');
        if (!p1)
        {
            break;
        }
        p2 = std::strchr(p1 + 1, '\n');
        if (!p2)
        {
            ma.mail_ = std::string(ps, p1 - ps);
            ma.name_ = std::string(p1 + 1);
            list.emplace_back(ma);
            break;
        }
        ma.mail_ = std::string(ps, p1 - ps);
        ma.name_ = std::string(p1 + 1, p2 - p1 - 1);
        list.emplace_back(ma);
        ps = p2 + 1;
    }
    return list;
}

std::string &mail_builder::mail_address::getDisplayName()
{
    if (!name_.empty())
    {
        return name_;
    }
    else
    {
        return mail_;
    }
}

json *mail_builder::mail_address::toJson()
{
    json *j = new json();
    j->object_update("name_", name_);
    j->object_update("mail_", mail_);
    return j;
}

std::string mail_builder::mail_address::mime_encode(const std::string &name_, const std::string &mail_)
{
    std::string r;
    if (!name_.empty())
    {
        r.append(mail_builder::encode_header_line(name_)).append(" ");
    }
    r.append("<").append(mail_).append(">");
    return r;
}

std::string mail_builder::mail_address::mime_encode()
{
    std::string r;
    if (!name_.empty())
    {
        r.append(mail_builder::encode_header_line(name_)).append(" ");
    }
    r.append("<").append(mail_).append(">");
    return r;
}

mail_builder::mail_builder()
{
}

mail_builder::~mail_builder()
{
}

void mail_builder::add_header(const std::string &key, const std::string &value)
{
    std::string line = key;
    line.append(": ").append(value);
    add_header(line);
}

void mail_builder::add_header(const std::string &line)
{
    extra_headers_.push_back(line);
}

void mail_builder::set_message_id(const std::string &message_id)
{
    message_id_ = message_id;
}

void mail_builder::set_references(const std::string &references)
{
    references_ = references;
}

void mail_builder::set_in_reply_to(const std::string &in_reply_to)
{
    in_reply_to_ = in_reply_to;
}

void mail_builder::set_reply_to(const std::string &name_, const std::string &mail_)
{
    reply_to_.name_ = name_;
    reply_to_.mail_ = mail_;
}

void mail_builder::set_priority(int priority)
{
    priority_ = priority;
}

void mail_builder::set_date(const std::string &date)
{
    date_ = date;
}

void mail_builder::set_subject(const std::string &subject)
{
    subject_ = subject;
}

void mail_builder::set_receipt(const std::string &name_, const std::string &mail_)
{
    receipt_.name_ = name_;
    receipt_.mail_ = mail_;
}

void mail_builder::set_sender(const std::string &name_, const std::string &mail_)
{
    sender_.name_ = name_;
    sender_.mail_ = mail_;
}

void mail_builder::set_from(const std::string &name_, const std::string &mail_)
{
    from_.name_ = name_;
    from_.mail_ = mail_;
}

void mail_builder::add_to(const std::string &name_, const std::string &mail_)
{
    if (name_.empty() && mail_.empty())
    {
        return;
    }
    mail_address ma;
    ma.name_ = name_;
    ma.mail_ = mail_;
    tos_.push_back(ma);
}

void mail_builder::add_cc(const std::string &name_, const std::string &mail_)
{
    if (name_.empty() && mail_.empty())
    {
        return;
    }
    mail_address ma;
    ma.name_ = name_;
    ma.mail_ = mail_;
    ccs_.push_back(ma);
}

void mail_builder::add_bcc(const std::string &name_, const std::string &mail_)
{
    if (name_.empty() && mail_.empty())
    {
        return;
    }
    mail_address ma;
    ma.name_ = name_;
    ma.mail_ = mail_;
    bccs_.push_back(ma);
}

void mail_builder::set_html_body(const char *html, int len)
{
    if (len < 0)
    {
        html_body_ = html;
    }
    else
    {
        html_body_.clear();
        html_body_.append(html, len);
    }
}

void mail_builder::set_plain_body(const char *plain, int len)
{
    if (len < 0)
    {
        plain_body_ = plain;
    }
    else
    {
        plain_body_.clear();
        plain_body_.append(plain, len);
    }
}

void mail_builder::add_attachment(const attachment &info)
{
    if (info.inline_image_)
    {
        inline_images_.push_back(info);
    }
    else
    {
        atts_.push_back(info);
    }
}

void mail_builder::build_header_tcb(const char *key, std::list<mail_address> &tcb)
{
    if (tcb.empty())
    {
        return;
    }
    append_data(key).append_data(":");
    bool first = true;
    for (auto it = tcb.begin(); it != tcb.end(); it++)
    {
        if (first)
        {
            first = false;
            append_data(" ");
        }
        else
        {
            append_data("\t");
        }
        append_data(it->mime_encode());
        auto it_next = it;
        it_next++;
        if (it_next != tcb.end())
        {
            append_data(",");
        }
        append_data("\r\n");
    }
}

void mail_builder::build_header_date()
{
    if (date_.empty())
    {
        date_ = rfc822_time();
    }
    append_data("Date: ").append_data(date_).append_data("\r\n");
}

void mail_builder::build_header_from()
{
    if (from_.name_.empty() && from_.mail_.empty())
    {
        append_data("From: \r\n");
        return;
    }
    append_data("From: ").append_data(from_.mime_encode()).append_data("\r\n");
}

void mail_builder::build_header_sender()
{
    if (sender_.mail_.empty())
    {
        return;
    }
    append_data("Sender: ").append_data(sender_.mime_encode()).append_data("\r\n");
}

void mail_builder::build_header_receipt()
{
    if (receipt_.mail_.empty())
    {
        return;
    }
    append_data("Disposition-Notification-To: ").append_data(receipt_.mime_encode()).append_data("\r\n");
}

void mail_builder::build_header_subject()
{
    if (subject_.empty())
    {
        subject_ = " ";
    }
    append_data("Subject: ").append_data(encode_header_line(subject_)).append_data("\r\n");
}

void mail_builder::build_header_message_id()
{
    char tmpbuf[1024 + 1];
    if (message_id_.empty())
    {
        const char *domain = strrchr(from_.mail_.c_str(), '@');
        if (domain)
        {
            domain++;
        }
        else
        {
            domain = "zcc";
        }
        timeofday tv = gettimeofday();
        std::sprintf(tmpbuf, "%zd_%zd_%d_zcc@%s", tv.tv_sec, tv.tv_usec, gettid(), domain);
        message_id_ = tmpbuf;
    }
    append_data("Message-ID: <").append_data(message_id_).append_data(">\r\n");
}

void mail_builder::build_header_references()
{
    if (references_.empty())
    {
        return;
    }
    std::string r;
    auto rs = split_and_ignore_empty_token(references_, "<> \t,\r\n");
    bool first = true;
    for (auto &p : rs)
    {
        if (!p.empty())
        {
            if (first)
            {
                first = false;
            }
            else
            {
                r.append("\t");
            }
            r.append(" <").append(p).append(">,\r\n");
        }
    }
    if (r.empty())
    {
        return;
    }
    r.resize(r.size() - 3);
    append_data("References:").append_data(r).append_data("\r\n");
    return;
}

void mail_builder::build_header_in_reply_to()
{
    if (in_reply_to_.empty())
    {
        return;
    }
    append_data("In-Reply-To: <").append_data(in_reply_to_).append_data(">\r\n");
}

void mail_builder::build_header_reply_to()
{
    if (reply_to_.mail_.empty())
    {
        return;
    }
    append_data("Reply-To: ").append_data(reply_to_.mime_encode()).append_data("\r\n");
}

void mail_builder::build_header_priority()
{
    if (priority_ > 0 || priority_ < 6)
    {
        append_data("X-Priority: ").append_data(std::to_string(priority_)).append_data("\r\n");
    }
}

void mail_builder::build_header_extra()
{
    for (auto it = extra_headers_.begin(); it != extra_headers_.end(); it++)
    {
        append_data(*it).append_data("\r\n");
    }
}

void mail_builder::build_header_mime()
{
    append_data("MIME-Version: 1.0\r\n");

    if (atts_.empty() && inline_images_.empty())
    {
        if ((!html_body_.empty()) || (plain_body_.empty()))
        {
            append_data("Content-Type: text/html; charset=\"UTF-8\"\r\n");
        }
        else
        {
            append_data("Content-Type: text/plain; charset=\"UTF-8\"\r\n");
        }
        append_data("Content-Transfer-Encoding: base64\r\n");
        return;
    }

    char tmpbuf[128 + 1];
    timeofday tv = gettimeofday();
    std::sprintf(tmpbuf, "%zd-%zd-%d-zcc", tv.tv_sec, tv.tv_usec, gettid());
    boundaray_mixed_ = "1-";
    boundaray_mixed_.append(tmpbuf);
    boundaray_related_ = "2-";
    boundaray_related_.append(tmpbuf);

    if (!atts_.empty())
    {
        append_data("Content-Type: multipart/mixed;\r\n");
        append_data("\tboundary=\"").append_data(boundaray_mixed_).append_data("\"\r\n");
    }
    else
    {
        append_data("Content-Type: multipart/related;\r\n");
        append_data("\tboundary=\"").append_data(boundaray_related_).append_data("\"\r\n");
    }
}

void mail_builder::build_header_mailer()
{
    append_data("X-Mailer: ZCC mail_-builder 1.0\r\n");
}

void mail_builder::build_header()
{
    build_header_date();
    build_header_from();
    build_header_tcb("To", tos_);
    build_header_tcb("Cc", ccs_);
    build_header_tcb("Bcc", bccs_);
    build_header_message_id();
    build_header_references();
    build_header_subject();
    build_header_mime();
    build_header_in_reply_to();
    build_header_reply_to();
    build_header_priority();
    build_header_sender();
    build_header_receipt();
    build_header_extra();
    build_header_mailer();
    append_data("\r\n");
}

void mail_builder::build_body_html()
{
    if ((!atts_.empty()) || (!inline_images_.empty()))
    {
        if ((!html_body_.empty()) || (plain_body_.empty()))
        {
            append_data("Content-Type: text/html; charset=\"UTF-8\"\r\n");
        }
        else
        {
            append_data("Content-Type: text/plain; charset=\"UTF-8\"\r\n");
        }
        append_data("Content-Transfer-Encoding: base64\r\n");
        append_data("\r\n");
    }

    std::string r;
    if ((!html_body_.empty()) || (plain_body_.empty()))
    {
        base64_encode(html_body_, r, true);
    }
    else
    {
        base64_encode(plain_body_, r, true);
    }
    trim_right(r);
    r.append("\r\n");

    append_data(r);
}

void mail_builder::build_body_att_one(attachment &info)
{
    const char *filename = info.filename.c_str();
    // content_type
    append_data("Content-Type: ");
    if (info.content_type.empty())
    {
        append_data(get_mime_type_from_pathname(filename, var_default_mime_type));
    }
    else
    {
        append_data(info.content_type);
    }
    append_data(";\r\n");
    append_data("\tname=\"").append_data(encode_header_line(info.filename)).append_data("\"\r\n");
    //
    append_data("Content-Transfer-Encoding: base64\r\n");
    if (info.inline_image_)
    {
        append_data("Content-Disposition: inline;\r\n");
    }
    else
    {
        append_data("Content-Disposition: attachment;\r\n");
    }
    append_data("\tfilename=\"").append_data(encode_header_line(info.filename)).append_data("\"\r\n");
    //
    if (!info.content_id.empty())
    {
        append_data("Content-ID: <").append_data(info.content_id).append_data(">\r\n");
    }
    //
    append_data("\r\n");

    // data
    std::string r;
    base64_encode(info.content_data, r, true);
    trim_right(r);
    r.append("\r\n");
    append_data(r);
}

void mail_builder::build_body_related()
{
    if (!atts_.empty())
    {
        append_data("Content-Type: multipart/related;\r\n");
        append_data("\tboundary=\"").append_data(boundaray_related_).append_data("\"\r\n");
        append_data("\r\n");
    }
    append_data("--").append_data(boundaray_related_).append_data("\r\n");
    build_body_html();
    for (auto it = inline_images_.begin(); it != inline_images_.end(); it++)
    {
        append_data("--").append_data(boundaray_related_).append_data("\r\n");
        build_body_att_one(*it);
    }
    append_data("--").append_data(boundaray_related_).append_data("--\r\n");
}

void mail_builder::build_body_mixed()
{
    append_data("--").append_data(boundaray_mixed_).append_data("\r\n");
    if (!inline_images_.empty())
    {
        build_body_related();
    }
    else
    {
        build_body_html();
    }

    for (auto it = atts_.begin(); it != atts_.end(); it++)
    {
        append_data("--").append_data(boundaray_mixed_).append_data("\r\n");
        build_body_att_one(*it);
    }
    append_data("--").append_data(boundaray_mixed_).append_data("--\r\n");
}

void mail_builder::build_body()
{
    if (!atts_.empty())
    {
        append_data("This is a multi-part message in MIME format.\r\n\r\n");
        build_body_mixed();
    }
    else if (!inline_images_.empty())
    {
        append_data("This is a multi-part message in MIME format.\r\n\r\n");
        build_body_related();
    }
    else
    {
        build_body_html();
    }
}

std::string mail_builder::build()
{
    build_header();
    build_body();
    return result_string_;
}

mail_builder &mail_builder::append_data(const std::string &data)
{
    result_string_.append(data);
    return *this;
}

mail_builder &mail_builder::append_data(const char *data, int len)
{
    if (len < 0)
    {
        len = strlen(data);
    }
    result_string_.append(data, len);
    return *this;
}

mail_builder &mail_builder::append_data(int ch)
{
    result_string_.push_back(ch);
    return *this;
}

zcc_namespace_end;
