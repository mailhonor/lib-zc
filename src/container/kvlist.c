/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-20
 * ================================
 */

#include "libzc.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

struct zkvlist_t
{
    char *path;
    zdict_t *dict;
    FILE *list_fp;
    int lock_fd;
    int locks;
    ino_t ino;
    time_t mtime;
    off_t size;
    int len;
};

static int zkvlist_arrangement(zkvlist_t * kv);
static int zkvlist_stat_update(zkvlist_t * kv);

zkvlist_t *zkvlist_create(char *path)
{
    zkvlist_t *kv;

    kv = (zkvlist_t *) zcalloc(1, sizeof(zkvlist_t));
    kv->path = zstrdup(path);
    kv->lock_fd = -1;
    kv->dict = zdict_create();

    return kv;
}

void zkvlist_free(zkvlist_t * kv)
{
    kv->locks = 1;
    zkvlist_end(kv);
    zfree(kv->path);
    zdict_free(kv->dict);
    zfree(kv);
}

zdict_t *zkvlist_get_dict(zkvlist_t * kv)
{
    return kv->dict;
}

int zkvlist_begin(zkvlist_t * kv)
{
    char pbuf[1024];

    if (kv->locks)
    {
        kv->locks++;
        return 0;
    }

    sprintf(pbuf, "%s.lock", kv->path);
    kv->lock_fd = open(pbuf, O_RDWR | O_CREAT, 0666);
    if (kv->lock_fd == -1)
    {
        return -1;
    }
    if (zflock(kv->lock_fd, LOCK_EX))
    {
        close(kv->lock_fd);
        kv->lock_fd = -1;
        return -1;
    }

    kv->list_fp = fopen(kv->path, "a+");
    if (!kv->list_fp)
    {
        close(kv->lock_fd);
        kv->lock_fd = -1;
        return -1;
    }

    kv->locks = 1;

    return 0;
}

int zkvlist_end(zkvlist_t * kv)
{
    if (!kv->locks)
    {
        return 0;
    }
    kv->locks--;
    if (kv->locks)
    {
        return 0;
    }
    if (kv->list_fp)
    {
        fclose(kv->list_fp);
        kv->list_fp = 0;
    }
    if (kv->lock_fd != -1)
    {
        zflock(kv->lock_fd, LOCK_UN);
        close(kv->lock_fd);
        kv->lock_fd = -1;
    }

    zkvlist_stat_update(kv);

    return 0;
}

int zkvlist_changed(zkvlist_t * kv)
{
    struct stat st;

    if (stat(kv->path, &st) < 0)
    {
        return -1;
    }
    if (st.st_ino != kv->ino)
    {
        return 1;
    }
    if (st.st_mtime != (kv->mtime))
    {
        return 1;
    }
    if (st.st_size != kv->size)
    {
        return 1;
    }

    return 0;
}

static int zkvlist_stat_update(zkvlist_t * kv)
{
    struct stat st;

    if (stat(kv->path, &st) < 0)
    {
        return -1;
    }
    st.st_ino = kv->ino;
    st.st_mtime = kv->mtime;
    st.st_size = kv->size;

    return 0;
}

int zkvlist_add(zkvlist_t * kv, char *key, char *value)
{
    FILE *fp;

    if ((!key) && (!value))
    {
        return 0;
    }

    if (zkvlist_begin(kv))
    {
        return -1;
    }
    fp = kv->list_fp;
    value = (value ? value : "");
    if (key)
    {
        fprintf(fp, "%s=%s\n", key, value);
    }
    else
    {
        fprintf(fp, "%s=\n", value);
    }

    zkvlist_end(kv);

    if (key)
    {
        zdict_add(kv->dict, key, value);
    }
    else
    {
        zdict_delete(kv->dict, value);
    }

    return 0;
}

int zkvlist_delete(zkvlist_t * kv, char *key)
{
    return zkvlist_add(kv, 0, key);
}

int zkvlist_lookup(zkvlist_t * kv, char *key, char **value)
{
    if (zdict_lookup(kv->dict, key, value))
    {
        return 1;
    }

    return 0;
}

int zkvlist_load(zkvlist_t * kv)
{
    FILE *fp;
    char lbuf[102401];
    char *name, *ne, *value;
    int llen;
    zdict_t *dict;
    int line_count = 0;

    if (zkvlist_begin(kv))
    {
        return -1;
    }
    fp = kv->list_fp;
    fseek(fp, 0, SEEK_SET);

    dict = zdict_create();
    while ((fgets(lbuf, 102401, fp)))
    {
        line_count++;
        llen = strlen(lbuf);
        if (llen < 2)
        {
            continue;
        }
        if (lbuf[llen - 1] == '\n')
        {
            llen--;
        }
        if (lbuf[llen - 1] == '\r')
        {
            llen--;
        }
        lbuf[llen] = 0;
        if (llen < 2)
        {
            continue;
        }
        name = lbuf;
        llen--;
        if (*name == '=')
        {
            continue;
        }
        ne = strchr(name, '=');
        if (!ne)
        {
            continue;
        }
        *ne = 0;
        value = ne + 1;
        if (*value)
        {
            zdict_add(dict, name, value);
        }
        else
        {
            zdict_delete(dict, name);
        }
    }

    zdict_free(kv->dict);
    kv->dict = dict;

    if (line_count > 1000)
    {
        if ((1.5 * dict->len) < line_count)
        {
            zkvlist_arrangement(kv);
        }
    }

    zkvlist_end(kv);

    return 0;
}

static int zkvlist_arrangement(zkvlist_t * kv)
{
    FILE *fp;
    char pbuf[1024];

    zdict_t *dict = kv->dict;
    zdict_node_t *n;

    sprintf(pbuf, "%s_arr", kv->path);
    fp = fopen(pbuf, "w+");
    if (!fp)
    {
        return -1;
    }
    for (n = zdict_first(dict); n; n = zdict_next(n))
    {
        fprintf(fp, "%s=%s\n", n->key, (char *)(n->value));
    }

    fflush(fp);
    fclose(fp);

    if (rename(pbuf, kv->path))
    {
        return -1;
    }

    fp = fopen(pbuf, "a+");
    if (!fp)
    {
        return -1;
    }

    fclose(kv->list_fp);
    kv->list_fp = fp;

    return 0;
}
