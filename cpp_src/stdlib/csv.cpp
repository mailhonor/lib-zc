/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-05-14
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

const char *csv_unserialize_one_row(const char *data_start, const char *data_end, std::vector<std::string> &fields)
{
    fields.clear();
    const char *ps;
    std::string field;
    char ch, ch_next;
    bool quoted = false;

    if (data_start >= data_end)
    {
        return data_start;
    }

    for (ps = data_start; ps < data_end; ps++)
    {
        ch = *ps;
        if (quoted)
        {
            if (ch == '"')
            {
                ch_next = 0;
                if ((ps + 1) < data_end)
                {
                    ch_next = *(ps + 1);
                }
                if (ch_next == '"')
                {
                    ps++;
                    field += ch;
                    continue;
                }
                else
                {
                    quoted = false;
                    continue;
                }
            }
            field += ch;
            continue;
        }
        else
        {
            if (ch == '"')
            {
                quoted = true;
                continue;
            }
            else if (ch == ',')
            {
                fields.push_back(field);
                field.clear();
                continue;
            }
            else if (ch == '\r')
            {
                ch_next = 0;
                if ((ps + 1) < data_end)
                {
                    ch_next = *(ps + 1);
                }
                if (ch_next == '\n')
                {
                    continue;
                }
                else
                {
                    field += ch;
                    continue;
                }
            }
            else if (ch == '\n')
            {
                fields.push_back(field);
                ps += 1;
                return ps;
            }
            else
            {
                field += ch;
                continue;
            }
        }
    }
    fields.push_back(field);
    return ps;
}

std::string csv_serialize_one_field(const std::string &field)
{
    if (field.find('"') != std::string::npos)
    {
        return "\"" + str_replace(field, "\"", "\"\"") + "\"";
    }

    if (field.find(',') != std::string::npos || field.find('\r') != std::string::npos || field.find('\n') != std::string::npos)
    {
        return "\"" + field + "\"";
    }
    return field;
}

zcc_namespace_end;