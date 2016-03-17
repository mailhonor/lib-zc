#include "zc.h"

/*
 * leak.c
 */
#ifndef ZC_DEBUG
void *zmalloc_malloc(size_t len)
{
	void *r;

	if ((r = malloc(len)) == 0) {
		zlog_fatal("zmalloc: insufficient memory for %ld bytes: %m", (long)len);
	}
	return r;
}

void *zmalloc_calloc(size_t nmemb, size_t size)
{
	void *r;

	if ((r = calloc(nmemb, size)) == 0) {
		zlog_fatal("zcalloc: insufficient memory for %ldx%ld bytes: %m", (long)nmemb, size);
	}
	return r;
}

void *zmalloc_realloc(void *ptr, size_t len)
{
	void *r;

	if ((r = realloc(ptr, len)) == 0) {
		zlog_fatal("zrealloc: insufficient memory for %ld bytes: %m", (long)len);
	}
	return r;
}

void zmalloc_free(void *ptr)
{
	if (ptr && (ptr != (void *)z_string_empty)) {
		free(ptr);
	}
}

char *zmalloc_str_strdup(const char *ptr)
{
	char *r;
	size_t len;

	if (!ptr)
		return z_string_empty;
	len = strlen(ptr);
	r = (char *)zmalloc(len + 1);
	if (r == NULL) {
		zlog_fatal("zmalloc_str_strdup: insufficient memory : %m");
	}
	memcpy(r, ptr, len);
	r[len] = 0;

	return r;
}

char *zmalloc_str_strndup(const char *ptr, size_t n)
{
	char *r;
	size_t len;

	if (!ptr)
		return z_string_empty;
	len = strlen(ptr);
	if (len > n) {
		len = n;
	}
	r = (char *)zmalloc(len + 1);
	if (r == NULL) {
		zlog_fatal("zmalloc_str_strndup: insufficient memory for %ld bytes: %m", (long)n);
	}
	memcpy(r, ptr, len);
	r[len] = 0;

	return r;
}

char *zmalloc_str_memdup(void *ptr, size_t n)
{
	char *r;

	if (!ptr)
		return z_string_empty;
	r = (char *)zmalloc(n + 1);
	memcpy(r, ptr, n);
	r[n] = 0;

	return r;
}

void zmalloc_str_free(void *ptr)
{
	zfree(ptr);
}

#endif
