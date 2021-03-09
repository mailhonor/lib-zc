#include "zyc.h"
#include <test.h>

static ZSTR *___general_concatenate=NULL;

ZSTR * zstr_create(int len){
	ZSTR * s;
	if(len < 0)
		len = 0;
	s = (ZSTR *)z_malloc(sizeof(ZSTR));
	s->len = 0;
	s->size = len + 1;
	s->str = (char *)z_malloc(s->size);
	if(s->str == NULL){
		free(s);
		return(NULL);
	}
	s->str[0] = 0;
	return(s);
}

void zstr_free(ZSTR *zs){
	free(zstr_str(zs));
	free(zs);
}

inline void zstr_space(ZSTR *zs, int space){
	int incr;
	incr = zs->len + space - zs->size;
	if(incr>0){
		zs->size=zs->size+(zs->size<incr?incr:zs->size);
		zs->str = (char *)z_realloc(zs->str, zs->size+1);
	}
}

inline void zstr_put(ZSTR *zs, int ch){
	if(zs->len>=zs->size){
		zstr_space(zs, 1);
	}
	zs->str[zs->len++]=ch;
}

void zstr_truncate(ZSTR *zs, int len){
	if(len<0)
		len=0;
	if(zs->len > len){
		zs->str[len]=0;
		zs->len=len;
	}
}

ZSTR *zstr_strncpy(ZSTR *zs, char *src, int len) {
	zstr_reset(zs);
	while (len-- && *src) {
		ZSTR_PUT(zs, *src);
		src++;
	}
	zstr_terminate(zs);
	return(zs);
}

ZSTR *zstr_strcpy(ZSTR *zs, char *src) {
	zstr_reset(zs);
	while (*src) {
		ZSTR_PUT(zs, *src);
		src++;
	}
	zstr_terminate(zs);
	return (zs);
}

ZSTR *zstr_strncat(ZSTR *zs, char *src, int len) {
	while (len-- && *src) {
		ZSTR_PUT(zs, *src);
		src++;
	}
	zstr_terminate(zs);
	return (zs);
}

ZSTR *zstr_strcat(ZSTR *zs, char *src) {
	while (*src) {
		ZSTR_PUT(zs, *src);
		src++;
	}
	zstr_terminate(zs);
	return (zs);
}

ZSTR *zstr_strcats(ZSTR *zs, ...){
	va_list ap;
	char *arg;

	va_start(ap,zs);
	while ((arg = va_arg(ap, char *)) != 0)
		zstr_strcat(zs, arg);
	va_end(ap);
	zstr_terminate(zs);
	return (zs);
}

ZSTR *zstr_strcpys(ZSTR *zs, ...){
	va_list ap;
	char *arg;

	zstr_reset(zs);
	va_start(ap,zs);
	while ((arg = va_arg(ap, char *)) != 0)
		zstr_strcat(zs, arg);
	va_end(ap);
	zstr_terminate(zs);
	return (zs);
}

ZSTR *zstr_memcpy(ZSTR *zs, char *src, int len) {
	zstr_reset(zs);
	ZSTR_SPACE(zs, len);
	memcpy(zs->str, src, len);
	zs->len=len;
	zstr_terminate(zs);
	return (zs);
}

ZSTR *zstr_memcat(ZSTR *zs, char *src, int len) {
	ZSTR_SPACE(zs, len);
	memcpy(zs->str, src, len);
	zs->len+=len;
	zstr_terminate(zs);
	return (zs);
}

int zstr_snprintf_append(ZSTR *zs, int len, char *fmt, ...){
	va_list args;
	int i;

	zstr_space(zs, len+1);
	va_start(args, fmt);
	i = vsnprintf(zs->str + zs->len, len, fmt, args);
	va_end(args);
	zs->len += (i<len?i:len)-1;
	zstr_terminate(zs);
	return(i);
}

int zstr_vsnprintf_append(ZSTR *zs, int len, char *fmt, va_list ap){
	int i;

	zstr_space(zs, len+1);
	i = vsnprintf(zs->str + zs->len, len, fmt, ap);
	zs->len += (i<len?i:len)-1;
	zstr_terminate(zs);
	return(i);
}

int zstr_sprintf_append(ZSTR *zs, char *fmt, ...){
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret=zstr_vsprintf_append(zs, fmt, ap);
	va_end(ap);
	return ret;
}

int zstr_vsprintf_append(ZSTR *zs, char *format, va_list ap) {
	static ZSTR *fmt;
	unsigned char *cp;
	char *s;
	unsigned long_flag;
	int width, prec, ch;
	int plen, old_len;

	old_len=zstr_len(zs);

	/* FIXME
	 * The func is generally used for very many conditions, so this code may be use in confusion. example;
	 * sigal handler.
	 * All I says is followed "fmt".
	 */
	if (fmt == 0)
		fmt = zstr_create(128);

	for (cp = (unsigned char *) format; *cp; cp++) {
		if (*cp != '%') {
			ZSTR_PUT(zs, *cp);
			continue;
		}
		if (cp[1] == '%') {
			ZSTR_PUT(zs, *cp++);
			continue;
		} 
		zstr_reset(fmt);
		ZSTR_PUT(fmt, *cp++);
		if (*cp == '-')
			ZSTR_PUT(fmt, *cp++);
		if (*cp == '+')	
			ZSTR_PUT(fmt, *cp++);
		if (*cp == '0')
			ZSTR_PUT(fmt, *cp++);
		if (*cp == '*') {
			width = va_arg(ap, int);
			ZSTR_SPACE(fmt, 16);
			plen=sprintf(fmt->str+fmt->len, "%d", width);
			fmt->len+=plen;
			cp++;
		} else {
			for (width = 0; ch = *cp, isdigit(ch); cp++) {
				width = width * 10 + ch - '0';
				ZSTR_PUT(fmt, ch);
			}
		}
		if (width < 0) {
			zmsg_warn("zstr_vsnprintf: bad width %d in %.50s",width, format);
			width = 0;
		}
		if (*cp == '.')
			ZSTR_PUT(fmt, *cp++);
		if (*cp == '*') {
			prec = va_arg(ap, int);
			ZSTR_SPACE(fmt, 16);
			plen=sprintf(fmt->str+fmt->len, "%d", prec);
			fmt->len+=plen;
			cp++;
		} else {
			for (prec = 0; ch = *cp, isdigit(ch); cp++) {
				prec = prec * 10 + ch - '0';
				ZSTR_PUT(fmt, ch);
			}
		}
		if (prec < 0) {
			zmsg_warn("zstr_vsnprintf: bad precision %d in %.50s", prec, format);
			prec = 0;
		}
		if ((long_flag = (*cp == 'l')) != 0)
			ZSTR_PUT(fmt, *cp++);
		if (*cp == 0)
			break;
		ZSTR_PUT(fmt, *cp);
		zstr_terminate(fmt);

		switch (*cp) {
		case 's':
			s = va_arg(ap, char *);
			if (prec > 0 || (width > 0 && width > strlen(s))) {
				ZSTR_SPACE(zs, (width > prec ? width : prec) + 16);
				plen=sprintf(zs->str+zs->len, zstr_str(fmt), s);
				zs->len+=plen;
			} else {
				zstr_strcat(zs, s);
			}
			break;
		case 'c':
		case 'd':
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			ZSTR_SPACE(zs, (width > prec ? width : prec) + 16);
			if (long_flag)
				plen=sprintf(zs->str+zs->len, zstr_str(fmt), va_arg(ap, long));
			else
				plen=sprintf(zs->str+zs->len, zstr_str(fmt), va_arg(ap, int));
			zs->len+=plen;
			break;
		case 'e':	
		case 'f':
		case 'g':
			ZSTR_SPACE(zs, (width > prec ? width : prec) + 16);
			plen = sprintf(zs->str+zs->len, zstr_str(fmt), va_arg(ap, double));
			zs->len+=plen;
			break;
		case 'm':
			zstr_strncat(zs, strerror(errno), -1);
			break;
		case 'p':
			ZSTR_SPACE(zs, (width > prec ? width : prec) + 16);
			plen=sprintf((char *) zs->str+zs->len, zstr_str(fmt), va_arg(ap, char *));
			zs->len += plen;
			break;
		default:
			zmsg_error("zstr_vsprintf_append: unknown format type: %c", *cp);
			break;
		}
	}
	zstr_terminate(zs);
	return (zstr_len(zs)-old_len);
}

inline int zstr_vsprintf(ZSTR *zs, char *format, va_list ap) {
	zstr_reset(zs);
	return zstr_vsprintf_append(zs, format, ap);
}

int zstr_sprintf(ZSTR *zs, char *fmt, ...){
	va_list ap;
	int i;

	zstr_reset(zs);
	va_start(ap, fmt);
	i = zstr_vsprintf_append(zs, fmt, ap);
	va_end(ap);
	return i;
}

inline int zstr_vsnprintf(ZSTR *zs, int len, char *fmt, va_list ap){
	zstr_reset(zs);
	return zstr_vsnprintf_append(zs, len, fmt, ap);
}

int zstr_snprintf(ZSTR *zs, int len, char *fmt, ...){
	va_list ap;
	int i;

	zstr_reset(zs);
	va_start(ap, fmt);
	i = zstr_vsnprintf_append(zs, len, fmt, ap);
	va_end(ap);
	return i;
}

ZSTR *zstr_fconcatenate(char *fmt, ...){
	va_list ap;

	if(___general_concatenate==NULL)
		___general_concatenate=zstr_create(128);
	zstr_reset(___general_concatenate);
	va_start(ap, fmt);
	zstr_vsprintf(___general_concatenate, fmt, ap);
	va_end(ap);
	return ___general_concatenate;
}

ZSTR *zstr_concatenate(char *src0, ...){
	va_list ap;
	char *arg;

	if(___general_concatenate==NULL)
		___general_concatenate=zstr_create(128);
	zstr_reset(___general_concatenate);
	zstr_strcat(___general_concatenate, src0);
	va_start(ap, src0);
	while ((arg = va_arg(ap, char *)) != 0)
		zstr_strcat(___general_concatenate, arg);
	va_end(ap);
	zstr_terminate(___general_concatenate);
	return ___general_concatenate;
}
