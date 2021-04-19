/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-11
 * ================================
 */

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#include "zcc_json.h"
#include <stdarg.h>
#include <string.h>

namespace zcc
{

typedef std::string ___std_string;

struct _json_walker_node {
    json *new_json;
    json *current_json;
    std::map<std::string, json *>::const_iterator map_it;
    int idx;
    int status; /* 0: begin, 1: middle, 2: end */
};

class _json_walker {
public:
    zinline _json_walker() { }
    zinline ~_json_walker() { }
    /* 本 json 库的 "序列化", "反序列化", "深度复制" 等操作,没有使用递归的方法 */
    std::vector<json *> new_json_vec;
    std::vector<json *> json_vec;
    std::vector<std::map<std::string, json *>::const_iterator> object_vec;
    std::vector<int> array_vec;
    std::vector<int> status_vec;
public:
    bool pop(_json_walker_node &node);
    void push(_json_walker_node &node);
};

bool _json_walker::pop(_json_walker_node &node)
{
    if (json_vec.empty()) {
        return false;
    }
    node.new_json = new_json_vec.back();
    node.current_json = json_vec.back();
    node.idx = array_vec.back();
    node.map_it = object_vec.back();
    node.status = status_vec.back();

    new_json_vec.pop_back();
    json_vec.pop_back();
    array_vec.pop_back();
    object_vec.pop_back();
    status_vec.pop_back();

    return true;
}

void _json_walker::push(_json_walker_node &node)
{
    new_json_vec.push_back(node.new_json);
    json_vec.push_back(node.current_json);
    array_vec.push_back(node.idx);
    object_vec.push_back(node.map_it);
    status_vec.push_back(node.status);
}

static zinline int _empty(const void *ptr) { return ((!ptr)||(!(*(const char *)(ptr)))); }

static std::string &_sprintf_1024(std::string &str, const char *format, ...)
{
    va_list ap;
    char buf[1024+1];

    va_start(ap, format);
    ::vsnprintf(buf, 1024, format, ap);
    va_end(ap);
    str.append(buf);
    return str;
}

static unsigned const char _char_xdigitval_vector[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
json::json()
{
    parent_ = 0;
    type_ = json_type_null;
}

json::json(const std::string &val)
{
    parent_ = 0;
    type_ = json_type_string;
    new (val_.s) std::string(val);
}

json::json(const char *val, int len)
{
    parent_ = 0;
    type_ = json_type_string;
    if (len < 0) {
        len = strlen(val);
    }
    if (len == 0) {
        new (val_.s) std::string();
    } else {
        new (val_.s) std::string(val, len);
    }
}

json::json(long val)
{
    parent_ = 0;
    type_ = json_type_long;
    val_.l = val;
}

json::json(double val)
{
    parent_ = 0;
    type_ = json_type_double;
    val_.d = val;
}

json::json(bool val)
{
    parent_ = 0;
    type_ = json_type_bool;
    val_.b = val;
}

json::json(const unsigned char type)
{
    parent_ = 0;
    type_ = type;
    if (type == json_type_null) {
    } else if (type == json_type_bool) {
        val_.b = false;
    } else if (type == json_type_double) {
        val_.d = 0;
    } else if (type == json_type_long) {
        val_.l = 0;
    } else if (type == json_type_string) {
        new (val_.s) std::string();
    } else if (type == json_type_array) {
        val_.v = new std::vector<json *>();
    } else if (type == json_type_object) {
        val_.m = new std::map<std::string, json *>();
    }
}

json::~json()
{
    reset();
}

json *json::reset()
{
    json *js;
    std::vector<json *> *vec_for_delete = 0;
    switch(type_) {
        case json_type_string:
            ((___std_string *)(val_.s))->~___std_string();
        case json_type_null:
        case json_type_bool:
        case json_type_double:
        case json_type_long:
            type_ = json_type_null;
            return this;
    }

    vec_for_delete = new std::vector<json *>();
    auto &for_delete = *vec_for_delete;
    for_delete.push_back(this);
    while (!for_delete.empty()) {
        js = for_delete.back();
        for_delete.pop_back();
        if (js == 0) {
            continue;
        }

        if (js->type_ == json_type_array) {
            if (js->val_.v->empty()) {
                delete js->val_.v;
                js->type_ = json_type_null;
                if (js != this) {
                    delete js;
                }
            } else {
                for_delete.push_back(js);
                for_delete.push_back(js->val_.v->back());
                js->val_.v->pop_back();
            }
        } else if (js->type_ == json_type_object) {
            if (js->val_.m->empty()) {
                delete js->val_.m;
                js->type_ = json_type_null;
                if (js != this) {
                    delete js;
                }
            } else {
                for_delete.push_back(js);
                for_delete.push_back(js->val_.m->begin()->second);
                js->val_.m->erase(js->val_.m->begin());
            }
        } else {
            if (js != this) {
                delete js;
            }
        }
    }
    js->type_ = json_type_null;
    delete vec_for_delete;

    return this;
}

std::string &json::get_string_value()
{
    if (type_ != json_type_string) {
        reset();
    }
    if (type_ == json_type_null) {
        new (val_.s) std::string();
        type_ = json_type_string;
    }
    return *((std::string *)(val_.s));
}

long &json::get_long_value()
{
    if (type_ != json_type_long) {
        reset();
    }
    if (type_ == json_type_null) {
        val_.l = 0;
        type_ = json_type_long;
    }
    return val_.l;
}

double &json::get_double_value()
{
    if (type_ != json_type_double) {
        reset();
    }
    if (type_ == json_type_null) {
        val_.d = 0;
        type_ = json_type_double;
    }
    return val_.d;
}

bool &json::get_bool_value()
{
    if (type_ != json_type_bool) {
        reset();
    }
    if (type_ == json_type_null) {
        val_.b = 0;
        type_ = json_type_bool;
    }
    return val_.b;
}

const std::vector<json *> &json::get_array_value()
{
    if (type_ != json_type_array) {
        reset();
    }
    if (type_ == json_type_null) {
        val_.v = new std::vector<json *>();
        type_ = json_type_array;
    }
    return *(val_.v);
}

const std::map<std::string, json *> &json::get_object_value()
{
    if (type_ != json_type_object) {
        reset();
    }
    if (type_ == json_type_null) {
        val_.m = new std::map<std::string, json *>();
        type_ = json_type_object;
    }
    return *(val_.m);
}

json *json::set_string_value(const char *val, int len)
{
    std::string &s = get_string_value();
    s.clear();
    if (len < 0) {
        s.append(val);
    } else if (len > 0) {
        s.append(val, len);
    }
    return this;
}

json *json::used_for_bool()
{
    get_bool_value();
    return this;
}

json *json::used_for_long()
{
    get_long_value();
    return this;
}

json *json::used_for_double()
{
    get_double_value();
    return this;
}

json *json::used_for_string()
{
    get_string_value();
    return this;
}

json *json::used_for_array()
{
    get_array_value();
    return this;
}

json *json::used_for_object()
{
    get_object_value();
    return this;
}

json *json::array_get(int idx)
{
    if (type_ != json_type_array) {
        used_for_array();
    }

    int size = int(val_.v->size());

    if (size < 1) {
        return 0;
    }

    if (idx < 0) {
        idx = size - 1;
    }
    if (size <= idx) {
        return 0;
    }
    return (*val_.v)[idx];
}

json *json::object_get(const char *key)
{
    if (type_ != json_type_object) {
        used_for_object();
    }
    auto it = val_.m->find(key);
    if (it == val_.m->end()) {
        return 0;
    }
    return it->second;
}

int json::array_size()
{
    if (type_ != json_type_array) {
        used_for_array();
    }
    return (int)(val_.v->size());
}

int json::object_size()
{
    if (type_ != json_type_object) {
        used_for_object();
    }
    return (int)(val_.m->size());
}

json *json::array_insert(int idx, json *j, bool return_child)
{
    j->parent_ = this;
    used_for_array();
    std::vector<json *> &v = *(val_.v);

    int size = int(v.size());

    if (idx < 0) {
        idx = size;
    }
    if (idx < 0) {
        idx = 0;
    }
    if (idx < size) {
        v.insert(v.begin()+idx, j);
    } else {
        v.resize(idx+1, 0);
        v[idx] = j;
    }
    return return_child?j:this;
}

json *json::array_update(int idx, json *j, json **old, bool return_child)
{
    j->parent_ = this;
    json *tmpj = 0;
    used_for_array();
    std::vector<json *> &v = *(val_.v);

    int size = int(v.size());

    if (idx < 0) {
        idx = size - 1;
    }
    if (idx < 0) {
        idx = 0;
    }
    if (idx < size) {
        tmpj = v[idx];
    } else {
        v.resize(idx+1, 0);
    }
    v[idx] = j;

    if (old) {
        if (tmpj) {
            tmpj->parent_ = 0;
        }
        *old = tmpj;
    } else if (tmpj) {
        delete tmpj;
    }
    return return_child?j:this;
}

bool json::array_delete(int idx, json **old)
{
    json *tmpj = 0;
    used_for_array();
    std::vector<json *> &v = *(val_.v);
    int size = int(v.size());

    if (size < 1) {
        return false;
    }

    if (idx < 0) {
        idx = size - 1;
    }

    if (old) {
        *old = 0;
    }

    if (idx >= size) {
        return false;
    }

    tmpj = v[idx];
    if (tmpj) {
        tmpj->parent_ = 0;
    }
    if (old) {
        *old = tmpj;
    } else if (tmpj) {
        delete tmpj;
    }
    v.erase(v.begin()+idx);

    return true;
}

json *json::object_update(const char *key, json *j, json **old, bool return_child)
{
    j->parent_ = this;
    json *tmpj = 0;
    used_for_object();
    std::map<std::string, json *> &m = *(val_.m);

    auto it = m.find(key);
    if (it == m.end()) {
        m[key] = j;
    } else {
        tmpj = it->second;
        it->second = j;
        if (tmpj) {
            tmpj->parent_ = 0;
        }
    }
    if (old) {
        *old = tmpj;
    } else if (tmpj) {
        delete tmpj;
    }
    return return_child?j:this;
}

bool json::object_delete(const char *key, json **old)
{
    json *tmpj = 0;
    used_for_object();
    std::map<std::string, json *> &m = *(val_.m);

    if (old) {
        *old = 0;
    }
    auto it = m.find(key);
    if (it == m.end()) {
        return false;
    }

    tmpj = it->second;
    if (tmpj) {
        tmpj->parent_ = 0;
    }
    if (old) {
        *old = tmpj;
    } else if (tmpj) {
        delete tmpj;
    }
    m.erase(it);
    return true;
}

static bool _get_path_idx(const char *ps, int &idx)
{
    idx = 0;
    if (!ps[0]) {
        return true;
    }
    if (ps[0] == '-') {
        ps++;
        idx = -1;
    }
    for (; *ps; ps++) {
        if (!isdigit(*ps)) {
            return false;
        }
        idx = idx * 10 + (*ps - '0');
    }
    return true;
}

json *json::get_by_path(const char *path)
{
    json *j = this;
    if (_empty(path)) {
        return j;
    }
    std::string tmpkey;
    const char *ps = path, *p;
    int idx;
    while (j) {
        tmpkey.clear();
        p = strchr(ps, '/');
        if (p) {
            if (ps < p) {
                tmpkey.append(ps, p - ps);
            }
        } else {
            tmpkey.append(ps);
        }
        if (j->type_== json_type_array) {
            if (!_get_path_idx(tmpkey.c_str(), idx)) {
                j = 0;
                break;
            }
            j = j->array_get(idx);
        } else if (j->type_== json_type_object) {
            j = j->object_get(tmpkey.c_str());
        } else {
            j = 0;
        }
        tmpkey.clear();
        if (!p) {
            break;
        }
        ps = p + 1;
    }

    return j;
}

json *json::get_by_path_vec(const char *path0, ...)
{
    if (_empty(path0)) {
        return 0;
    }
    json *j = this;
    int first = 1, idx;
    const char *ps;
    va_list ap;
    va_start(ap, path0);
    while(j) {
        if (first) {
            first = 0;
            ps = path0;
        } else {
            ps = va_arg(ap, const char *);
        }
        if (ps == 0) {
            break;
        }
        if (j->type_== json_type_array) {
            if (!_get_path_idx(ps, idx)) {
                j = 0;
                break;
            }
            j = j->array_get(idx);
        } else if (j->type_== json_type_object) {
            j = j->object_get(ps);
        } else {
            j = 0;
        }
    }
    va_end(ap);
    return j;
}

bool json::get_value_by_path(const char *name, std::string &value)
{
    json *j = get_by_path(name);
    if (!j) {
        return false;
    }
    if (j->type_ == json_type_long) {
        _sprintf_1024(value, "%l", j->val_.l);
    } else if (j->type_ == json_type_string) {
        value.append(j->val_.s);
    } else if (j->type_ == json_type_double) {
        _sprintf_1024(value, "%f", j->val_.d);
    } else if (j->type_ == json_type_bool) {
        value.append(j->val_.b?"1":"0");
    } else if (j->type_ == json_type_null) {
        value.append("0");
    } else {
        return false;
    }
    return true;
}

bool json::get_value_by_path(const char *name, long *value)
{
    json *j = get_by_path(name);
    if (!j) {
        return false;
    }
    if (j->type_ == json_type_long) {
        *value = j->val_.l;
    } else if (j->type_ == json_type_string) {
        *value = atol(j->get_string_value().c_str());
    } else if (j->type_ == json_type_double) {
        *value = (long)j->val_.l;
    } else if (j->type_ == json_type_bool) {
        *value = j->val_.b?1:0;
    } else if (j->type_ == json_type_null) {
        *value = 0;
    } else {
        return false;
    }
    return true;
}

json *json::get_top()
{
    json *j = this;
    while (j->parent_) {
        j = j->parent_;
    }
    return j;
}

bool json::load_from_pathname(const char *pathname)
{
    std::string con;
    int ch;
    FILE *fp = fopen(pathname, "r");;
    if (!fp) {
        return false;
    }
    while ((ch = fgetc(fp)) != EOF) {
        con.push_back(ch);
    }
    if (ferror(fp)) {
        fclose(fp);
        return false;
    }
    fclose(fp);
    if (!unserialize(con.c_str(), (int)con.size())) {
        return false;
    }
    return true;
}

static zinline const char *___ignore_blank(const char *ps, const char *str_end)
{
    while(ps < str_end){
        if ((*ps != ' ') && (*ps != '\t') && (*ps != '\r') && (*ps != '\n')) {
            break;
        }
        ps++;
    }
    return ps;
}

static zinline int ___ncr_decode(int ins, char *wchar)
{
    if (ins < 128) {
        *wchar = ins;
        return 1;
    }
    if (ins < 2048) {
        *wchar++ = (ins >> 6) + 192;
        *wchar++ = (ins & 63) + 128;
        return 2;
    }
    if (ins < 65536) {
        *wchar++ = (ins >> 12) + 224;
        *wchar++ = ((ins >> 6) & 63) + 128;
        *wchar++ = (ins & 63) + 128;
        return 3;
    }
    if (ins < 2097152) {
        *wchar++ = (ins >> 18) + 240;
        *wchar++ = ((ins >> 12) & 63) + 128;
        *wchar++ = ((ins >> 6) & 63) + 128;
        *wchar++ = (ins & 63) + 128;
        return 4;
    }

    return 0;
}

static const char *___fetch_string(const char *ps, const char *str_end, std::string &str)
{
    char begin = *ps ++, ch, ch2, ch3;
    while (ps < str_end) {
        ch = *ps ++;
        if (ch =='"') {
            if (begin == '"') {
                return ps; /* true */
            }
            str.push_back(ch);
            continue;
        }
        if (ch =='\'') {
            if (begin == '\'') {
                return ps; /* true */
            }
            str.push_back(ch);
            continue;
        }
        if (ps == str_end) {
            return ps; /* false */
        }
        if (ch == '\\') {
            ch2 = *ps ++;
            ch3 = 0;
            if (ch2 == 'u') {
                ch3 = 'u';
                if (ps + 4 > str_end) {
                    return ps; /* false */
                }
                int uval = 0;
                for (int count = 4; count ;count --) {
                    int ch4 = _char_xdigitval_vector[(unsigned char)(*ps++)];
                    if (ch4 == -1) {
                        return ps; /* false */
                    }
                    uval = (uval << 4) + ch4;
                }
                char buf[8];
                int len = ___ncr_decode(uval, buf);
                str.append(buf, len);
                continue;
            } else {
                switch (ch2) {
                case '\\':
                    ch3 = '\\';
                    break;
                case '/':
                    ch3 = '/';
                    break;
                case '"':
                    ch3 = '"';
                    break;
                case '\'':
                    ch3 = '\'';
                    break;
                case 'b':
                    ch3 = '\b';
                    break;
                case 'f':
                    ch3 = '\f';
                    break;
                case 'r':
                    ch3 = '\r';
                    break;
                case 'n':
                    ch3 = '\n';
                    break;
                case 't':
                    ch3 = '\t';
                    break;
                }
            }
            if (ch3) {
                str.push_back(ch3);
            } else {
                str.push_back('\\');
                str.push_back(ch2);
            }
            continue;
        } else {
            str.push_back(ch);
        }
    }
    return ps; /* false */
}

static const char *___fetch_string2(const char *ps, const char *str_end, std::string &str)
{
    char ch;
    while (ps < str_end) {
        ch = *ps;
        if ((ch == '\r') || (ch=='\n') || (ch==' ') || (ch == '\t') || (ch == ':')) {
            break;
        }
        str.push_back(ch);
        ps++;
    }
    if (ps == str_end) {
        return ps; /* false */
    }
    return ps; /* false */
}


bool json::unserialize(const char *jstr, int jlen)
{
    if (jlen < 0) {
        jlen = strlen(jstr);
    }

    reset();

    std::string tmpkey;
    const char *ps = jstr, *str_end = ps + jlen;
    std::vector<json *> json_vec;
    json_vec.push_back(this);
    json *current_json, *new_json, *old_json;
    bool ret = false;
    while(ps < str_end) {
        ps = ___ignore_blank(ps, str_end);
        if (ps == str_end) {
            break;
        }
        if (json_vec.empty()) {
            break;
        }
        current_json = json_vec.back();
        json_vec.pop_back();
        if (current_json->type_ == json_type_object) {
            int comma_count = 0;
            while(ps < str_end) {
                ps = ___ignore_blank(ps, str_end);
                if (ps == str_end) {
                    goto err;
                }
                if (*ps != ',') {
                    break;
                }
                comma_count ++;
                ps ++;
            }
            if (ps == str_end) {
                goto err;
            }
            if (*ps == '}') {
                ps ++;
                continue;
            }
            if ((current_json->object_size() > 0) && (comma_count == 0)) {
                goto err;
            }
            tmpkey.clear();
            if ((*ps == '"') || (*ps == '\'')) {
                ps = ___fetch_string(ps, str_end, tmpkey);
            } else {
                ps = ___fetch_string2(ps, str_end, tmpkey);
            }
            new_json = new json();
            current_json->object_update(tmpkey.c_str(), new_json, &old_json);
            if (old_json) {
                delete old_json;
                goto err;
            }
            json_vec.push_back(current_json);
            json_vec.push_back(new_json);
            ps = ___ignore_blank(ps, str_end);
            if (ps == str_end) {
                goto err;
            }
            if (*ps != ':') {
                goto err;
            }
            ps++;
            continue;
        }
        if (current_json->type_ == json_type_array) {
            int comma_count = 0;
            while(ps < str_end) {
                ps = ___ignore_blank(ps, str_end);
                if (ps == str_end) {
                    goto err;
                }
                if (*ps != ',') {
                    break;
                }
                comma_count++;
                ps ++;
            }
            if (ps == str_end) {
                goto err;
            }
            if (*ps == ']') {
                for (int i = 0; i < comma_count; i++) {
                    current_json->array_push(new json());
                }
                ps ++;
                continue;
            }
            for (int i = 0; i < comma_count -1; i++) {
                current_json->array_push(new json());
            }
            if ((current_json->array_size() > 0) && (comma_count < 1)){
                goto err;
            }
            json_vec.push_back(current_json);
            json_vec.push_back(current_json->array_push(new json(), true));
            continue;
        }
        if (*ps == '{') {
            json_vec.push_back(current_json);
            current_json->used_for_object();
            ps++;
            continue;
        }
        if (*ps == '[') {
            json_vec.push_back(current_json);
            current_json->used_for_array();
            ps++;
            continue;
        }
        if ((*ps == '"') || (*ps == '\'')) {
            tmpkey.clear();
            ps = ___fetch_string(ps, str_end, tmpkey);
            if((current_json->type_ != json_type_string) && (current_json->type_ != json_type_null)) {
                goto err;
            }
            current_json->get_string_value() = tmpkey;
            tmpkey.clear();
            continue;
        }
        if ((*ps == '-') || ((*ps >= '0') && (*ps <= '9'))) {
            tmpkey.clear();
            bool is_double = false;
            while(ps < str_end) {
                int ch = *ps;
                if ((ch!='-') && (ch!='+') && (ch!='.') && (ch!= 'E') &&(ch!='e') &&(!isdigit(ch))){
                    break;
                }
                if (ch == '.' || ch=='e' || ch == 'E'){
                    is_double = true;
                }
                tmpkey.push_back(ch);
                ps++;
            }
            if (is_double) {
                current_json->get_double_value() = atof(tmpkey.c_str());
            } else {
                current_json->get_long_value() = atol(tmpkey.c_str());
            }
            continue;
        }
        tmpkey.clear();
        while(ps < str_end) {
            int ch = *ps;
            if (!isalpha(ch)){
                break;
            }
            tmpkey.push_back(tolower(ch));
            ps++;
            if (tmpkey.size() > 10) {
                goto err;
            }
        }

        if ((!strcmp(tmpkey.c_str(), "null")) || (!strcmp(tmpkey.c_str(), "undefined"))) {
            current_json->reset();
            continue;
        }
        if (!strcmp(tmpkey.c_str(), "true")) {
            current_json->get_bool_value() = true;
            continue;
        }
        if (!strcmp(tmpkey.c_str(), "false")) {
            current_json->get_bool_value() = false;
            continue;
        }
        goto err;
    }
    ret = true;
err:
    if (ret == false) {
        reset();
    }
    return ret;
}

static void ___serialize_string(std::string &result, const char *data, int size)
{
    const char *json_hex_chars = "0123456789abcdef";
    result.push_back('"');
    unsigned char *ps = (unsigned char *)data;
    for (int i = 0; i < size; i++) {
        int ch = ps[i];
        switch(ch) {
            case '\\':
                result.push_back('\\');
                result.push_back('\\');
                break;
            case '/':
                result.push_back('\\');
                result.push_back('/');
                break;
            case '\"':
                result.push_back('\\');
                result.push_back('"');
                break;
            case '\b':
                result.push_back('\\');
                result.push_back('b');
                break;
            case '\f':
                result.push_back('\\');
                result.push_back('f');
                break;
            case '\n':
                result.push_back('\\');
                result.push_back('n');
                break;
            case '\r':
                result.push_back('\\');
                result.push_back('r');
                break;
            case '\t':
                result.push_back('\\');
                result.push_back('t');
                break;
            default:
                if (ch < ' ') {
                    result.append("\\u00");
                    result.push_back(json_hex_chars[ch>>4]);
                    result.push_back(json_hex_chars[ch&0X0F]);
                } else {
                    result.push_back(ch);
                }
                break;
        }
    }
    result.push_back('"');
}

static zinline void ___serialize_string2(std::string &result, const std::string &str)
{
    ___serialize_string(result, str.c_str(), (int)str.size());
}

json *json::serialize(std::string &result, bool strict_flag)
{
    _json_walker walker;
    _json_walker_node wnode;

    wnode.current_json = this;
    wnode.status = 0;
    walker.push(wnode);

    while(walker.pop(wnode)) {
        json *current_json = wnode.current_json;
        unsigned char type = (current_json?current_json->type_:json_type_null);
        if (type == json_type_null) {
            result.append("null");
        } else if (type == json_type_string) {
            ___serialize_string2(result, current_json->get_string_value());
        } else if (type == json_type_bool) {
            result.append((current_json->val_.b)?"true":"false");
        } else if (type == json_type_long) {
            _sprintf_1024(result, "%ld", current_json->get_long_value());
        } else if (type == json_type_double) {
            double d = current_json->get_double_value();
            long l = (long)d;
            if ((l > 1000L * 1000 * 1000 * 1000) || (l < -1000L * 1000 * 1000 * 1000)){
                _sprintf_1024(result, "%e", d);
            } else {
                _sprintf_1024(result, "%lf", d);
            }
        } else if (type == json_type_array) {
            int length = (int)current_json->array_size();
            if (length == 0) {
                result.append("[]");
                continue;
            }
            if (wnode.status == 0) {
                result.push_back('[');
                wnode.idx = 0;
                wnode.status = 1;
                walker.push(wnode);
                continue;
            }
            if (wnode.status == 2) {
                result.push_back(']');
                continue;
            }
            if ((wnode.idx > 0) && (wnode.idx < length)) {
                result.push_back(',');
            }
            wnode.idx++;
            if (wnode.idx == length) {
                wnode.status = 2;
            }
            walker.push(wnode);
            
            wnode.current_json = current_json->array_get(wnode.idx - 1);
            wnode.status = 0;
            walker.push(wnode);
        } else if (type == json_type_object) {
            auto &m = current_json->get_object_value();
            if (current_json->object_size() == 0) {
                result.append("{}");
                continue;
            }

            if (wnode.status == 0) {
                result.push_back('{');
                wnode.status = 1;
                wnode.map_it = m.begin();
                walker.push(wnode);
                continue;
            }
            if (wnode.status == 2) {
                result.push_back('}');
                continue;
            }
            if (wnode.map_it != m.begin()) {
                result.push_back(',');
            }
            ___serialize_string2(result, wnode.map_it->first);
            result.push_back(':');

            wnode.map_it ++;
            if (wnode.map_it == m.end()) {
                wnode.status = 2;
            }
            walker.push(wnode);
            
            wnode.map_it --;
            wnode.current_json = wnode.map_it->second;
            wnode.status = 0;
            walker.push(wnode);

            continue;
        }
    }
    return this;
}

static json *_deep_copy_simple(json *js, bool &is_simple)
{
    json  *njs = new json();
    int type = (js?js->get_type():json_type_null);
    if ((type == json_type_object) || (type == json_type_array)) {
        is_simple = false;
        return njs;
    }
    
    if (type == json_type_string) {
        njs->set_string_value(js->get_string_value());
    } else if (type == json_type_bool) {
        njs->set_bool_value(js->get_bool_value());
    } else if (type == json_type_long) {
        njs->set_long_value(js->get_long_value());
    } else if (type == json_type_double) {
        njs->set_double_value(js->get_double_value());
    }

    is_simple = true;
    return njs;
}

static json *_deep_copy_complex(json *r, json *current_json)
{
    bool is_simple;
    json *js, *new_json, *njs;
    _json_walker walker;
    _json_walker_node wnode;

    wnode.new_json = r;
    wnode.current_json = current_json;
    wnode.status = 0;
    walker.push(wnode);

    while(walker.pop(wnode)) {
        new_json = wnode.new_json;
        current_json = wnode.current_json;
        unsigned char type = current_json->get_type();
        if (type == json_type_array) {
            int length = current_json->array_size();
            if (length == 0) {
                new_json->used_for_array();
                continue;
            }

            if (wnode.status == 0) {
                wnode.status = 1;
                wnode.idx = 0;
                walker.push(wnode);
                continue;
            }

            if (wnode.status == 2) {
                continue;
            }

            js = current_json->array_get(wnode.idx);
            njs = _deep_copy_simple(js, is_simple);
            new_json->array_push(njs);
            wnode.idx ++;
            if (wnode.idx == length) {
                wnode.status = 2;
            }
            walker.push(wnode);

            if (is_simple) {
                continue;
            }

            wnode.new_json = njs;
            wnode.current_json = js;
            wnode.status = 0;
            walker.push(wnode);
            continue;
        } else /* if (type == json_type_object) */ {
            auto &m = current_json->get_object_value();
            if (current_json->object_size() == 0) {
                new_json->used_for_object();
                continue;
            }

            if (wnode.status == 0) {
                wnode.status = 1;
                wnode.map_it = m.begin();
                walker.push(wnode);
                continue;
            }
            if (wnode.status == 2) {
                continue;
            }
            js = wnode.map_it->second;
            njs = _deep_copy_simple(js, is_simple);
            new_json->object_update(wnode.map_it->first.c_str(), njs);

            wnode.map_it ++;
            if (wnode.map_it == m.end()) {
                wnode.status = 2;
            }
            walker.push(wnode);

            if (is_simple) {
                continue;
            }

            wnode.new_json = njs;
            wnode.current_json = js;
            wnode.status = 0;
            walker.push(wnode);

            continue;
        }
    }

    return r;
}

json *json::deep_copy()
{
    bool is_simple;
    json *r = _deep_copy_simple(this, is_simple);
    if (is_simple) {
        return r;
    }
    return _deep_copy_complex(r, this);
}

json *json::debug_show()
{
    std::string s;
    serialize(s);
    printf("JSON: %s\n", s.c_str());
    return this;
}

}

