
/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-01-13
 * ================================
 */

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif // __GNUC__

#include <cstdio>
#include "./mime.h"

zcc_namespace_begin;

static void _debug_show_addr(zcc::mail_parser *parser, const char *h, const zcc::mail_parser::mail_address &addr)
{
    const char *fmt = "%15s: %s\n";
    std::string line = parser->get_raw_header_line(h);
    std::fprintf(stderr, fmt, h, line.c_str());
    std::fprintf(stderr, fmt, "", addr.mail_.c_str());
    std::fprintf(stderr, fmt, "", addr.name_.c_str());
    std::fprintf(stderr, fmt, "", addr.name_utf8_.c_str());
}

static void _debug_show_addr_vector(zcc::mail_parser *parser, const char *h, const std::vector<zcc::mail_parser::mail_address> &address_vec)
{
    const char *fmt = "%15s: %s\n";
    char nh[256];
    int i = 0;

    std::string line = parser->get_raw_header_line(h);
    std::fprintf(stderr, fmt, h, line.c_str());

    for (auto it = address_vec.begin(); it != address_vec.end(); it++)
    {
        if (i == 0)
        {
            std::sprintf(nh, "%s (1)", h);
        }
        else
        {
            std::sprintf(nh, "(%d)", i + 1);
        }
        i++;
        auto &addr = *it;
        std::fprintf(stderr, fmt, nh, addr.mail_.c_str());
        std::fprintf(stderr, fmt, "", addr.name_.c_str());
        std::fprintf(stderr, fmt, "", addr.name_utf8_.c_str());
    }
}

void mail_parser::debug_show()
{
    const char *fmt = "%15s: %s\n\n";
    int i = 0;

    std::fprintf(stderr, fmt, "Date", get_date().c_str());
    std::fprintf(stderr, "%15s: %zd\n", "", get_date_unix());
    if (1)
    {
        int64_t t = get_date_unix_by_received();
        std::fprintf(stderr, "%15s: %zd\n", "Received Date", t);
        std::string r = zcc::rfc822_time(t);
        std::fprintf(stderr, "%15s: %s\n", "Received Date", r.c_str());
    }
    std::fprintf(stderr, "\n");
    std::fprintf(stderr, fmt, "Subject", get_subject().c_str());
    std::fprintf(stderr, fmt, "Subject_utf8", get_subject_utf8().c_str());
    std::fprintf(stderr, "\n");
    _debug_show_addr(this, "From", get_from_utf8());
    std::fprintf(stderr, "\n");
    _debug_show_addr(this, "Sender", get_sender());
    std::fprintf(stderr, "\n");
    _debug_show_addr(this, "Reply-To", get_reply_to());
    std::fprintf(stderr, "\n");
    _debug_show_addr(this, "Disposition-Notification-To", get_receipt());
    std::fprintf(stderr, "\n");
    std::fprintf(stderr, fmt, "In-Reply-To", get_in_reply_to().c_str());
    std::fprintf(stderr, "\n");
    _debug_show_addr_vector(this, "To", get_to_utf8());
    std::fprintf(stderr, "\n");
    _debug_show_addr_vector(this, "Cc", get_cc_utf8());
    std::fprintf(stderr, "\n");
    _debug_show_addr_vector(this, "Bcc", get_bcc_utf8());
    std::fprintf(stderr, "\n");
    std::fprintf(stderr, fmt, "Message-Id", get_message_id().c_str());

    {
        i = 0;
        std::fprintf(stderr, "\n");
        std::string line = get_raw_header_line("References:");
        std::fprintf(stderr, fmt, "References", line.c_str());
        auto &references = get_references();
        for (auto it = references.begin(); it != references.end(); it++)
        {
            if (i == 0)
            {
                std::fprintf(stderr, fmt, "References", it->c_str());
            }
            else
            {
                std::fprintf(stderr, fmt, "", it->c_str());
            }
            i++;
        }
    }

    auto &allm = get_all_mimes();
    i = 0;
    for (auto it = allm.begin(); it != allm.end(); it++)
    {
        auto m = *it;
        std::fprintf(stderr, "\n");
        char buf[128];
        std::sprintf(buf, "Mime (%d)", i + 1);
        std::fprintf(stderr, fmt, buf, m->get_content_type().c_str());
        std::string line = m->get_raw_header_line("Content-Type");
        std::fprintf(stderr, fmt, "Content-Type", line.c_str());
        line = m->get_raw_header_line("Content-Transfer-Encoding");
        std::fprintf(stderr, fmt, "Content-Transfer-Encoding", line.c_str());
        line = m->get_raw_header_line("Content-Disposition");
        std::fprintf(stderr, fmt, "Content-Disposition", line.c_str());
        std::fprintf(stderr, fmt, "disposition", m->get_disposition().c_str());
        std::fprintf(stderr, fmt, "name", m->get_name().c_str());
        std::fprintf(stderr, fmt, "name_utf8", m->get_name_utf8().c_str());
        std::fprintf(stderr, fmt, "filename", m->get_filename().c_str());
        std::fprintf(stderr, fmt, "filename2231", m->get_filename2231().c_str());
        std::fprintf(stderr, fmt, "filename_utf8", m->get_filename_utf8().c_str());
        std::fprintf(stderr, fmt, "content_id", m->get_content_id().c_str());
        std::fprintf(stderr, fmt, "encoding", m->get_encoding().c_str());
        std::fprintf(stderr, fmt, "boundary", m->get_boundary().c_str());
        std::fprintf(stderr, fmt, "section", m->get_imap_section().c_str());
        std::sprintf(buf, "%zd,%zd,%zd,%zd", m->get_header_offset(), m->get_header_size(), m->get_body_offset(), m->get_body_size());
        std::fprintf(stderr, fmt, "", buf);
        i++;
    }

    i = 0;
    auto &textm = get_text_mimes();
    for (auto it = textm.begin(); it != textm.end(); it++)
    {
        auto m = *it;
        std::fprintf(stderr, "\n");
        std::string c = m->get_decoded_content_utf8();
        std::fprintf(stderr, fmt, m->get_content_type().c_str(), c.c_str());
    }
}

zcc_namespace_end;
