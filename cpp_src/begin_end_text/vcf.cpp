/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2025-08-14
 * ================================
 */

#include <set>
#include "zcc/zcc_vcf.h"
#include "zcc/zcc_charset.h"

zcc_namespace_begin;

vcf_contact::vcf_contact()
{
}

vcf_contact::~vcf_contact()
{
    delete top_node_;
}

static void simple_check_is_home_work_pref(std::vector<begin_end_text_node_param> &params, bool &is_home, bool &is_work, bool &pref)
{
    is_home = false;
    is_work = false;
    pref = false;
    for (auto &param : params)
    {
        if (param.key == "type")
        {
            auto v = param.value;
            tolower(v);
            if (v == "home")
            {
                is_home = true;
            }
            else if (v == "work")
            {
                is_work = true;
            }
        }
        else if (param.key == "home")
        {
            is_home = true;
        }
        else if (param.key == "work")
        {
            is_work = true;
        }
        else if (param.key == "pref")
        {
            pref = true;
        }
    }
}

static vcf_contact_tel simple_parse_tel(begin_end_text_node &node)
{
    vcf_contact_tel tel;
    std::set<std::string> types;
    for (auto &param : node.params_)
    {
        if (param.key == "type")
        {
            auto type = param.value;
            tolower(type);
            for (auto &type : split(type, ","))
            {
                types.insert(type);
            }
        }
        else
        {
            types.insert(param.key);
        }
    }
    for (auto &type : types)
    {
        if (type == "pref")
        {
            tel.is_pref = true;
        }
        else if (type == "cell")
        {
            tel.is_cell = true;
        }
        else if (type == "home")
        {
            tel.is_home = true;
        }
        else if (type == "work")
        {
            tel.is_work = true;
        }
        else if (type == "fax")
        {
            tel.is_fax = true;
        }
        else if (type == "text")
        {
            tel.is_text = true;
        }
        else if (type == "video")
        {
            tel.is_video = true;
        }
    }
    tel.number = node.value_;
    return tel;
}

static vcf_contact_email simple_parse_email(begin_end_text_node &node)
{
    vcf_contact_email email;
    std::set<std::string> types;
    for (auto &param : node.params_)
    {
        if (param.key == "type")
        {
            auto type = param.value;
            tolower(type);
            for (auto &type : split(type, ","))
            {
                types.insert(type);
            }
        }
        else
        {
            types.insert(param.key);
        }
    }
    for (auto &type : types)
    {
        if (type == "pref")
        {
            email.is_pref = true;
        }
        else if (type == "home")
        {
            email.is_home = true;
        }
        else if (type == "work")
        {
            email.is_work = true;
        }
    }
    email.email = node.value_;
    return email;
}

void vcf_contact::parse(const char *text, int text_len)
{
    auto charset = charset::detect_cjk(text, text_len);
    top_node_ = begin_end_text_node::parse(text, text_len);
    std::string type;
    bool is_home, is_work, pref;

    for (auto &node : top_node_->children_)
    {
        auto &key = node->key_;
        if (key == "version")
        {
            version_ = node->value_;
        }
        else if (key == "prodid")
        {
            prodid_ = node->value_;
        }
        else if (key == "source")
        {
            source_ = node->value_;
        }
        else if (key == "uid")
        {
            uid_ = node->value_;
        }
        else if (key == "tz")
        {
            tz_ = node->value_;
        }
        else if (key == "rev")
        {
            rev_ = node->value_;
        }
        else if (key == "gender")
        {
            gender_ = node->value_;
            if (!gender_.empty())
            {
                gender_.resize(1);
                toupper(gender_);
            }
        }
        else if (key == "n")
        {
            name_ = node->get_decoded_value_utf8();
        }
        else if (key == "fn")
        {
            full_name_ = node->get_decoded_value_utf8();
        }
        else if (key == "nickname")
        {
            simple_check_is_home_work_pref(node->params_, is_home, is_work, pref);
            if (is_home || !is_work)
            {
                nick_name_home_ = node->get_decoded_value_utf8();
            }
            if (is_work)
            {
                nick_name_work_ = node->get_decoded_value_utf8();
            }
        }
        else if (key == "photo")
        {
            simple_check_is_home_work_pref(node->params_, is_home, is_work, pref);
            if (is_home || !is_work)
            {
                photo_home_ = node->value_;
            }
            if (is_work)
            {
                photo_work_ = node->value_;
            }
        }
        else if (key == "tel")
        {
            auto tel = simple_parse_tel(*node);
            if (!tel.number.empty())
            {
                tel_.push_back(tel);
            }
        }
        else if (key == "email")
        {
            auto email = simple_parse_email(*node);
            email.email = node->value_;
            if (!email.email.empty())
            {
                email_.push_back(email);
            }
        }
        else if (key == "url")
        {
            url_.push_back(node->get_decoded_value());
        }
        else if (key == "bday")
        {
            birthday_ = iso8601_2004_time_from_date(node->value_, true);
        }
        else if (key == "org")
        {
            org_ = node->get_decoded_value_utf8();
        }
        else if (key == "title")
        {
            title_ = node->get_decoded_value_utf8();
        }
        else if (key == "adr")
        {
            simple_check_is_home_work_pref(node->params_, is_home, is_work, pref);
            if (is_home || !is_work)
            {
                address_home_ = node->get_decoded_value_utf8();
            }
            if (is_work)
            {
                address_work_ = node->get_decoded_value_utf8();
            }
        }
        else if (key == "note")
        {
            note_ = node->get_decoded_value_utf8();
        }

        else if (key == "role")
        {
            role_ = node->get_decoded_value_utf8();
        }
        else if (key == "title")
        {
            title_ = node->get_decoded_value_utf8();
        }
        else if (key == "category")
        {
            category_ = split_and_ignore_empty_token(node->get_decoded_value_utf8(), ",");
        }
        else if (key == "logo")
        {
            logo_ = node->get_decoded_value();
        }
        else if (key == "sound")
        {
            sound_ = node->get_decoded_value();
        }
    }
}

std::string vcf_contact::debug_info()
{
    std::string info;
    info += "version: " + version_ + "\n";
    info += "prodid: " + prodid_ + "\n";
    info += "source: " + source_ + "\n";
    info += "uid: " + uid_ + "\n";
    info += "tz: " + tz_ + "\n";
    info += "rev: " + rev_ + "\n";
    info += "name: " + name_ + "\n";
    info += "full_name: " + full_name_ + "\n";
    info += "nick_name_home: " + nick_name_home_ + "\n";
    info += "nick_name_work: " + nick_name_work_ + "\n";
    if (tel_.size() < 1)
    {
        info += "tel: none\n";
    }
    else
    {
        for (auto &tel : tel_)
        {
            info += "tel: " + tel.number;
            if (tel.is_pref)
            {
                info += ", pref";
            }
            if (tel.is_cell)
            {
                info += ", cell";
            }
            if (tel.is_home)
            {
                info += ", home";
            }
            if (tel.is_work)
            {
                info += ", work";
            }
            if (tel.is_video)
            {
                info += ", video";
            }
            if (tel.is_text)
            {
                info += ", text";
            }
            info += "\n";
        }
    }
    if (email_.size() < 1)
    {
        info += "email: none\n";
    }
    else
    {
        for (auto &email : email_)
        {
            info += "email: " + email.email;
            if (email.is_pref)
            {
                info += ", pref";
            }
            if (email.is_home)
            {
                info += ", home";
            }
            if (email.is_work)
            {
                info += ", work";
            }
            info += "\n";
        }
    }
    if (url_.size() < 1)
    {
        info += "url: none\n";
    }
    else
    {
        for (auto &url : url_)
        {
            info += "url: " + url + "\n";
        }
    }
    info += "birthday:" + zcc::simple_date_time_with_second(birthday_) + "\n";
    info += "address_home: " + address_home_ + "\n";
    info += "address_work: " + address_work_ + "\n";
    info += "org: " + org_ + "\n";
    info += "title: " + title_ + "\n";
    info += "role: " + role_ + "\n";
    info += "category: " + join(category_, ", ") + "\n";
    info += "logo: " + logo_ + "\n";
    info += "sound: " + sound_ + "\n";
    info += "note: " + note_ + "\n";

    return info;
}

zcc_namespace_end;
