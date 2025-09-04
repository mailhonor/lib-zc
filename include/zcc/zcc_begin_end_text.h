/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-08-13
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_BEGINE_END_TEXT__
#define ZCC_LIB_INCLUDE_BEGINE_END_TEXT__

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

struct begin_end_text_node_param
{
    inline begin_end_text_node_param() {}
    inline ~begin_end_text_node_param() {}
    std::string key;
    std::string value;
};

class begin_end_text_node
{
public:
    static begin_end_text_node *parse(const char *text, int text_len);
    inline static begin_end_text_node *parse(const std::string &text)
    {
        return parse(text.c_str(), (int)text.size());
    }

public:
    begin_end_text_node();
    ~begin_end_text_node();
    std::string debug_info();
    std::string get_decoded_value();
    std::string get_decoded_value_utf8(const std::string &default_charset = var_blank_string);
    std::string get_param_value(const std::string &key, const std::string &default_value = var_blank_string);
    std::vector<std::string> get_param_values(const std::string &key);

private:
    std::string inner_debug_info(int depth);

public:
    std::string key_;
    std::string value_;
    std::vector<begin_end_text_node_param> params_;
    std::vector<begin_end_text_node *> children_;
    begin_end_text_node *parent_{nullptr};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_BEGINE_END_TEXT__
