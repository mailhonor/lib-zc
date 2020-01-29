/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-08-11
 * ================================
 */

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#include "zc.h"
#include <ctype.h>

zbool_t zjson_load_from_pathname(zjson_t *j, const char *pathname)
{
    zbool_t r = 0;
    zbuf_t *content = zbuf_create(-1);
    if (zfile_get_contents(pathname, content) < 0) {
        r = zjson_unserialize(j, zbuf_data(content), zbuf_len(content));
    }
    zbuf_free(content);
    return r;
}

static inline __attribute__((always_inline)) char *___ignore_blank(char *ps, char *str_end)
{
    while(ps < str_end){
        if ((*ps != ' ') && (*ps != '\t') && (*ps != '\r') && (*ps != '\n')) {
            break;
        }
        ps++;
    }
    return ps;
}

static inline __attribute__((always_inline)) size_t ___ncr_decode(size_t ins, char *wchar)
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

static char * ___fetch_string(char *ps, char *str_end, zbuf_t *str)
{
    char begin = *ps ++, ch, ch2, ch3;
    while (ps < str_end) {
        ch = *ps ++;
        if (ch =='"') {
            if (begin == '"') {
                return ps; /* true */
            }
            zbuf_put(str, ch);
            continue;
        }
        if (ch =='\'') {
            if (begin == '\'') {
                return ps; /* true */
            }
            zbuf_put(str, ch);
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
                    int ch4 = zchar_xdigitval_vector[(unsigned char)(*ps++)];
                    if (ch4 == -1) {
                        return ps; /* false */
                    }
                    uval = (uval << 4) + ch4;
                }
                char buf[8];
                int len = ___ncr_decode((size_t)uval, buf);
                zbuf_memcat(str, buf, len);
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
                case '0':
                    ch3 = '\0';
                    break;
                }
            }
            if (ch3) {
                zbuf_put(str, ch3);
            } else {
                zbuf_put(str, '\\');
                zbuf_put(str, ch2);
            }
            continue;
        } else {
            zbuf_put(str, ch);
        }
    }
    return ps; /* false */
}

static char *___fetch_string2(char *ps, char *str_end, zbuf_t *str)
{
    char ch;
    while (ps < str_end) {
        ch = *ps;
        if ((ch == '\r') || (ch=='\n') || (ch==' ') || (ch == '\t') || (ch == ':')) {
            break;
        }
        zbuf_put(str, ch);
        ps++;
    }
    if (ps == str_end) {
        return ps; /* false */
    }
    return ps; /* false */
}


zbool_t zjson_unserialize(zjson_t *j, const char *jstr, int jlen)
{
    if (jlen < 0) {
        jlen = strlen(jstr);
    }
    zjson_reset(j);
    zbuf_t *tmpkey = zbuf_create(128);
    char *ps = ZCONVERT_CHAR_PTR(jstr), *str_end = ps + jlen;
    zvector_t *json_vec = zvector_create(32);
    zvector_push(json_vec, j);
    zjson_t *current_json, *new_json, *old_json;
    zbool_t ret = 0;
    while(ps < str_end) {
        ps = ___ignore_blank(ps, str_end);
        if (ps == str_end) {
            break;
        }
        if (!zvector_pop(json_vec, (void **)&current_json)) {
            break;
        }
        if (current_json->type == zvar_json_type_object) {
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
            if ((zjson_object_get_len(current_json)>0) && (comma_count == 0)) {
                goto err;
            }
            zbuf_reset(tmpkey);
            if ((*ps == '"') || (*ps == '\'')) {
                ps = ___fetch_string(ps, str_end, tmpkey);
            } else {
                ps = ___fetch_string2(ps, str_end, tmpkey);
            }
            new_json = zjson_create();
            zjson_object_update(current_json, zbuf_data(tmpkey), new_json, &old_json);
            if (old_json) {
                zjson_free(old_json);
                goto err;
            }
            zvector_push(json_vec, current_json);
            zvector_push(json_vec, new_json);
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
        if (current_json->type == zvar_json_type_array) {
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
                    new_json = zjson_create();
                    zjson_array_push(current_json, new_json);
                }
                ps ++;
                continue;
            }
            for (int i = 0; i < comma_count -1; i++) {
                new_json = zjson_create();
                zjson_array_push(current_json, new_json);
            }
            if ((zjson_array_get_len(current_json) >0) && (comma_count < 1)){
                goto err;
            }
            new_json = zjson_create();
            zjson_array_push(current_json, new_json);
            zvector_push(json_vec, current_json);
            zvector_push(json_vec, new_json);
            continue;
        }
        if (*ps == '{') {
            zvector_push(json_vec, current_json);
            zjson_get_object_value(current_json);
            ps++;
            continue;
        }
        if (*ps == '[') {
            zvector_push(json_vec, current_json);
            zjson_get_array_value(current_json);
            ps++;
            continue;
        }
        if ((*ps == '"') || (*ps == '\'')) {
            zbuf_reset(tmpkey);
            ps = ___fetch_string(ps, str_end, tmpkey);
            if((current_json->type != zvar_json_type_string) && (current_json->type != zvar_json_type_null)) {
                goto err;
            }
            zbuf_memcpy(*zjson_get_string_value(current_json), zbuf_data(tmpkey), zbuf_len(tmpkey));
            zbuf_reset(tmpkey);
            continue;
        }
        if ((*ps == '-') || ((*ps >= '0') && (*ps <= '9'))) {
            zbuf_reset(tmpkey);
            zbool_t is_double = 0;
            while(ps < str_end) {
                int ch = *ps;
                if ((ch!='-') && (ch!='+') && (ch!='.') && (ch!= 'E') &&(ch!='e') &&(!isdigit(ch))){
                    break;
                }
                if (ch == '.' || ch=='e' || ch == 'E'){
                    is_double = 1;
                }
                zbuf_put(tmpkey, ch);
                ps++;
            }
            if (is_double) {
                *zjson_get_double_value(current_json) = atof(zbuf_data(tmpkey));
            } else {
                *zjson_get_long_value(current_json) = atol(zbuf_data(tmpkey));
            }
            continue;
        }
        zbuf_reset(tmpkey);
        while(ps < str_end) {
            int ch = *ps;
            if (!isalpha(ch)){
                break;
            }
            zbuf_put(tmpkey, ztolower(ch));
            ps++;
            if (zbuf_len(tmpkey) > 10) {
                goto err;
            }
        }

        if ((!strcmp(zbuf_data(tmpkey), "null")) || (!strcmp(zbuf_data(tmpkey), "undefined"))) {
            zjson_reset(current_json);
            continue;
        }
        if (!strcmp(zbuf_data(tmpkey), "true")) {
            *zjson_get_bool_value(current_json) = 1;
            continue;
        }
        if (!strcmp(zbuf_data(tmpkey), "false")) {
            *zjson_get_bool_value(current_json) = 0;
            continue;
        }
        goto err;
    }
    ret = 1;
err:
    if (ret == 0) {
        zjson_reset(j);
    }
    zbuf_free(tmpkey);
    zvector_free(json_vec);
    return ret;
}

static void ___serialize_string(zbuf_t *result, const char *data, int size)
{
    zbuf_put(result, '"');
    char *ps = (char *)data;
    for (int i = 0; i < size; i++) {
        unsigned char ch = ps[i];
        if (ch == '\\') {
            zbuf_put(result, '\\');
            zbuf_put(result, '\\');
        } else if (ch == '\"') {
            zbuf_put(result, '\\');
            zbuf_put(result, '"');
        } else if (ch == '\b') {
            zbuf_put(result, '\\');
            zbuf_put(result, 'b');
        } else if (ch == '\f') {
            zbuf_put(result, '\\');
            zbuf_put(result, 'f');
        } else if (ch == '\r') {
            zbuf_put(result, '\\');
            zbuf_put(result, 'r');
        } else if (ch == '\n') {
            zbuf_put(result, '\\');
            zbuf_put(result, 'n');
        } else if (ch == '\t') {
            zbuf_put(result, '\\');
            zbuf_put(result, 't');
        } else if (ch == '\0') {
            zbuf_put(result, '\\');
            zbuf_put(result, '\0');
        }else {
            zbuf_put(result, ch);
        }
    }
    zbuf_put(result, '"');
}

static inline __attribute__((always_inline))  void ___serialize_string2(zbuf_t *result, zbuf_t *str)
{
    ___serialize_string(result, zbuf_data(str), zbuf_len(str));
}

void zjson_serialize(zjson_t *j, zbuf_t *result, int strict)
{
    zjson_t *current_json;
    long idx;
    zmap_node_t *map_it;
    zvector_t *json_vec = zvector_create(32); /* zjson_t* */
    zvector_t *array_vec = zvector_create(32); /* int */
    zvector_t *object_vec = zvector_create(32); /* zmap_node_t* */
    zvector_push(json_vec, j);
    zvector_push(array_vec, 0);
    zvector_push(object_vec, 0);
    int length;
    double d;
    long l;
    while(1) {
        if (zvector_len(json_vec)==0) {
            break;
        }
        zvector_pop(json_vec, (void **)&current_json);
        zvector_pop(array_vec, (void **)&idx);
        zvector_pop(object_vec, (void **)&map_it);

        switch(current_json?current_json->type:zvar_json_type_null) {
        case zvar_json_type_null:
            current_json = 0;
            if (zvector_len(json_vec)) {
                current_json = (zjson_t *)(zvector_data(json_vec)[zvector_len(json_vec)-1]);
            }
            if ((current_json==0) || (current_json->type != zvar_json_type_array)) {
                zbuf_puts(result, "null");
            } else {
                zbuf_puts(result, "null");
                /* like [1,,3] == [1,null,3] */
            }
            break;
        case zvar_json_type_string:
            ___serialize_string2(result, *zjson_get_string_value(current_json));
            break;
        case zvar_json_type_bool:
            zbuf_puts(result, ((*zjson_get_bool_value(current_json))?"true":"false"));
            break;
        case zvar_json_type_long:
            zbuf_printf_1024(result, "%ld", *zjson_get_long_value(current_json));
            break;
        case zvar_json_type_double:
            d = *zjson_get_double_value(current_json);
            l = (long)d;
            if ((l > 1000L * 1000 * 1000 * 1000) || (l < -1000L * 1000 * 1000 * 1000)){
                zbuf_printf_1024(result, "%e", d);
            } else {
                zbuf_printf_1024(result, "%lf", d);
            }
            break;
        case zvar_json_type_array:
            length = zjson_array_get_len(current_json);
            if (length == 0) {
                zbuf_puts(result, "[]");
                break;
            }
            if (idx == 0) {
                zbuf_put(result, '[');
                zvector_push(json_vec, current_json);
                idx++;
                zvector_push(array_vec, (void *)idx);
                zvector_push(object_vec, 0);
                break;
            }
            if (idx == length + 1) {
                zbuf_put(result, ']');
                break;
            }
            if ((idx >1) && (idx < length + 1)) {
                zbuf_put(result, ',');
            }
            zvector_push(json_vec, current_json);
            zvector_push(array_vec, (void *)(idx+1));
            zvector_push(object_vec, 0);
            
            zvector_push(json_vec, zjson_array_get(current_json, idx-1));
            zvector_push(array_vec, 0);
            zvector_push(object_vec, 0);
            break;
        case zvar_json_type_object:
            if (zjson_object_get_len(current_json) == 0) {
                zbuf_puts(result, "{}");
                break;
            }
            if (map_it == 0) {
                if (idx == 0) {
                    zbuf_put(result, '{');
                    zvector_push(json_vec, current_json);
                    zvector_push(array_vec, (void *)1);
                    zvector_push(object_vec, zmap_first(current_json->val.m));
                    break;
                } else {
                    zbuf_put(result, '}');
                    break;
                }
            } else {
                if (map_it != zmap_first(current_json->val.m)) { /* FIXME */
                    zbuf_put(result, ',');
                }
                zvector_push(json_vec, current_json);
                zvector_push(array_vec, (void *)1);
                zvector_push(object_vec, zmap_next(map_it));
                ___serialize_string(result, zmap_node_key(map_it), strlen(zmap_node_key(map_it)));
                zbuf_put(result, ':');
                zvector_push(json_vec, zmap_node_value(map_it));
                zvector_push(array_vec, 0);
                zvector_push(object_vec, 0);
                break;
            }
            break;
        default:
            break;
        }
    }
    zvector_free(json_vec);
    zvector_free(array_vec);
    zvector_free(object_vec);
}
