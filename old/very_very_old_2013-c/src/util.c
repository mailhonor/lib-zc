#include "zyc.h"
#include <libgen.h>
        
char *z_strdup(const char *ptr){
	char *r;

	if(!ptr) return NULL;
	r=strdup(ptr);
	if(r==NULL){
		zmsg_fatal("z_strdup: insufficient memory : %m");
	}
	return r;
}
char *z_strndup(const char *ptr,size_t n){
	char *r;

	if(!ptr) return NULL;
	r=strndup(ptr, n);
	if(r==NULL){
		zmsg_fatal("z_strndup: insufficient memory for %ld bytes: %m", (long)n);
	}
	return r;
}

char *z_memdup(const char *ptr, size_t n){
	char *r;

	if(!ptr) return  NULL;
	r=(char *)z_malloc(n);
	memcpy(r, ptr, n);
	return r;
}

/* strtok */
void z_strtok_create(ZSTRTOK *k,  char *sstr)
{
	k->sstr = sstr;
	k->len = 0;
	k->str=NULL;
}
ZSTRTOK *z_strtok(ZSTRTOK *k, const char *delim)
{
	char *r = k->sstr;
	r += strspn(k->sstr, delim);
	if (*r == 0)
		return(NULL);
	k->len = strcspn(r, delim);
	if(k->len==0)
		return(NULL);
	k->sstr = r + k->len;
	k->str=r;
	return(k);
}

/* fgets etc */
int z_fget_delimiter(FILE *fp, ZSTR *zs, char delimiter){
	int c;

	zstr_reset(zs);
	while((c=fgetc(fp))!=EOF){
		zstr_put(zs, c);
		if(c == delimiter){
		       	break;
		}
	}
	zstr_terminate(zs);
	return zs->len;
}

int z_fget_delimiter_bound(FILE *fp, ZSTR *zs, char delimiter, int bound){
	int c;

	zstr_reset(zs);
	while(bound-->0 && (c=fgetc(fp))!=EOF){
		zstr_put(zs, c);
		if(c == delimiter){
		       	break;
		}
	}
	zstr_terminate(zs);
	return zs->len;
}

/* rand */
static int rand_init=0;
int z_rand(void){
	if(!rand_init){
		srand(getpid() ^ time((time_t *)0));
		rand_init=1;
	}
	return(rand());
}

/* realpath */
/* The z_pathjoin return value must not reinject into path1 or path2
 */
char *z_pathjoin(char *path1, char *path2){
	static char *buf1=0;
	static char *buf2=0;
	char *npath1, *r;
	struct stat st;
#define _Z_PATHJOIN_LEN		4096

	if(path1==0){
		if(buf1){
			z_free(buf1);buf1=0;
			z_free(buf2);buf2=0;
		}
		return 0;
	}

	if(buf1==0){
		buf1=(char *)z_malloc(_Z_PATHJOIN_LEN);
		buf2=(char *)z_malloc(_Z_PATHJOIN_LEN);
	}
	
	if(stat(path1, &st)==-1){
		if(errno == ENOENT){
			return 0;
		}else{
			zmsg_fatal("stat: %s :%m", path1);
		}
	}
	npath1=path1;
	if(((st.st_mode) & (S_IFMT)) != (S_IFDIR)){
		strcpy(buf2, path1);
		npath1=dirname(buf2);
	}
	if(path2==0){
		return realpath(path1, buf2);
	}
	if(path2[0]=='/'){
		return realpath(path2, buf2);
	}
	snprintf(buf1, _Z_PATHJOIN_LEN, "%s/%s", npath1, path2);
	r=realpath(buf1, buf2);
	return r;
}

/* mkdir */
int z_mkdir(const char *path){
	char *p = (char *) path;
	int save_errno;
	struct stat sbuf;

	while ((p = strchr(p+1, '/'))) {
		*p = '\0';
		if (mkdir(path, 0755) == -1 && errno != EEXIST) {
			save_errno = errno;
			if (stat(path, &sbuf) == -1) {
				errno = save_errno;
				zmsg_warning("IOERR: creating directory %s: ", path);
				return -1;
			}
		}
		*p = '/';
	}
	return 0;
}
