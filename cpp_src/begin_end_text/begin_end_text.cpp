/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2025-08-13
 * ================================
 */

#include "zcc/zcc_begin_end_text.h"
#include "zcc/zcc_charset.h"

zcc_namespace_begin;

class begin_end_text_parser
{
public:
    begin_end_text_parser();
    ~begin_end_text_parser();
    void parse(const char *text, int text_len);
    std::list<std::string> get_lines(const char *text, int text_len);
    begin_end_text_node *parse_one_line(const char *text, int text_len);
    void parse_one_param(begin_end_text_node *node, const std::string &line);

public:
    begin_end_text_node *root_{nullptr};
};

begin_end_text_parser::begin_end_text_parser()
{
    root_ = new begin_end_text_node();
}

begin_end_text_parser::~begin_end_text_parser()
{
    delete root_;
}

void begin_end_text_parser::parse(const char *text, int text_len)
{
    std::list<std::string> lines = get_lines(text, text_len);
    begin_end_text_node *current_begin_node = root_;
    for (auto &line : lines)
    {
        begin_end_text_node *node = parse_one_line(line.c_str(), (int)line.size());
        if (node->key_ == "begin")
        {
            node->parent_ = current_begin_node;
            current_begin_node->children_.push_back(node);
            current_begin_node = node;
            continue;
        }
        else if (node->key_ == "end")
        {
            if (current_begin_node->parent_ != nullptr)
            {
                current_begin_node = current_begin_node->parent_;
            }
            delete node;
            continue;
        }
        current_begin_node->children_.push_back(node);
    }
}

std::list<std::string> begin_end_text_parser::get_lines(const char *text, int text_len)
{
    std::list<std::string> lines;
    std::string line;
    for (int i = 0; i < text_len; i++)
    {
        if (text[i] == '\n')
        {
            if (i < text_len - 1)
            {
                if (text[i + 1] == ' ' || text[i + 1] == '\t')
                {
                    i++;
                    continue;
                }
            }
            lines.push_back(line);
            line.clear();
        }
        else if (text[i] == '\r')
        {
            continue;
        }
        else
        {
            line += text[i];
        }
    }
    if (line.size() > 0)
    {
        lines.push_back(line);
    }
    while (!lines.empty())
    {
        if (!lines.front().empty())
        {
            break;
        }
        lines.pop_front();
    }
    return lines;
}

begin_end_text_node *begin_end_text_parser::parse_one_line(const char *text, int text_len)
{
    // 解析上面类似的字符串
    begin_end_text_node *node = new begin_end_text_node();
    std::string token;
    bool key_dealed = false;
    int i = 0;
    bool quoted = false;
    for (i = 0; i < text_len; i++)
    {
        int ch = text[i];
        if (quoted)
        {
            if (ch == '"')
            {
                quoted = false;
                continue;
            }
            token += ch;
            continue;
        }
        if (ch == '"')
        {
            quoted = true;
            continue;
        }
        else if (ch == ';' || ch == ':')
        {
            if (key_dealed)
            {
                parse_one_param(node, token);
            }
            else
            {
                node->key_ = token;
                tolower(node->key_);
                key_dealed = true;
            }
            token.clear();
            if (ch == ':')
            {
                i += 1;
                break;
            }
            continue;
        }
        else
        {
            token += ch;
        }
    }
    if (!token.empty())
    {
        if (key_dealed)
        {
            parse_one_param(node, token);
        }
        else
        {
            node->key_ = token;
            tolower(node->key_);
            key_dealed = true;
        }
    }
    token.clear();
    for (; i < text_len; i++)
    {
        // 转义 \r\n\t
        int ch = text[i];
        if (ch == '\\')
        {
            i += 1;
            if (i >= text_len)
            {
                break;
            }
            ch = text[i];
            if (ch == 'r')
            {
                ch = '\r';
            }
            else if (ch == 'n')
            {
                ch = '\n';
            }
            else if (ch == 't')
            {
                ch = '\t';
            }
            token += ch;
        }
        else
        {
            token += ch;
        }
    }
    node->value_ = token;
    return node;
}

void begin_end_text_parser::parse_one_param(begin_end_text_node *node, const std::string &line)
{
    begin_end_text_node_param param;
    auto pos = line.find('=');
    if (pos == std::string::npos)
    {
        param.key = line;
    }
    else
    {
        param.key = line.substr(0, pos);
        param.value = line.substr(pos + 1);
    }
    tolower(param.key);
    node->params_.push_back(param);
}

// begin_end_text_node
begin_end_text_node::begin_end_text_node()
{
}

begin_end_text_node::~begin_end_text_node()
{
    for (auto child : children_)
    {
        delete child;
    }
}

begin_end_text_node *begin_end_text_node::parse(const char *text, int text_len)
{
    begin_end_text_parser parser;
    parser.parse(text, text_len);
    begin_end_text_node *node = parser.root_;
    parser.root_ = nullptr;
    if (node->children_.empty())
    {
        return node;
    }
    else
    {
        auto tmp_node = node->children_[0];
        node->children_[0] = nullptr;
        delete node;
        return tmp_node;
    }
}

std::string begin_end_text_node::debug_info()
{
    return inner_debug_info(0);
}

std::string begin_end_text_node::inner_debug_info(int depth)
{
    std::string info;
    info += std::string(depth, ' ') + std::string(depth, ' ') + key_ + ":\"" + value_ + "\"";
    for (auto &param : params_)
    {
        info += "; " + param.key + "=\"" + param.value + "\"";
    }
    info += "\n";
    for (auto &child : children_)
    {
        info += child->inner_debug_info(depth + 1);
    }
    return info;
}

std::string begin_end_text_node::get_decoded_value()
{
    if (value_.empty())
    {
        return value_;
    }
    auto encode = get_param_value("encoding");
    if (encode[0] == 'Q' || encode[0] == 'q')
    {
        return qp_decode_2045(value_);
    }
    else if (encode[0] == 'B' || encode[0] == 'b')
    {
        return base64_decode(value_);
    }
    return value_;
}

std::string begin_end_text_node::get_decoded_value_utf8(const std::string &default_charset)
{
    auto value = get_decoded_value();
    if (value.empty())
    {
        return value;
    }
    auto charset = get_param_value("charset", default_charset);
    return charset::convert_to_utf8(charset, value);
}
//
std::string begin_end_text_node::get_param_value(const std::string &key, const std::string &default_value)
{
    auto lowerKey = key;
    tolower(lowerKey);
    for (auto &param : params_)
    {
        if (param.key == lowerKey)
        {
            return param.value;
        }
    }
    return default_value;
}

std::vector<std::string> begin_end_text_node::get_param_values(const std::string &key)
{
    auto lowerKey = key;
    tolower(lowerKey);
    std::vector<std::string> values;
    for (auto &param : params_)
    {
        if (param.key == lowerKey)
        {
            values.push_back(param.value);
        }
    }
    return values;
}

zcc_namespace_end;
