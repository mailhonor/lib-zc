/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-20
 * ================================
 */

#include "zc.h"

#define _VEC_LEFT_(v) (((v)->size - (v)->offset) - (v)->len)

typedef struct zvector_mpool_t zvector_mpool_t;
struct zvector_mpool_t {
    zvector_t v;
    zmpool_t *mpool;
};

void zvector_init(zvector_t *v, int size)
{
    if (size < 0) {
        size = 13;
    } else if (size == 0) {
        size = 1;
    }
    v->data = (char **)zmalloc((size + 1) * sizeof(char *));
    v->data[0] = 0;
    v->size = size;
    v->len = 0;
    v->offset = 0;
    v->mpool_used = 0;
}

void zvector_init_mpool(zvector_t *v, int size, zmpool_t *mpool)
{
    if (size < 0) {
        size = 13;
    } else if (size == 0) {
        size = 1;
    }
    v->data = (char **)zmpool_malloc(mpool, (size + 1) * sizeof(char *));
    v->data[0] = 0;
    v->size = size;
    v->len = 0;
    v->offset = 0;

    v->mpool_used = 1;
    zvector_mpool_t *vmv = (zvector_mpool_t *)v;
    vmv->mpool = mpool;
}

void zvector_fini(zvector_t *v)
{
    if (v->mpool_used) {
        zvector_mpool_t *vmv = (zvector_mpool_t *)v;
        zmpool_free(vmv->mpool, (v->data)-(v->offset));
    } else {
        zfree((v->data)-(v->offset));
    }
    memset(v, 0, sizeof(zvector_t));
}

zvector_t *zvector_create(int size)
{
    zvector_t *v = (zvector_t *) zmalloc(sizeof(zvector_t));
    zvector_init(v, size);
    return (v);
}

void zvector_free(zvector_t * v)
{
    zvector_fini(v);
    zfree(v);
}

void zvector_push(zvector_t * v, const void *val)
{
    if (_VEC_LEFT_(v) < 1) {
        if (v->mpool_used) {
            zvector_mpool_t *vmv = (zvector_mpool_t *)v;
            char ** n = (char **)zmpool_malloc(vmv->mpool, ((v->size)*2+1)*sizeof(char *));
            if (v->len > 0) {
                memcpy(n+(v->offset), v->data, (v->len)*sizeof(char *));
            }
            zmpool_free(vmv->mpool, (v->data)-(v->offset));
            v->data = n + (v->offset);
        } else {
            v->data = ((char **)zrealloc((v->data)-(v->offset), ((v->size)*2+1)*sizeof(char *)))+(v->offset);
        }
        v->size *= 2;
    }
    v->data[v->len++] = ZCONVERT_CHAR_PTR(val);
    v->data[v->len] = 0;
}

void zvector_unshift(zvector_t *v, const void *val)
{
    int i, len = v->len, nsize;
    char **data = v->data;
    if (v->offset == 0) {
        if (_VEC_LEFT_(v) > 100) {
            for (i = len-1; i > -1; i--) {
                data[i+100] = data[i];
            }
            v->data += 99;
            v->offset = 99;
            v->data[0] = ZCONVERT_CHAR_PTR(val);
        } else {
            nsize = (v->size)*2 + 100;
            zvector_mpool_t *vmv = (zvector_mpool_t *)v;
            char **n;
            if (v->mpool_used) {
                n = (char **)zmpool_malloc(vmv->mpool, (nsize+1)*sizeof(char *));
            } else {
                n = (char **)zmalloc((nsize+1)*sizeof(char *));
            }
            if (len > 0) {
                memcpy(n + 100, v->data, len*sizeof(char *));
            }
            if (v->mpool_used) {
                zmpool_free(vmv->mpool, (v->data));
            } else {
                zfree(v->data);
            }
            v->data = n + 99;
            v->offset = 99;
            v->data[0] = ZCONVERT_CHAR_PTR(val);
            v->size = nsize;
        }
    } else {
        v->offset--;
        v->data--;
        v->data[0] = ZCONVERT_CHAR_PTR(val);
    }
    v->len++;
    v->data[v->len] = 0;
}

zbool_t zvector_pop(zvector_t *v, void **val)
{
    if (val) {
        *val = 0;
    }
    if (v->len < 1) {
        return 0;
    }
    if (val){
        *val = (v->data)[v->len-1];
    }
    v->len--;
    (v->data)[v->len] = 0;
    return 1;
}

zbool_t zvector_shift(zvector_t *v, void **val)
{
    if (val) {
        *val = 0;
    }
    if (v->len < 1) {
        return 0;
    }
    if (val){
        *val = (v->data)[0];
    }
    v->offset++;
    v->data++;
    v->len--;
    if (v->offset > 200) {
        int len = v->len, i;
        char **data = v->data, **ndata = data - (v->offset) + 100;
        for(i=0;i<len;i++) {
            ndata[i] = data[i];
        }
        v->offset = 100;
        v->data = ndata;
        ndata[len] = 0;
    }
    return 1;
}

void zvector_insert(zvector_t *v, int idx, void *val)
{
    int len = v->len, i;
    if (idx < 0) {
        idx = len;
    }
    if (idx == 0) {
        zvector_unshift(v, val);
    } else if (idx == len) {
        zvector_push(v, val);
    } else if (idx < len) {
        zvector_push(v, 0);
        char **data = v->data;
        for (i=len;i>idx;i--) {
            data[i] = data[i-1];
        }
        data[idx] = val;
        data[v->len] = 0;
    } else /* if (idx > len) */ {
        while(idx > v->len) {
            zvector_push(v, 0);
        }
        zvector_push(v, val);
    }
}

zbool_t zvector_delete(zvector_t *v, int idx, void **val)
{
    if (val) {
        *val = 0;
    }
    int len = v->len, i;
    if ((idx < 0) || (idx>=(v->len))) {
        return 0;
    }
    if (idx == 0) {
        return zvector_shift(v, val);
    } else {
        char **data = v->data;
        if (val) {
            *val = data[idx];
        }
        for (i=idx;i<len;i++) {
            data[i] = data[i+1];
        }
        v->len--;
        data[v->len] = 0;
        return 1;
    }
    return 0;
}

void zvector_reset(zvector_t * v)
{
    v->len = 0;
    v->data[0] = 0;
}

void zvector_truncate(zvector_t * v, int new_len)
{
    if (new_len < v->len) {
        v->len = new_len;
    }
    v->data[v->len] = 0;
}

void zbuf_vector_reset(zvector_t *v)
{
    if (!v) {
        return;
    }
    ZVECTOR_WALK_BEGIN(v, zbuf_t *, bf) {
        if (bf) {
            zbuf_free(bf);
        }
    } ZVECTOR_WALK_END;
    v->len = 0;
    v->data[0] = 0;
}

void zbuf_vector_free(zvector_t *v)
{
    if (!v) {
        return;
    }
    ZVECTOR_WALK_BEGIN(v, zbuf_t *, bf) {
        if (bf) {
            zbuf_free(bf);
        }
    } ZVECTOR_WALK_END;
    zvector_free(v);
}

