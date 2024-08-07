/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include <tuple>
#include "./redis.h"

zcc_namespace_begin;

redis_client_basic_engine::redis_client_basic_engine()
{
}

redis_client_basic_engine::~redis_client_basic_engine()
{
}

static int ___query_by_io_list(redis_client_basic_engine &rc, int list_count, std::list<std::string> *list_val, stream &fp)
{
    std::string rstr;
    const char *rp;
    int firstch;
    int rlen, i, tmp, num2;
    for (i = 0; i < list_count; i++)
    {
        rstr.clear();
        if (fp.gets(rstr, 1024 * 1024) < 3)
        {
            rc.info_msg_ = "the length of the response < 3";
            return redis_fatal;
        }
        rp = rstr.c_str();
        rlen = rstr.size() - 2;
        rstr.resize(rlen);
        firstch = rp[0];
        if (firstch == '*')
        {
            tmp = atoi(rp + 1);
            if (tmp < 1)
            {
            }
            else
            {
                list_count += tmp;
            }
        }
        else if (firstch == ':')
        {
            if (rlen < 1)
            {
                if (list_val)
                {
                    list_val->push_back("");
                }
            }
            else
            {
                if (list_val)
                {
                    list_val->push_back(std::string(rp + 1, rlen - 1));
                }
            }
        }
        else if (firstch == '+')
        {
        }
        else if (firstch == '$')
        {
            num2 = std::atoi(rp + 1);
            rstr.clear();
            if (num2 > 0)
            {
                fp.readn(rstr, num2);
            }
            if (num2 > -1)
            {
                fp.readn(0, 2);
            }
            if (fp.is_exception())
            {
                rc.info_msg_ = "read/write";
                return redis_fatal;
            }
            if (list_val)
            {
                if (num2 > -1)
                {
                    list_val->push_back(rstr);
                }
                else
                {
                    list_val->push_back("");
                }
            }
        }
        else
        {
            rc.info_msg_ = "the initial of the response shold be $";
            return redis_fatal;
        }
    }
    return 1;
}

static int ___query_by_io_list_json(redis_client_basic_engine &rc, int list_count, json *json_val, stream &fp)
{
    std::string rstr;
    int idx, num, tmp;
    int ret, rlen, num2;
    const char *rp;
    int firstch;
    json *jn, *jn_tmp;
    std::list<std::tuple<int, int, json *>> stack_vec;

    json_val->used_for_array();
    stack_vec.push_back(std::make_tuple(list_count, -1, json_val));

    while (!stack_vec.empty())
    {
        auto nij = stack_vec.back();
        num = std::get<0>(stack_vec.back());
        idx = std::get<1>(stack_vec.back());
        jn = std::get<2>(stack_vec.back());
        stack_vec.pop_back();
        idx = idx + 1;
        for (; idx < num; idx++)
        {
            rstr.clear();
            if (fp.gets(rstr, 1024 * 1024) < 3)
            {
                rc.info_msg_ = "the length of the response < 3";
                ret = redis_fatal;
                goto over;
            }
            rp = rstr.c_str();
            rlen = rstr.size() - 2;
            rstr.resize(rlen);
            firstch = rp[0];
            if (firstch == '*')
            {
                tmp = atoi(rp + 1);
                if (tmp < 1)
                {
#if 0
                    zjson_array_push(jn, zjson_create_string("", 0));
#endif
                }
                else
                {
                    stack_vec.push_back(std::make_tuple(num, idx, jn));
                    stack_vec.push_back(std::make_tuple(tmp, -1, jn->array_push(new json(), true)));
                }
                break;
            }
            if (firstch == ':')
            {
		uint64_t v = ((rlen < 1) ? -1 : std::atol(rp + 1));
                jn->array_push(v);
                continue;
            }
            if (firstch == '+')
            {
                continue;
            }
            if (firstch == '$')
            {
                num2 = atoi(rp + 1);
                jn_tmp = jn->array_push(new json(), true);
                if (num2 < 0)
                {
                }
                else if (num2 < 1)
                {
                    jn_tmp->used_for_string();
                }
                else
                {
                    fp.readn(jn_tmp->get_string_value(), num2);
                }
                if (num2 > -1)
                {
                    fp.readn(0, 2);
                }
                if (fp.is_exception())
                {
                    rc.info_msg_ = "read/write";
                    ret = redis_fatal;
                    goto over;
                }
                continue;
            }
            rc.info_msg_ = "the initial of the response shold be $";
            ret = redis_fatal;
            goto over;
        }
    }
    ret = 1;
over:
    stack_vec.clear();
    return ret;
}

static int ___query_by_io_string(redis_client_basic_engine &rc, int length, std::string *string_val, stream &fp)
{
    if (length < 0)
    {
        return 0;
    }
    if (length > 0)
    {
        if (string_val)
        {
            fp.readn(*string_val, length);
        }
        else
        {
            fp.readn(0, length);
        }
    }
    fp.readn(0, 2);
    if (fp.is_exception())
    {
        rc.info_msg_ = "read/write";
        return redis_fatal;
    }
    return 1;
}

static int ___query_by_io_string_json(redis_client_basic_engine &rc, int length, json *json_val, stream &fp)
{
    if (length < 0)
    {
        return 0;
    }
    if (length > 0)
    {
        fp.readn(json_val->get_string_value(), length);
    }
    fp.readn(0, 2);
    if (fp.is_exception())
    {
        rc.info_msg_ = "read/write";
        return redis_fatal;
    }
    return 1;
}

// return:
//  3: 订阅用
static int ___query_by_io_prepare_and_write(redis_client_basic_engine &rc, std::string &rstr, int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens, stream &fp)
{
    rc.info_msg_.clear();
    rstr.clear();
    if (string_ret)
    {
        string_ret->clear();
    }
    if (list_ret)
    {
        list_ret->clear();
    }
    if (json_ret)
    {
        json_ret->reset();
    }

    if (query_tokens.empty())
    {
        if (fp.gets(rstr, 1024 * 1024) < 3)
        {
            rc.info_msg_ = "data too short";
            return redis_fatal;
        }
        return 1;
    }

    fp.printf_1024("*%zd\r\n", query_tokens.size());
    for (auto it = query_tokens.begin(); it != query_tokens.end(); it++)
    {
        fp.printf_1024("$%zd\r\n", it->size());
        fp.append(*it);
        fp.write("\r\n", 2);
    }
    fp.flush();

    if (number_ret == (int64_t *)-1)
    {
        return 3;
    }

    if (fp.gets(rstr, 1024 * 1024) < 3)
    {
        rc.info_msg_ = "data too short";
        return redis_fatal;
    }
    return 1;
}

int redis_client_basic_engine::query_protocol_by_stream(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens, stream &fp)
{
    std::string rstr;
    const char *rp;
    int rlen, num, firstch, ret;
    int64_t lret;
    if (fp.is_closed())
    {
        return redis_fatal;
    }

    if ((ret = ___query_by_io_prepare_and_write(*this, rstr, number_ret, string_ret, list_ret, json_ret, query_tokens, fp)) < 1)
    {
        goto over;
    }
    if (ret == 3)
    {
        ret = 1;
        goto over;
    }
    rp = rstr.c_str();
    rlen = rstr.size() - 2;
    rstr.resize(rlen);
    firstch = rp[0];
    if (firstch == '-')
    {
        info_msg_ = rp;
        if (json_ret)
        {
            json_ret->get_bool_value() = false;
        }
        ret = -1;
        goto over;
    }
    if (firstch == '+')
    {
        ret = 1;
        if (json_ret)
        {
            json_ret->get_bool_value() = true;
        }
        goto over;
    }
    if (firstch == '*')
    {
        num = std::atoi(rp + 1);
        if (json_ret)
        {
            ret = ___query_by_io_list_json(*this, num, json_ret, fp);
        }
        else
        {
            ret = ___query_by_io_list(*this, num, list_ret, fp);
        }
        goto over;
    }
    if (firstch == ':')
    {
        lret = std::atol(rp + 1);
        if (json_ret)
        {
            json_ret->get_long_value() = lret;
        }
        if (number_ret)
        {
            *number_ret = lret;
        }
        ret = 1;
        goto over;
    }
    if (firstch == '$')
    {
        num = std::atoi(rp + 1);
        if (json_ret)
        {
            ret = ___query_by_io_string_json(*this, num, json_ret, fp);
        }
        else
        {
            ret = ___query_by_io_string(*this, num, string_ret, fp);
        }
        goto over;
    }
    info_msg_ = "read/write, or unknown protocol";
over:
    return ret;
}

zcc_namespace_end;
