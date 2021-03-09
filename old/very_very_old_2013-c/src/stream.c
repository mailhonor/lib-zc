#include "zyc.h"

static int zstream_rbuf_ready(ZSTREAM *zsm);
static int zstream_wbuf_ready(ZSTREAM *zsm);

ZSTREAM * zstream_fdopen(int fd, int timeout){
	ZSTREAM *zsm;

	zsm=(ZSTREAM *)z_malloc(sizeof(ZSTREAM));
	zsm->fd=fd;
	zsm->flag=0;
	zsm->error=0;
	zsm->timeout=timeout;
	zsm->rbuf_start=0;
	zsm->rbuf_len=0;
	zsm->wbuf_len=0;
	return zsm;
}

int zstream_fdclose(ZSTREAM *zsm){
	int fd;

	fd=zsm->fd;
	z_free(zsm);
	return fd;
}

int zstream_fclose(ZSTREAM *zsm){
	zstream_fflush(zsm);
	close(zsm->fd);
	z_free(zsm);
	return 0;
}

int zstream_fflush(ZSTREAM *zsm){
	if(zsm->wbuf_len){
		return zstream_wbuf_ready(zsm);
	}
	return 0;
}

static int zstream_rbuf_ready(ZSTREAM *zsm){
	int ret;
	
	/* It is may be a good idea automatically to flush wirte-buf before to read any data, 
	 * so that we need not to flush write-buf ervey time by hand.
	 */
#if 0
	ZSTREAM_FFLUSH(zsm);
#endif 
	ret=zio_timed_read(zsm->fd, zsm->rbuf, ZSTREAM_RBUF_SIZE, zsm->timeout);
	/*
	 * We ignore error type which may be timeout, error, or eof.
	 */
	if(ret<0){
		/* timeout or error */
		zsm->error=ZSTREAM_EOF;
		return ZSTREAM_EOF;
	}
	if(ret == 0){
		/* end of stream */
		zsm->error=ZSTREAM_EOF;
		return ZSTREAM_EOF;
	}
	zsm->rbuf_start=1;
	zsm->rbuf_len=ret;
	return (*(zsm->rbuf));
}

inline int zstream_get_char(ZSTREAM *zsm){
	if(zsm->rbuf_start==zsm->rbuf_len){
		return zstream_rbuf_ready(zsm);
	}
	return (int)(zsm->rbuf[zsm->rbuf_start++]);
}

int zstream_read(ZSTREAM *zsm, ZSTR *zbuf, int len){
	int rc;
	int left;

	zstr_reset(zbuf);
	left=len;
	for(left=len;left;left--){
		rc=ZSTREAM_GET_CHAR(zsm);
		if(rc==ZSTREAM_EOF){
			break;
		}
		ZSTR_PUT(zbuf, rc);
	}
	zstr_terminate(zbuf);
	return (len-left);
}

int zstream_get_delimiter(ZSTREAM *zsm, ZSTR *zs, char delimiter){
	int c;

	zstr_reset(zs);
	while((c=zstream_get_char(zsm))!=ZSTREAM_EOF){
		ZSTR_PUT(zs, c);
		if(c == delimiter){
		       	break;
		}
	}
	zstr_terminate(zs);
	return (c==delimiter?1:(zstr_len(zs)>0?0:ZSTREAM_EOF));
}

int zstream_get_nodelimiter(ZSTREAM *zsm, ZSTR *zs, char delimiter){
	int c;

	zstr_reset(zs);
	while((c=zstream_get_char(zsm))!=ZSTREAM_EOF){
		if(c == delimiter){
		       	break;
		}
		ZSTR_PUT(zs, c);
	}
	zstr_terminate(zs);
	return (c==delimiter?1:(zstr_len(zs)>0?0:ZSTREAM_EOF));
}

int zstream_get_delimiter_bound(ZSTREAM *zsm, ZSTR *zs, char delimiter, int bound){
	int c;

#if 1
	/* var init for avoid wanring*/
	c=0;
#endif
	zstr_reset(zs);
	while(bound-->0 && (c=zstream_get_char(zsm))!=ZSTREAM_EOF){
		ZSTR_PUT(zs, c);
		if(c == delimiter){
		       	break;
		}
	}
	zstr_terminate(zs);
	return (c==delimiter?1:(zstr_len(zs)>0?0:ZSTREAM_EOF));
}

int zstream_get_nodelimiter_bound(ZSTREAM *zsm, ZSTR *zs, char delimiter, int bound){
	int c;

#if 1
	/* var init for avoid wanring*/
	c=0;
#endif
	zstr_reset(zs);
	while(bound-->0 && (c=zstream_get_char(zsm))!=ZSTREAM_EOF){
		if(c == delimiter){
		       	break;
		}
		ZSTR_PUT(zs, c);
	}
	zstr_terminate(zs);
	return (c==delimiter?1:(zstr_len(zs)>0?0:ZSTREAM_EOF));
}

static int zstream_wbuf_ready(ZSTREAM *zsm){
	int ret, rc;

	rc=zsm->wbuf[zsm->wbuf_len-1];
	ret = zio_timed_strict_write(zsm->fd, zsm->wbuf, zsm->wbuf_len, zsm->timeout);
	if(ret != zsm->wbuf_len){
		zsm->error=ZSTREAM_EOF;
		return(-1);
	}
	zsm->wbuf_len=0;
	return rc;
}
inline int zstream_put_char(ZSTREAM *zsm, int inch){
	zsm->wbuf[zsm->wbuf_len++]=(char)inch;
	if(zsm->wbuf_len == ZSTREAM_WBUF_SIZE){
		return zstream_wbuf_ready(zsm);
	}
	return (inch);
}

int zstream_fputs(char *buf, ZSTREAM *zsm){
	int ch;

	while((ch=*buf++)!=0){
		if(ZSTREAM_PUT_CHAR(zsm, ch)==ZSTREAM_EOF)
			return (ZSTREAM_EOF);
	}
	return 0;
}

int zstream_write(ZSTREAM *zsm, char *buf, int len){
	int left, pc;
#if 0
	/*
	 * For buf-len is big situation, directly write the buf.
	 */
	int ret, dw;
	dw=0;
	left=zsm->wbuf_len+len;
	if(zsm->wbuf_len==0){
		if(len>ZSTREAM_WBUF_SIZE){
			dw=1;
		}
	}else if(left>2*ZSTREAM_WBUF_SIZE){
		dw=1;
	}
	if(dw){
		if(zsm->wbuf_len){
			ret = zstream_wbuf_ready(zsm);
			if(ret <0){
				return ret;
			}
		}
		ret = zio_timed_strict_write(zsm->fd, buf, len, zsm->timeout);
		if(ret<len){
			zsm->error=ZSTREAM_EOF;
			return ret;
		}
		return len;
	}
#endif 
	left=len;
	while(left > 0){
		pc=ZSTREAM_PUT_CHAR(zsm, *buf++);
		if(pc<0){
			return (pc);
		}
		left--;
	}

	return (len);
}

int zstream_vfprintf(ZSTREAM *zsm, char *format, va_list ap) {
	static ZSTR *fmt=0, *fcache=0;
	unsigned char *cp;
	char *s;
	int slen;
	unsigned long_flag;
	int width, prec, ch;
	int plen, wlen;
	int errno_bak;

#define _ZSTREAM_PUT_CHAR(ch)	{if(ZSTREAM_PUT_CHAR(zsm, (ch))<0) return -1;wlen++;}

	wlen = 0;
	errno_bak=errno;

	/* FIXME
	 * The func is generally used for very many conditions, so this code may be use in confusion. example;
	 * sigal handler.
	 * All I says is followed "fmt".
	 */
	if (fmt == 0)
		fmt = zstr_create(128);

	if (fcache == 0)
		fcache = zstr_create(1024);

	for (cp = (unsigned char *) format; *cp; cp++) {
		if (*cp != '%') {
			_ZSTREAM_PUT_CHAR(*cp);
			continue;
		}
		if (cp[1] == '%') {
			_ZSTREAM_PUT_CHAR(*cp++);
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
			zmsg_warn("zstream_vfprintf: bad width %d in %.50s",width, format);
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
			slen=strlen(s);
			if (prec > 0 || (width > 0 && width > slen)) {
				zstr_sprintf(fcache, zstr_str(fmt), s);
				plen=zstream_write(zsm, zstr_str(fcache), zstr_len(fcache));
				if(plen < zstr_len(fcache)){
					return -1;
				}
				wlen += plen;
			} else {
				plen=zstream_write(zsm, s, slen);
				if(plen < slen){
					return -1;
				}
				wlen += plen;
			}
			break;
		case 'c':
		case 'd':
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			//ZSTR_SPACE(zs, (width > prec ? width : prec) + 16);
			if (long_flag)
				zstr_sprintf(fcache, zstr_str(fmt), va_arg(ap, long));
			else
				zstr_sprintf(fcache, zstr_str(fmt), va_arg(ap, int));
			plen=zstream_write(zsm, zstr_str(fcache), zstr_len(fcache));
			if(plen < zstr_len(fcache)){
				return -1;
			}
			wlen += plen;
			break;
		case 'e':	
		case 'f':
		case 'g':
			//ZSTR_SPACE(zs, (width > prec ? width : prec) + 16);
			zstr_sprintf(fcache, zstr_str(fmt), va_arg(ap, double));
			plen=zstream_write(zsm, zstr_str(fcache), zstr_len(fcache));
			if(plen < zstr_len(fcache)){
				return -1;
			}
			wlen += plen;
			break;
		case 'm':
			s=strerror(errno_bak);
			slen=strlen(s);
			plen=zstream_write(zsm, s, slen);
			if(plen < slen){
				return -1;
			}
			wlen += plen;
			break;
		case 'p':
			//ZSTR_SPACE(zs, (width > prec ? width : prec) + 16);
			zstr_sprintf(fcache, zstr_str(fmt), va_arg(ap, char *));
			plen=zstream_write(zsm, zstr_str(fcache), zstr_len(fcache));
			if(plen < zstr_len(fcache)){
				return -1;
			}
			wlen += plen;
			break;
		default:
			zmsg_error("zstream_vfprintf: unknown format type: %c", *cp);
			break;
		}
	}
	return (wlen);
}

int zstream_fprintf(ZSTREAM *zsm, char *fmt, ...){
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = zstream_vfprintf(zsm, fmt, ap);
	va_end(ap);
	return i;
}
