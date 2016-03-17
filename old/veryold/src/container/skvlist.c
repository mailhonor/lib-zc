#include "zc.h"

static int zskvlist_arrangement(ZSKVLIST * kv);
static int zskvlist_stat_update(ZSKVLIST * kv);

ZSKVLIST *zskvlist_create(char *path)
{
	ZSKVLIST *kv;

	kv = (ZSKVLIST *) zcalloc(1, sizeof(ZSKVLIST));
	kv->path = zstr_strdup(path);
	kv->lock_fd = -1;
	kv->dict = zdict_create();

	return kv;
}

void zskvlist_free(ZSKVLIST * kv)
{
	kv->locks = 1;
	zskvlist_end(kv);
	zstr_free(kv->path);
	zdict_free_STR(kv->dict);
	zfree(kv);
}

int zskvlist_begin(ZSKVLIST * kv)
{
	char pbuf[1024];

	if (kv->locks) {
		kv->locks++;
		return 0;
	}

	sprintf(pbuf, "%s.lock", kv->path);
	kv->lock_fd = open(pbuf, O_RDWR | O_CREAT, 0666);
	if (kv->lock_fd == -1) {
		return -1;
	}
	if (zio_flock(kv->lock_fd, LOCK_EX)) {
		close(kv->lock_fd);
		kv->lock_fd = -1;
		return -1;
	}

	kv->list_fp = fopen(kv->path, "a+");
	if (!kv->list_fp) {
		close(kv->lock_fd);
		kv->lock_fd = -1;
		return -1;
	}

	kv->locks = 1;

	return 0;
}

int zskvlist_end(ZSKVLIST * kv)
{
	if (!kv->locks) {
		return 0;
	}
	kv->locks--;
	if (kv->locks) {
		return 0;
	}
	if (kv->list_fp) {
		fclose(kv->list_fp);
		kv->list_fp = 0;
	}
	if (kv->lock_fd != -1) {
		zio_flock(kv->lock_fd, LOCK_UN);
		close(kv->lock_fd);
		kv->lock_fd = -1;
	}

	zskvlist_stat_update(kv);

	return 0;
}

int zskvlist_changed(ZSKVLIST * kv)
{
	struct stat st;

	if (stat(kv->path, &st) < 0) {
		return -1;
	}
	if (st.st_ino != kv->ino) {
		return 1;
	}
	if (st.st_mtime != (kv->mtime)) {
		return 1;
	}
	if (st.st_size != kv->size) {
		return 1;
	}

	return 0;
}

static int zskvlist_stat_update(ZSKVLIST * kv)
{
	struct stat st;

	if (stat(kv->path, &st) < 0) {
		return -1;
	}
	st.st_ino = kv->ino;
	st.st_mtime = kv->mtime;
	st.st_size = kv->size;

	return 0;
}

int zskvlist_enter(ZSKVLIST * kv, char *key, char *value)
{
	FILE *fp;

	if ((!key) && (!value)) {
		return 0;
	}

	if (zskvlist_begin(kv)) {
		return -1;
	}
	fp = kv->list_fp;
	value = (value ? value : "");
	if (key) {
		fprintf(fp, "%s=%s\n", key, value);
	} else {
		fprintf(fp, "%s=\n", value);
	}

	zskvlist_end(kv);

	if (key) {
		zdict_enter_STR(kv->dict, key, value);
	} else {
		zdict_remove_STR(kv->dict, value);
	}

	return 0;
}

int zskvlist_delete(ZSKVLIST * kv, char *key)
{
	return zskvlist_enter(kv, 0, key);
}

int zskvlist_lookup(ZSKVLIST * kv, char *key, char **value)
{
	if (zdict_lookup(kv->dict, key, value)) {
		return 1;
	}

	return 0;
}

int zskvlist_load(ZSKVLIST * kv)
{
	FILE *fp;
	char lbuf[102401];
	char *name, *ne, *value;
	int llen;
	ZDICT *dict;
	int line_count = 0;

	if (zskvlist_begin(kv)) {
		return -1;
	}
	fp = kv->list_fp;
	fseek(fp, 0, SEEK_SET);

	dict = zdict_create();
	while ((fgets(lbuf, 102401, fp))) {
		line_count++;
		llen = strlen(lbuf);
		if (llen < 2) {
			continue;
		}
		if (lbuf[llen - 1] == '\n') {
			llen--;
		}
		if (lbuf[llen - 1] == '\r') {
			llen--;
		}
		lbuf[llen] = 0;
		if (llen < 2) {
			continue;
		}
		name = lbuf;
		llen--;
		if (*name == '=') {
			continue;
		}
		ne = strchr(name, '=');
		if (!ne) {
			continue;
		}
		*ne = 0;
		value = ne + 1;
		if (*value) {
			zdict_enter_STR(dict, name, value);
		} else {
			zdict_remove_STR(dict, name);
		}
	}

	zdict_free_STR(kv->dict);
	kv->dict = dict;

	if (line_count > 1000) {
		if ((1.5 * dict->len) < line_count) {
			zskvlist_arrangement(kv);
		}
	}

	zskvlist_end(kv);

	return 0;
}

static int zskvlist_arrangement(ZSKVLIST * kv)
{
	FILE *fp;
	char pbuf[1024];
	ZDICT *dict = kv->dict;
	ZDICT_NODE *n;

	sprintf(pbuf, "%s_arr", kv->path);
	fp = fopen(pbuf, "w+");
	if (!fp) {
		return -1;
	}
	for (n = zdict_first(dict); n; n = zdict_next(n)) {
		fprintf(fp, "%s=%s\n", n->key, (char *)(n->value));
	}

	fflush(fp);
	fclose(fp);

	if (rename(pbuf, kv->path)) {
		return -1;
	}

	fp = fopen(pbuf, "a+");
	if (!fp) {
		return -1;
	}

	fclose(kv->list_fp);
	kv->list_fp = fp;

	return 0;
}
