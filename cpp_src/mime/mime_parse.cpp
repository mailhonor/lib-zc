/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

struct mail_parser_running_boundary_line
{
    int64_t offset{0};
    int64_t len{0};
    int64_t part_offset{0};
};

struct mail_parser_running_node
{
    mail_parser::mime_node *cmime{nullptr};
    int64_t bls_idx{0};
    int64_t bls_len{0};
    const char *mail_data_{0};
    const char *mail_pos{0};
    int64_t mail_size{0};
};

class mail_parser_running_context
{
public:
    mail_parser_running_context(mail_parser &parser);
    ~mail_parser_running_context();
    void run();
    void decode_mime_get_all_boundary();
    void decode_mime_prepare_node();
    int64_t read_header_logic_line(mail_parser_running_node *cnode, char **ptr);
    void deal_content_type(mail_parser::mime_node *cmime, const std::string &line);
    bool read_and_deal_header_line();

public:
    mail_parser &parser_;
    mail_parser_running_node *cnode_{nullptr};
    std::vector<mail_parser_running_node *> node_vec_;
    const char *part_mail_data_{nullptr};
    int64_t part_mail_size_{0};
    mail_parser_running_boundary_line *bls_ptr_{nullptr};
    int64_t bls_idx_{0};
    int64_t bls_len_{0};
};

mail_parser_running_context::mail_parser_running_context(mail_parser &parser) : parser_(parser)
{
    part_mail_data_ = (char *)(void *)parser.mail_data_;
    part_mail_size_ = parser.mail_size_;
}

mail_parser_running_context::~mail_parser_running_context()
{
    for (auto it = node_vec_.begin(); it != node_vec_.end(); it++)
    {
        delete *it;
    }
    if (bls_ptr_)
    {
        delete[] (char *)bls_ptr_;
    }
}

int64_t mail_parser_running_context::read_header_logic_line(mail_parser_running_node *cnode, char **ptr)
{
    const char *pbegin = cnode->mail_pos;
    const char *pend = cnode->mail_data_ + cnode->mail_size;
    const char *ps, *p;
    int64_t len = 0;

    *ptr = (char *)pbegin;
    if (pbegin >= pend)
    {
        return 0;
    }
    if (pbegin[0] == '\n')
    {
        cnode->mail_pos += 1;
        return 0;
    }
    if (pend > pbegin)
    {
        if ((pbegin[0] == '\r') && (pbegin[1] == '\n'))
        {
            cnode->mail_pos += 2;
            return 0;
        }
    }

    ps = pbegin;
    while (pend > ps)
    {
        p = (char *)std::memchr(ps, '\n', pend - ps);
        if ((!p) || (p + 1 == pend))
        {
            /* not found or to end */
            len = pend - pbegin;
            break;
        }
        if ((p[1] == ' ') || (p[1] == '\t'))
        {
            ps = p + 1;
            continue;
        }
        len = p - pbegin + 1;
        break;
    }

    cnode->mail_pos += len;
    return len;
}

void mail_parser_running_context::deal_content_type(mail_parser::mime_node *cmime, const std::string &line)
{
    if (!cmime->content_type_.empty())
    {
        return;
    }
    cmime->content_type_flag_ = true;
    std::string val;
    std::vector<std::tuple<std::string, std::string>> params;
    mail_parser::header_line_get_params(line, cmime->content_type_, params);
    tolower(cmime->content_type_);
    for (auto it = params.begin(); it != params.end(); it++)
    {
        std::string &key = std::get<0>(*it);
        std::string &val = std::get<1>(*it);
        if (key == "boundary")
        {
            cmime->boundary_ = val;
        }
        else if (key == "name")
        {
            cmime->name_ = val;
        }
        else if (key == "charset")
        {
            cmime->charset_ = val;
        }
    }
}

bool mail_parser_running_context::read_and_deal_header_line()
{
    mail_parser_running_node *cnode = cnode_;
    mail_parser::mime_node *cmime = cnode->cmime, *pmime = cmime->parent_;
    char *line;
    int64_t llen, safe_llen;
    safe_llen = llen = read_header_logic_line(cnode, &line);
    if (llen == 0)
    {
        cmime->header_size_ = cnode->mail_pos - cnode->mail_data_;
        cmime->body_offset_ = cnode->mail_pos - parser_.mail_data_;

        if ((cmime->content_type_[0] != 'm') || (std::strncmp(cmime->content_type_.c_str(), "multipart/", 10)))
        {
            cmime->is_multipart_ = false;
        }
        else
        {
            cmime->is_multipart_ = true;
        }
        if (pmime)
        {
            if (pmime->child_head_ == 0)
            {
                pmime->child_head_ = cmime;
                pmime->child_tail_ = cmime;
            }
            else
            {
                pmime->child_tail_->next_ = cmime;
                cmime->prev_ = pmime->child_tail_;
                pmime->child_tail_ = cmime;
            }
        }
        return false;
    }
    if (safe_llen > mail_parser::header_line_max_length)
    {
        safe_llen = mail_parser::header_line_max_length - 2;
    }
    if (1)
    {
        size_data sd;
        sd.size = llen;
        sd.data = line;
        cmime->raw_header_lines_.push_back(sd);
    }

    if (!(cmime->content_type_flag_))
    {
        if ((llen > 12) && (line[7] == '-') && (line[12] == ':') && (!strncasecmp(line, "Content-Type:", 13)))
        {
            std::string hline = mail_parser::header_line_unescape(line + 13, safe_llen - 13);
            deal_content_type(cmime, hline);
        }
    }
    return true;
}

void mail_parser_running_context::decode_mime_prepare_node()
{
    if (parser_.top_mime_)
    {
        if (part_mail_size_ < 1)
        {
            return;
        }

        const char *data = part_mail_data_;
        int64_t i, ch, len = part_mail_size_;
        for (i = 0; i < len; i++)
        {
            ch = data[i];
            if ((ch == '\r') || (ch == '\n') || (ch == ' ') || (ch == '\t'))
            {
                continue;
            }
            break;
        }
        if (i == len)
        {
            return;
        }
    }
    else
    {
        cnode_ = 0;
    }
    mail_parser::mime_node *nmime = new mail_parser::mime_node(parser_);
    if (cnode_ && cnode_->cmime)
    {
        nmime->parent_ = cnode_->cmime;
    }
    parser_.all_mimes_.push_back(nmime);
    mail_parser_running_node *nnode = new mail_parser_running_node();
    nnode->cmime = nmime;
    nnode->bls_idx = bls_idx_;
    nnode->bls_len = bls_len_;
    nnode->mail_data_ = part_mail_data_;
    nnode->mail_pos = nnode->mail_data_;
    nnode->mail_size = part_mail_size_;
    node_vec_.push_back(nnode);
}

void mail_parser_running_context::decode_mime_get_all_boundary()
{
    bls_idx_ = 0;
    bls_len_ = 0;
    int64_t len;
    std::vector<mail_parser_running_boundary_line> bls_ptr;
    const char *ps = parser_.mail_data_, *pend = parser_.mail_data_ + parser_.mail_size_, *p;

    if (parser_.mail_size_ > 0)
    {
        if (ps[0] == '\n')
        {
            return;
        }
    }
    if (parser_.mail_size_ > 1)
    {
        if ((ps[0] == '\r') && (ps[1] == '\n'))
        {
            return;
        }
    }

    while (ps < pend)
    {
        len = pend - ps;
        if (len < 3)
        {
            break;
        }
        if (ps[1] != '-')
        {
            p = (char *)std::memchr(ps + 1, '\n', len - 1);
            if (p)
            {
                ps = p + 1;
            }
            else
            {
                ps += len;
            }
            continue;
        }
        if (ps[0] != '-')
        {
            p = (char *)std::memchr(ps, '\n', len);
            if (p)
            {
                ps = p + 1;
            }
            else
            {
                ps += len;
            }
            continue;
        }

        mail_parser_running_boundary_line bl;
        p = (char *)std::memchr(ps, '\n', len);
        if (p)
        {
            len = p - ps + 1;
        }
        bl.offset = (ps - parser_.mail_data_);
        bl.len = len;
        bl.part_offset = (ps + len - parser_.mail_data_);
        bls_ptr.push_back(bl);
        ps += len;
    }
    bls_len_ = bls_ptr.size();
    bls_ptr_ = (mail_parser_running_boundary_line *)new char[(bls_len_ + 1) * sizeof(mail_parser_running_boundary_line)];
    for (int64_t i = 0; i < bls_len_; i++)
    {
        bls_ptr_[i] = bls_ptr[i];
    }
}

void mail_parser_running_context::run()
{
    decode_mime_get_all_boundary();
    decode_mime_prepare_node();
    parser_.top_mime_ = node_vec_[0]->cmime;

    mail_parser_running_node *cnode = 0, *cnode_last = 0;
    while (!node_vec_.empty())
    {
        if (cnode_last)
        {
            delete (cnode_last);
        }
        cnode_last = cnode = cnode_ = node_vec_.back();
        node_vec_.pop_back();
        mail_parser::mime_node *cmime = cnode->cmime;

        /* header */
        cmime->header_offset_ = cnode->mail_data_ - parser_.mail_data_;
        while (read_and_deal_header_line())
            ;
        if (!cmime->content_type_flag_)
        {
            cmime->content_type_ = "text/plain";
            cmime->content_type_flag_ = true;
        }
        /* body */
        cmime->body_size_ = cnode->mail_data_ + cnode->mail_size - parser_.mail_data_ - cmime->body_offset_;
        if (!cmime->is_multipart_)
        {
            continue;
        }
        if (cnode->bls_len == 0)
        {
            bls_idx_ = -1;
            bls_len_ = 0;
            part_mail_data_ = parser_.mail_data_ + cmime->body_offset_;
            part_mail_size_ = cmime->body_size_;
            decode_mime_prepare_node();
            continue;
        }

        mail_parser_running_boundary_line *bls, *bls1, *bls2;
        bls1 = 0;
        bls2 = 0;
        int64_t count = 0;
        for (int64_t bls_idx = 0; bls_idx < cnode->bls_len; bls_idx++)
        {
            bls = bls_ptr_ + (cnode->bls_idx + bls_idx);
            int64_t len = bls->len - 2;
            const char *boundary = parser_.mail_data_ + bls->offset + 2;

            if (((int64_t)(cmime->boundary_.size()) > len) || (std::strncmp(cmime->boundary_.c_str(), boundary, cmime->boundary_.size())))
            {
                continue;
            }
            count++;
            bls1 = bls2;
            bls2 = bls;
            if (bls1 && bls2)
            {
                int64_t i1 = (bls1 - bls_ptr_) + 1;
                int64_t i2 = (bls2 - bls_ptr_);

                if (i1 <= i2)
                {
                    bls_idx_ = i1;
                    bls_len_ = i2 - i1 + 1;
                }
                else
                {
                    bls_idx_ = -1;
                    bls_len_ = 0;
                }
                part_mail_data_ = parser_.mail_data_ + bls1->part_offset;
                part_mail_size_ = bls2->offset - bls1->part_offset;
                decode_mime_prepare_node();
            }
        }
        if (count == 0)
        {
            continue;
        }
        int64_t len = bls2->len - 2;
        const char *boundary = parser_.mail_data_ + bls2->offset + 2;
        if (len - 2 >= (int64_t)cmime->boundary_.size())
        {
            if ((boundary[len - 2] == '-') && (boundary[len - 1] == '-'))
            {
                continue;
            }
        }

#if 1
        bls_idx_ = (bls2 - bls_ptr_) + 1;
        bls_len_ = cnode->bls_idx + cnode->bls_len - bls_idx_;

        if (bls_len_ < 0)
        {
            bls_idx_ = -1;
            bls_len_ = 0;
        }
#else
        bls_idx_ = -1;
        bls_len_ = 0;
#endif

        part_mail_data_ = parser_.mail_data_ + bls2->part_offset;
        part_mail_size_ = cnode->mail_data_ + cnode->mail_size - (parser_.mail_data_ + bls2->part_offset);
        decode_mime_prepare_node();
    }
    if (cnode_last)
    {
        delete cnode_last;
    }

    for (auto it = parser_.all_mimes_.begin(); it != parser_.all_mimes_.end(); it++)
    {
        auto cmime = *it;
        int64_t idx = cmime->body_offset_ + cmime->body_size_;
        if ((cmime->body_size_ > 0) && (parser_.mail_data_[idx - 1] == '\n'))
        {
            idx--;
            cmime->body_size_--;
        }
        if ((cmime->body_size_ > 0) && (parser_.mail_data_[idx - 1] == '\r'))
        {
            cmime->body_size_--;
        }
    }
}

void mail_parser_decode_mime_inner(mail_parser &parser)
{
    mail_parser_running_context ctx(parser);
    ctx.run();
}

zcc_namespace_end;
