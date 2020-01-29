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

zjson_t *zjson_create(void)
{
    zjson_t *j = (zjson_t *)zcalloc(1, sizeof(zjson_t));
    j->type = zvar_json_type_null;
    return j;
}

zjson_t *zjson_create_bool(zbool_t b)
{
    zjson_t *j = (zjson_t *)zcalloc(1, sizeof(zjson_t));
    j->type = zvar_json_type_bool;
    j->val.b = b;
    return j;
}

zjson_t *zjson_create_long(long l)
{
    zjson_t *j = (zjson_t *)zcalloc(1, sizeof(zjson_t));
    j->type = zvar_json_type_long;
    j->val.l = l;
    return j;
}

zjson_t *zjson_create_double(double d)
{
    zjson_t *j = (zjson_t *)zcalloc(1, sizeof(zjson_t));
    j->type = zvar_json_type_double;
    j->val.d = d;
    return j;
}

zjson_t *zjson_create_string(const void *s, int len)
{
    if (len < 0) {
        len = strlen(ZCONVERT_CHAR_PTR(s));
    }
    zjson_t *j = (zjson_t *)zcalloc(1, sizeof(zjson_t));
    j->type = zvar_json_type_string;
    j->val.s = zbuf_create(len);
    zbuf_memcpy(j->val.s, s, len);
    return j;
}

void zjson_free(zjson_t *j)
{
    if (j==0) {
        return;
    }

    if ((j->type == zvar_json_type_object) || (j->type == zvar_json_type_array)) {
        zjson_reset(j);
    } else if (j->type == zvar_json_type_string) {
        zbuf_free(j->val.s);
    }
    zfree(j);
}

void zjson_reset(zjson_t *j)
{
    int need_free;
    zjson_t *js, *js2;
    zvector_t *for_deteled = zvector_create(32);
    zvector_push(for_deteled, j);
    while (zvector_pop(for_deteled, (void **)&js)) {
        if (js == 0) {
            continue;
        }
        need_free = 0;
        if (js->type == zvar_json_type_string) {
            zbuf_free(js->val.s);
            need_free = 1;
        } else if (js->type == zvar_json_type_array) {
            if (zvector_len(js->val.v) == 0) {
                zvector_free(js->val.v);
                need_free = 1;
            } else {
                zvector_pop(js->val.v, (void **)&js2);
                zvector_push(for_deteled, js);
                need_free = 0;
                zvector_push(for_deteled, js2);
            }
        } else if (js->type == zvar_json_type_object) {
            if (zmap_len(js->val.m) == 0) {
                zmap_free(js->val.m);
                need_free = 1;
            } else {
                zmap_delete_node(js->val.m, zmap_first(js->val.m), (void **)&js2);
                zvector_push(for_deteled, js);
                need_free = 0;
                zvector_push(for_deteled, js2);
            }
        } else {
            need_free = 1;
        }
        if (need_free) {
            if (js != j) {
                zfree(js);
            }
        }
    }
    j->type = zvar_json_type_null;
    zvector_free(for_deteled);
}

zbuf_t **zjson_get_string_value(zjson_t *j)
{
    if (j->type != zvar_json_type_string) {
        zjson_reset(j);
    }
    if (j->type == zvar_json_type_null) {
        j->val.s = zbuf_create(-1);
        j->type = zvar_json_type_string;
    }
    return &(j->val.s);
}

long *zjson_get_long_value(zjson_t *j)
{
    if (j->type != zvar_json_type_long) {
        zjson_reset(j);
    }
    if (j->type == zvar_json_type_null) {
        j->val.l = 0;
        j->type = zvar_json_type_long;
    }
    return &(j->val.l);
}

double *zjson_get_double_value(zjson_t *j)
{
    if (j->type != zvar_json_type_double) {
        zjson_reset(j);
    }
    if (j->type == zvar_json_type_null) {
        j->val.d = 0;
        j->type = zvar_json_type_double;
    }
    return &(j->val.d);
}

zbool_t *zjson_get_bool_value(zjson_t *j)
{
    if (j->type != zvar_json_type_bool) {
        zjson_reset(j);
    }
    if (j->type == zvar_json_type_null) {
        j->val.b = 0;
        j->type = zvar_json_type_bool;
    }
    return &(j->val.b);
}

const zvector_t *zjson_get_array_value(zjson_t *j)
{
    if (j->type != zvar_json_type_array) {
        zjson_reset(j);
    }
    if (j->type == zvar_json_type_null) {
        j->val.v = zvector_create(-1);
        j->type = zvar_json_type_array;
    }
    return j->val.v;
}

const zmap_t *zjson_get_object_value(zjson_t *j)
{
    if (j->type != zvar_json_type_object) {
        zjson_reset(j);
    }
    if (j->type == zvar_json_type_null) {
        j->val.m = zmap_create();
        j->type = zvar_json_type_object;
    }
    return j->val.m;
}

zjson_t *zjson_array_get(zjson_t *j, int idx)
{
    if (idx < 0) {
        return 0;
    }
    if (j->type != zvar_json_type_array) {
        zjson_get_array_value(j);
    }
    if (zvector_len(j->val.v) <= idx) {
        return 0;
    }
    return (zjson_t *)(zvector_data(j->val.v)[idx]);
}

zjson_t *zjson_object_get(zjson_t *j, const char *key)
{
    if (j->type != zvar_json_type_object) {
        zjson_get_object_value(j);
    }
    zjson_t *r;
    if (zmap_find(j->val.m, key, (void **)&r)) {
        return r;
    }
    return 0;
}

int zjson_array_get_len(zjson_t *j)
{
    if (j->type != zvar_json_type_array) {
        zjson_get_array_value(j);
    }
    return zvector_len(j->val.v);
}

int zjson_object_get_len(zjson_t *j)
{
    if (j->type != zvar_json_type_object) {
        zjson_get_object_value(j);
    }
    return zmap_len(j->val.m);
}

zjson_t *zjson_array_push(zjson_t *j, zjson_t *element)
{
    element->parent = j;
    zvector_t *v = (zvector_t *)(void *)zjson_get_array_value(j);
    zvector_push(v, element);
    return element;
}

zbool_t zjson_array_pop(zjson_t *j, zjson_t **element)
{
    zjson_t *tmpj = 0;
    zvector_t *v = (zvector_t *)(void *)zjson_get_array_value(j);
    zbool_t ret = zvector_pop(v, (void **)tmpj);
    if (ret) {
        if (tmpj) {
            tmpj->parent = 0;
        }
        if (element) {
            *element = tmpj;
        } else {
            zjson_free(tmpj);
        }
    } else {
        if (element) {
            *element = 0;
        }
    }
    return ret;
}

zjson_t *zjson_array_unshift(zjson_t *j, zjson_t *element)
{
    element->parent = j;
    zvector_t *v = (zvector_t *)(void *)zjson_get_array_value(j);
    zvector_unshift(v, element);
    return element;
}

zbool_t zjson_array_shift(zjson_t *j, zjson_t **element)
{
    zjson_t *tmpj = 0;
    zvector_t *v = (zvector_t *)(void *)zjson_get_array_value(j);
    zbool_t ret = zvector_shift(v, (void **)tmpj);
    if (ret) {
        if (tmpj) {
            tmpj->parent = 0;
        }
        if (element) {
            *element = tmpj;
        } else {
            zjson_free(tmpj);
        }
    } else {
       if (element) {
            *element = 0;
        }
    }
    return ret;
}

zjson_t *zjson_array_update(zjson_t *j, int idx, zjson_t *element, zjson_t **old_element)
{
    element->parent = j;
    zjson_t *tmpj = 0;
    zvector_t *v = (zvector_t *)(void *)zjson_get_array_value(j);
    if (idx < 0) {
        idx = v->len;
    }
    if (idx < zvector_len(v)) {
        tmpj = (zjson_t *)(zvector_data(v)[idx]);
        zvector_data(v)[idx] = (char *)element;
    } else {
        zvector_insert(v, idx, element);
    }
    if (old_element) {
        if (tmpj) {
            tmpj->parent = 0;
        }
        *old_element = tmpj;
    } else if (tmpj) {
        zjson_free(tmpj);
    }
    return element;
}

zjson_t *zjson_array_insert(zjson_t *j, int idx, zjson_t *element)
{
    zvector_t *v = (zvector_t *)(void *)zjson_get_array_value(j);
    zvector_insert(v, idx, element);
    return element;
}

void zjson_array_delete(zjson_t *j, int idx, zjson_t **old_element)
{
    zjson_t *tmpj = 0;
    zvector_t *v = (zvector_t *)(void *)zjson_get_array_value(j);
    if (zvector_delete(v, idx, (void **)&tmpj)) {
        if (tmpj) {
            tmpj->parent = 0;
        }
        if (old_element) {
            *old_element = tmpj;
        } else {
            zjson_free(tmpj);
        }
    } else {
        if (old_element) {
            *old_element = 0;
        }
    }
}

zjson_t *zjson_object_update(zjson_t *j, const char *key, zjson_t *element, zjson_t **old_element)
{
    element->parent = j;
    zjson_t *tmpj = 0;
    zmap_t *m = (zmap_t *)(void *)zjson_get_object_value(j);
    zmap_update(m, key, element, (void **)&tmpj);
    if (old_element) {
        if (tmpj) {
            tmpj->parent = 0;
        }
        *old_element = tmpj;
    } else if (tmpj){
        zjson_free(tmpj);
    }
    return element;
}

void zjson_object_delete(zjson_t *j, const char *key, zjson_t **old_element)
{
    zjson_t *tmpj = 0;
    zmap_t *m = (zmap_t *)(void *)zjson_get_object_value(j);
    zmap_delete(m, key, (void **)&tmpj);
    if (old_element) {
        if (tmpj) {
            tmpj->parent = 0;
        }
        *old_element = tmpj;
    } else if (tmpj){
        zjson_free(tmpj);
    }
}

zjson_t *zjson_get_element_by_path(zjson_t *j, const char *path)
{
    if (zempty(path)) {
        return j;
    }
    char *path_buf = zstrdup(path), *ps = path_buf, *p, *p2;
    zjson_t *jn = j;
    int idx;
    while (jn) {
        p = strchr(ps, '/');
        if (p) {
            *p++ = 0;
        }
        if (jn->type == zvar_json_type_array) {
            p2 = ps;
            idx = 0;
            while(*p2) {
                if (!isdigit(*p2)) {
                    jn = 0;
                    break;
                }
                idx = idx * 10 + (*p2 - '0');
                p2++;
            }
            if (jn) {
                jn = zjson_array_get(jn, idx);
            }
        } else if (jn->type == zvar_json_type_object) {
            jn = zjson_object_get(jn, ps);
        } else {
            jn = 0;
        }
        if (!p) {
            break;
        }
        ps = p;
    }
    zfree(path_buf);

    return jn;
}

zjson_t *zjson_get_element_by_path_vec(zjson_t *j, const char *path0, ...)
{
    if (zempty(path0)) {
        return 0;
    }
    zjson_t *jn = j;
    int first = 1, idx;
    char *ps, *p;
    va_list ap;
    va_start(ap, path0);
    while(jn) {
        if (first) {
            first = 0;
            ps = ZCONVERT_CHAR_PTR(path0);
        } else {
            ps = va_arg(ap, char *);
        }
        if (ps == 0) {
            break;
        }
        if (jn->type == zvar_json_type_array) {
            p = ps;
            idx = 0;
            while(*p) {
                if (!isdigit(*p)) {
                    jn = 0;
                }
                idx = idx * 10 + (*p - '0');
                p++;
            }
            if (jn) {
                jn = zjson_array_get(jn, idx);
            }
        } else if (jn->type == zvar_json_type_object) {
            jn = zjson_object_get(jn, ps);
        } else {
            jn = 0;
        }
    }
    va_end(ap);
    return jn;
}

zjson_t *zjson_get_top(zjson_t *j)
{
    zjson_t *r = j;
    while (r->parent) {
        r = r->parent;
    }
    return r;
}
