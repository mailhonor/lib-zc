#include "zc.h"

/* MARK:
 * The code only support zmalloc/zfree ...
 */

typedef struct {
	int line;
	char *fn;
	char *data;
	int count;
	ZRBTREE_NODE rb;
} ZLEAK;
ZRBTREE z_malloc_leak_list_buf, *z_malloc_leak_list = &z_malloc_leak_list_buf;

static pthread_mutex_t zleak_locker = PTHREAD_MUTEX_INITIALIZER;
static int zleak_check_flag = 0;
static int ___cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2);
static void zleak_enter(void *ptr, int line, char *fn);
static ZLEAK *zleak_lookup(void *ptr);
static void zleak_report(void);

static int ___cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	ZLEAK *e1, *e2;

	e1 = ZCONTAINER_OF(n1, ZLEAK, rb);
	e2 = ZCONTAINER_OF(n2, ZLEAK, rb);

	return (e1->data - e2->data);
}

static void zleak_enter(void *ptr, int line, char *fn)
{
	ZLEAK *zl;

	zl = zleak_lookup(ptr);

	if (zl) {
		zl->count++;
		return;
	}

	zl = malloc(sizeof(ZLEAK));
	zl->line = line;
	zl->fn = strdup(fn);
	zl->data = (char *)ptr;
	zl->count = 1;

	zrbtree_attach(z_malloc_leak_list, &(zl->rb));
}

static ZLEAK *zleak_lookup(void *ptr)
{
	ZLEAK zl;
	ZRBTREE_NODE *rn;

	zl.data = (char *)ptr;

	rn = zrbtree_lookup(z_malloc_leak_list, &(zl.rb));

	if (!rn) {
		return 0;
	}
	return ((ZLEAK *) ZCONTAINER_OF(rn, ZLEAK, rb));
}

static void zleak_report(void)
{
	ZLEAK *zl;
	ZRBTREE_NODE *rn;
	int find = 0;

	printf("now print leak memory\n");
	for (rn = zrbtree_first(z_malloc_leak_list); rn; rn = zrbtree_next(rn)) {
		zl = ZCONTAINER_OF(rn, ZLEAK, rb);
		if (zl->count == 0) {
			continue;
		}
		printf("leak: FN: %-30s LN:%-6d CT: %d\n", zl->fn, zl->line, zl->count);
		find = 1;
	}
	if (!find) {
		printf("very good for not found leak memory\n\n");
	}
}

void zleak_init(void)
{
	zrbtree_init(z_malloc_leak_list, ___cmp);
	zleak_check_flag = 1;
	atexit(zleak_report);
}

void *zleak_malloc(size_t len, int line, char *fn)
{
	void *r;

	r = malloc(len);

	if (zleak_check_flag) {
		pthread_mutex_lock(&zleak_locker);
		zleak_enter(r, line, fn);
		pthread_mutex_unlock(&zleak_locker);
	}

	return r;
}

void *zleak_calloc(size_t nmemb, size_t size, int line, char *fn)
{
	void *r;

	(r = calloc(nmemb, size));

	if (zleak_check_flag) {
		pthread_mutex_lock(&zleak_locker);
		zleak_enter(r, line, fn);
		pthread_mutex_unlock(&zleak_locker);
	}

	return r;
}

void *zleak_realloc(void *ptr, size_t len, int line, char *fn)
{
	void *r;
	ZLEAK *zl;

	r = realloc(ptr, len);

	if (zleak_check_flag) {
		pthread_mutex_lock(&zleak_locker);
		if (ptr) {
			zl = zleak_lookup(ptr);
			if (!zl) {
				zlog_fatal("ptr should be found : %s: %d", fn, line);
			}
			free(zl->fn);
			zl->fn = strdup(fn);
			zl->line = line;
			if (r != ptr) {
				zrbtree_detach(z_malloc_leak_list, &(zl->rb));
				zl->data = (char *)r;
				zrbtree_attach(z_malloc_leak_list, &(zl->rb));
			}
		} else {
			zleak_enter(r, line, fn);
		}
		pthread_mutex_unlock(&zleak_locker);
	}

	return r;
}

void zleak_free(void *ptr, int line, char *fn)
{
	ZLEAK *zl;

	if (!ptr || (ptr == (void *)z_string_empty)) {
		return;
	}

	if (zleak_check_flag) {
		pthread_mutex_lock(&zleak_locker);
		zl = zleak_lookup(ptr);
#if 0
		if (!zl) {
			zlog_fatal("ptr should be found : %s: %d, may is double free", fn, line);
		}
		zl->count--;
		if (zl->count == 0) {
			//zrbtree_detach(z_malloc_leak_list, &(zl->rb));
			//free(zl->fn);
			//free(zl);
		}
#else
		if (zl) {
			zl->count--;
			if (zl->count == 0) {
				//zrbtree_detach(z_malloc_leak_list, &(zl->rb));
				//free(zl->fn);
				//free(zl);
			}
		}
#endif
		pthread_mutex_unlock(&zleak_locker);
	}
}

char *zleak_str_strdup(const char *ptr, int line, char *fn)
{
	char *r;
	size_t len;

	if (!ptr)
		return z_string_empty;
	len = strlen(ptr);
	r = (char *)zleak_malloc(len + 1, line, fn);
	if (r == NULL) {
		zlog_fatal("zleak_str_strdup: insufficient memory : %m");
	}
	memcpy(r, ptr, len);
	r[len] = 0;

	return r;
}

char *zleak_str_strndup(const char *ptr, size_t n, int line, char *fn)
{
	char *r;
	size_t len;

	if (!ptr)
		return z_string_empty;
	len = strlen(ptr);
	if (len > n) {
		len = n;
	}
	r = (char *)zleak_malloc(len + 1, line, fn);
	if (r == NULL) {
		zlog_fatal("zleak_str_strndup: insufficient memory for %ld bytes: %m", (long)n);
	}
	memcpy(r, ptr, len);
	r[len] = 0;

	return r;
}

char *zleak_str_memdup(void *ptr, size_t n, int line, char *fn)
{
	char *r;

	if (!ptr)
		return z_string_empty;
	r = (char *)zleak_malloc(n + 1, line, fn);
	memcpy(r, ptr, n);
	r[n] = 0;

	return r;
}

void zleak_str_free(void *ptr, int line, char *fn)
{
	zleak_free(ptr, line, fn);
}
