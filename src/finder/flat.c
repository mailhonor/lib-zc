
/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-07
 * ================================
 */

#include "libzc.h"

typedef struct zfinder_flat_t zfinder_flat_t;
struct zfinder_flat_t {
    char *fn;
    zdict_t *dict;
};

static int _close(zfinder_t * finder)
{
    zfinder_flat_t *my_db = (zfinder_flat_t *)(finder->db);

    if (my_db->dict) {
        zdict_free(my_db->dict);
    }
    zfree(my_db);

    return 0;
}

static int _get(zfinder_t * finder, const char *query, zbuf_t * result, int timeout)
{
    zfinder_flat_t *my_db = (zfinder_flat_t *)(finder->db);
    char *v;
    int len;

    if (zdict_lookup(my_db->dict, query, &v)) {
        len = strlen(v);
        if (len > 10240) {
            len = 10240;
        }
        if (len > 0) {
            zbuf_memcpy(result, v, len);
            zbuf_terminate(result);
        }
        return 1;
    }

    return 0;
}

int ___load_data(zfinder_t * finder)
{
    zfinder_flat_t *my_db;
    FILE *fp;
    char buf_raw[10240 + 10], *buf, *p;
    int len;

    my_db = (zfinder_flat_t *) (finder->db);
    fp = fopen(my_db->fn, "r");
    if (!fp) {
        zfatal("zfinder_create: %s, can not open (%m)", finder->title);
    }
    while ((!feof(fp)) && (!ferror(fp))) {
        if (!fgets(buf_raw, 10240, fp)) {
            break;
        }
        buf = buf_raw;
        while (*buf) {
            if (*buf == ' ' || *buf == '\t') {
                p++;
                continue;
            }
            break;
        }
        if (buf[0] == '#') {
            continue;
        }
        if (buf[0] == 0) {
            continue;
        }
        p = strchr(buf, ' ');
        if (!p) {
            continue;
        }
        *p++ = 0;
        while (*p) {
            if (*p == ' ' || *p == '\t') {
                p++;
                continue;
            }
            break;
        }
        len = strlen(p);
        if ((len > 0) && (p[len - 1] == '\n')) {
            len--;
        }
        if ((len > 0) && (p[len - 1] == '\r')) {
            len--;
        }
        p[len] = 0;
        zdict_add(my_db->dict, buf, p);
    }

    fclose(fp);

    return 0;
}

int zfinder_create_flat(zfinder_t * finder)
{
    zfinder_flat_t *my_db;

    my_db = (zfinder_flat_t *) zcalloc(1, sizeof(zfinder_flat_t));
    finder->db = my_db;
    finder->close = _close;
    finder->get = _get;

    my_db->fn = finder->uri;
    my_db->dict = zdict_create();

    ___load_data(finder);

    return 0;
}
