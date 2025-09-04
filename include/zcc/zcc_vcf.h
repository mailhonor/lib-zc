/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-08-13
 * ================================
 */

// rfc6350

#pragma once

#ifndef ZCC_LIB_INCLUDE_VCF__
#define ZCC_LIB_INCLUDE_VCF__

#include "./zcc_begin_end_text.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

struct vcf_contact_tel
{
    std::string number;
    bool is_pref{false};
    bool is_cell{false};
    bool is_home{false};
    bool is_work{false};
    bool is_fax{false};
    bool is_text{false};
    bool is_video{false};
};

struct vcf_contact_email
{
    std::string email;
    bool is_pref{false};
    bool is_work{false};
    bool is_home{false};
};

class vcf_contact
{
public:
    vcf_contact();
    ~vcf_contact();
    void parse(const char *text, int text_len);
    inline void parse(const std::string &text)
    {
        parse(text.c_str(), (int)text.size());
    }
    std::string debug_info();

public:
    // 版本
    std::string version_;
    // 产品ID
    std::string prodid_;
    // source
    std::string source_;
    // uid
    std::string uid_;
    // 时区
    std::string tz_;
    // 修订时间
    std::string rev_;
    // gender
    std::string gender_; // M:MALE, F:FEMALE, U:unknown or other
    // photo
    std::string photo_home_;
    // photo
    std::string photo_work_;
    // 姓名
    std::string name_;
    // 姓名
    std::string full_name_;
    // 昵称
    std::string nick_name_home_;
    // 昵称
    std::string nick_name_work_;
    // 电话
    std::vector<vcf_contact_tel> tel_;
    // 邮箱
    std::vector<vcf_contact_email> email_;
    // URL
    std::vector<std::string> url_;
    // 生日
    int64_t birthday_{var_invalid_time};
    // 附注
    std::string note_;
    // 工作地址
    std::string address_work_;
    // 住宅或个人地址
    std::string address_home_;
    // 公司
    std::string org_;
    // 职务
    std::string title_;
    // 角色
    std::string role_;
    // logo
    std::string logo_;
    // sound
    std::string sound_;
    // categrory
    std::vector<std::string> category_;

public:
    begin_end_text_node *top_node_{nullptr};

private:
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_VCF__
