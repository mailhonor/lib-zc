#include "zyc.h"

int zattr_fprint(ZSTREAM *zsm, ...){
	va_list ap;
	int i;
	va_start(ap, zsm);
	i = zattr_vfprint(zsm, ap);
	va_end(ap);
	return i;
}

int zattr_fscan(ZSTREAM *zsm, ...){
	va_list ap;
	int i;
	va_start(ap, zsm);
	i = zattr_vfscan(zsm, ap);
	va_end(ap);
	return i;
}

int zattr_vfprint(ZSTREAM *zsm, va_list ap){
	int i, type, c;
	int int_val;
	long long_val;
	float float_val;
	double double_val;
	char *str_val;
	ZARGV *zargv_val;
	char *data_val;
	int data_len;
	ZATTR_PRINT_FN func_val;
	char *func_arg;

	while((type=va_arg(ap, int))!=ZATTR_END){
		switch(type){
		case ZATTR_CHAR:
			c = ((char)(va_arg(ap, int)));
			ZSTREAM_PUT(zsm, c);
			break;
		case ZATTR_INT:
			int_val=va_arg(ap, int);
			zstream_write(zsm, (char *)&int_val, sizeof(int));
			break;
		case ZATTR_LONG:
			long_val=va_arg(ap, long);
			zstream_write(zsm, (char *)&long_val, sizeof(long));
			break;
		case ZATTR_FLOAT:
			float_val=(float)va_arg(ap, double);
			zstream_write(zsm, (char *)&float_val, sizeof(float));
			break;
		case ZATTR_DOUBLE:
			double_val=va_arg(ap, double);
			zstream_write(zsm, (char *)&double_val, sizeof(double));
			break;
		case ZATTR_STR:
			str_val=va_arg(ap, char *);
			zstream_fputs(str_val, zsm);
			ZSTREAM_PUT(zsm, '\0');
			break;
		case ZATTR_DATA:
			data_val=va_arg(ap, char *);
			data_len=va_arg(ap, int);
			zstream_write(zsm, (char *)&data_len, sizeof(int));
			zstream_write(zsm, data_val, data_len);
			break;
		case ZATTR_ZARGV:
			zargv_val=va_arg(ap, ZARGV *);
			data_len=ZARGV_LEN(zargv_val);
			zstream_write(zsm, (char *)&data_len, sizeof(int));
			for(i=0;i<data_len;i++){
				zstream_fputs(zargv_val->argv[i], zsm);
				ZSTREAM_PUT(zsm, '\0');
			}
			break;
		case ZATTR_FUNC:
			func_val=va_arg(ap, ZATTR_PRINT_FN);
			func_arg=va_arg(ap, char *);
			if(func_val){
				func_val(zsm, func_arg);
			}
			break;
		default:
			zmsg_error("zattr_vfprint: unknown type code: %d", type);
		}
		if(zstream_ferror(zsm))return ZSTREAM_EOF;
	}
	return(zstream_ferror(zsm));
}

int zattr_vfscan(ZSTREAM *zsm, va_list ap){
	int i, type, c;
	char *char_val;
	int *int_val;
	long *long_val;
	float *float_val;
	double *double_val;
	ZSTR *zstr_val;
	ZARGV *zargv_val;
	int zargv_lenlen;
	int data_len;
	ZATTR_PRINT_FN func_val;
	char *func_arg;

	static ZSTR *ztmp = 0;
	if(ztmp==0){
		ztmp=zstr_create(128);
	}
#define	_read_int()	(zstream_read(zsm, ztmp, sizeof(int)),*((int *)(zstr_str(ztmp))))
	while((type=va_arg(ap, int))!=ZATTR_END){
		switch(type){
		/*
		 * It is must to handle that EOF.
		 */
		case ZATTR_CHAR:
			char_val=va_arg(ap, char *);
			*char_val=ZSTREAM_GET(zsm);
			break;
		case ZATTR_INT:
			int_val=va_arg(ap, int *);
			*int_val=_read_int();
			break;
		case ZATTR_LONG:
			long_val=va_arg(ap, long *);
			*long_val=*((long *)(zstr_str(ztmp)));
			break;
		case ZATTR_FLOAT:
			float_val=va_arg(ap, float *);
			zstream_read(zsm, ztmp, sizeof(float));
			*float_val=*((float *)(zstr_str(ztmp)));
			break;
		case ZATTR_DOUBLE:
			double_val=va_arg(ap, double *);
			zstream_read(zsm, ztmp, sizeof(double));
			*double_val=(*((double *)(zstr_str(ztmp))));
			break;
		case ZATTR_STR:
			zstr_val=va_arg(ap, ZSTR *);
			zstr_reset(zstr_val);
			while((c=ZSTREAM_GET(zsm))!=0 && c!=-1){
				ZSTR_PUT(zstr_val, c);
			}
			zstr_terminate(zstr_val);
			break;
		case ZATTR_DATA:
			zstr_val=va_arg(ap, ZSTR *);
			zstr_reset(zstr_val);
			data_len=_read_int();
			if(zstream_ferror(zsm)) break;
			zstream_read(zsm, zstr_val, data_len);
			zstr_terminate(zstr_val);
			break;
		case ZATTR_ZARGV:
			zargv_val=va_arg(ap, ZARGV *);
			zargv_lenlen=_read_int();
			if(zstream_ferror(zsm)) break;
			for(i=0;i<zargv_lenlen;i++){
				zstr_reset(ztmp);
				while((c=ZSTREAM_GET(zsm))!=0 && c!=-1){
					ZSTR_PUT(ztmp, c);
				}
				zstr_terminate(ztmp);
				if(zstream_ferror(zsm)) break;
				zargv_addn(zargv_val, zstr_str(ztmp), zstr_len(ztmp));
			}
			if(zstream_ferror(zsm)) break;
			break;
		case ZATTR_FUNC:
			func_val=va_arg(ap, ZATTR_PRINT_FN);
			func_arg=va_arg(ap, char *);
			if(func_val){
				func_val(zsm, func_arg);
			}
			break;
		default:
			zmsg_error("zattr_vfscan: unknown type code: %d", type);
		}
		if(zstream_ferror(zsm))return ZSTREAM_EOF;
	}
	return(zstream_ferror(zsm));
}
