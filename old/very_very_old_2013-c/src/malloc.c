#include "zyc.h"
        
void *z_malloc(ssize_t len){
	void *r;

	if((r = malloc(len)) == 0)
		zmsg_fatal("z_malloc: insufficient memory for %ld bytes: %m", (long)len);
	return r;
}
void *z_realloc(char *ptr, ssize_t len){
	char *r;

	if((r = realloc(ptr, len)) == 0)
		zmsg_fatal("z_realloc: insufficient memory for %ld bytes: %m", (long)len);
	return r;
}

inline void z_free(void *ptr){
	free(ptr);
}

