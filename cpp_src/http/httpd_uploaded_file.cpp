/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-04-05
 * ================================
 */

#include "zcc/zcc_http.h"

zcc_namespace_begin;

int64_t httpd_uploaded_file::get_size()
{

    if (size_ != -2)
    {
        return size_;
    }

    auto &httpd = this->httpd_;
    std::string data;

    if (encoding_ == var_encoding_type_base64)
    {
        base64_decode(httpd.post_data_parser_->get_mail_data() + offset_, raw_size_, data);
        size_ = data.size();
    }
    else if (encoding_ == var_encoding_type_qp)
    {
        qp_decode_2045(httpd.post_data_parser_->get_mail_data() + offset_, raw_size_, data);
        size_ = data.size();
    }
    else
    {
        size_ = raw_size_;
    }

    return size_;
}

std::string httpd_uploaded_file::get_data()
{
    auto &httpd = this->httpd_;
    std::string data;

    if (encoding_ == var_encoding_type_base64)
    {
        base64_decode(httpd.post_data_parser_->get_mail_data() + offset_, raw_size_, data);
        size_ = data.size();
    }
    else if (encoding_ == var_encoding_type_qp)
    {
        qp_decode_2045(httpd.post_data_parser_->get_mail_data() + offset_, raw_size_, data);
        size_ = data.size();
    }
    else
    {
        data.append(httpd.post_data_parser_->get_mail_data() + offset_, raw_size_);
    }
    return data;
}

bool httpd_uploaded_file::save_to(const char *pathname)
{
    std::string data = get_data();
    return (file_put_contents(pathname, data) > 0);
}

zcc_namespace_end;