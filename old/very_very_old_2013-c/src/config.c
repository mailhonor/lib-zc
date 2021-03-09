#include "zyc.h"

#define SKIP(start, var, cond)  for (var = start; *var && (cond); var++);

ZCONFIG *z_global_config=0;

void zconfig_show(ZCONFIG *zc){
	ZHASH_NODE **nl, **bn;

	bn=zhash_list(zc);
	for(nl=bn; *nl; nl++){
		printf("%s = %s\n", (*nl)->key, (*nl)->value);
	}
	z_free(bn);
}

static int zconfig_load_loop(ZCONFIG *zconfig, char *path, int flag, ZHASH *config_files){
	FILE *cfp;
	ZSTR *zline, *zline_tmp;
	char *serr, *p, *name, *value, *ovalue, *npath;
	int sret, line_number;

	if(zhash_find(config_files, path, 0)){
		zmsg_warning("config file repeat include");
		return 0;
	}
	zhash_enter(config_files, path, 0);
	zline=zstr_create(1024);
	zline_tmp=zstr_create(1024);
	line_number=0;
	cfp=fopen(path, "r");
	if(cfp==0){
		zmsg_fatal("zconfig_load: unable load config: %s error: %m", path);
	}
	zmsg_debug("load config: %s", path);
	while(!feof(cfp)){
		serr="";
		z_fget_delimiter(cfp, zline, '\n');
		line_number++;
		zstr_strcpy(zline_tmp, zstr_str(zline));
		SKIP(zstr_str(zline), p, isspace(*p));
		if(*p=='#' || *p==0){
			continue;
		}
		sret=zconfig_split_nameval(zstr_str(zline_tmp), &name, &value);
		if(sret == -2){
			serr="missing =";
			zmsg_fatal("config: %s, line: %d, error: %s", path, line_number, serr);
		}
		if(!sret && *name==0){
			sret=-1;
			serr="missing attribute name";
		}
		if(sret){
			zmsg_fatal("config: %s, line: %d, error: %s", path, line_number, serr);
			return -1;
		}
		if(!strcasecmp(name, "zinclude")){
			npath=z_pathjoin(path, value);
			if(npath==0){
				zmsg_fatal("config file: (%s join %s) none exist", path, value);
			}
			npath=z_strdup(npath);
			zconfig_load_loop(zconfig, npath, flag, config_files);
			z_free(npath);
		}else{
			zhash_enter_unique(zconfig, name, z_strdup(value), &ovalue);
			if(ovalue){
				z_free(ovalue);
			}
		}
	}
	fclose(cfp);
	zstr_free(zline);
	zstr_free(zline_tmp);
	return 0;
}

int zconfig_load(ZCONFIG *zconfig, char *path, int flag){
	ZHASH *config_files;
	int ret;
	char *npath;

	config_files=zhash_create(100);
	npath=z_pathjoin(path, 0);
	if(npath==0){
		zmsg_fatal("config file:%s none exist", path);
	}
	npath=z_strdup(npath);
	ret = zconfig_load_loop(zconfig, npath, flag, config_files);
	zhash_free(config_files,0,0);
	z_free(npath);
	z_pathjoin(0,0);
	return ret;
}

ZCONFIG *zconfig_create(void){
	return zhash_create(128);
}

static void zconfig_free_fn(char *v, char *ptr){
	z_free(v);
}

void zconfig_free(ZCONFIG *zc){
	zhash_free(zc, zconfig_free_fn, 0);
}

int zconfig_split_nameval(char *buf, char **name, char **value){
	char *np,*vp,*cp,*ep;
	int len;

	SKIP(buf, np, isspace(*np));
	if(*np==0){
		return (-1);
	}
	SKIP(np, ep, (!isspace(*ep) && *ep != '='));
	SKIP(ep, cp, isspace(*cp));
	if(*cp !='='){
		return (-2);
	}
	*ep=0;
	*name=np;

	cp++;
	SKIP(cp, vp, isspace(*vp));

	len=strlen(vp);
	np=vp+(len-1);
	while(np>=vp){
		if(isspace(*np)){
			*np=0;
			np--;
		}else{
			break;
		}
	}
	*value=vp;
	return 0;
}

void zconfig_update(ZCONFIG *zc, char *name, char *value){
	zhash_enter_unique(zc, name, z_strdup(value?value:""), 0);
}

char *zconfig_get_str(ZCONFIG *zc, char *name, char *def){
	char *value;

	if(zc == 0){ 
		return def;
	}
	if(zhash_find(zc, name, &value)){
		return value;
	}
	return def;
}

int zconfig_get_bool(ZCONFIG *zc, char *name, int def){
	char *value;

	value =zconfig_get_str(zc, name, 0);

	if(value==0 || *value==0)return def;

	if(!strcasecmp(value, "1") || !strcasecmp(value, "y") || !strcasecmp(value, "yes") || !strcasecmp(value, "true"))
		return 1;
	if(!strcasecmp(value, "0") || !strcasecmp(value, "n") || !strcasecmp(value, "no") || !strcasecmp(value, "false"))
		return 0;
	return def;
}

int zconfig_get_int(ZCONFIG *zc, char *name, int def){
	char *value;

	value =zconfig_get_str(zc, name, 0);

	if(value==0 || *value==0)return def;
	
	return atoi(value);
}

int zconfig_get_time(ZCONFIG *zc, char *name, char *def){
	char unit,junk, *value;
	int intval;

	value =zconfig_get_str(zc, name, 0);
	
	if(value==0 || *value==0) value=def;

	switch(sscanf(value, "%d%c%c", &intval, &unit, &junk)){
	case 1:
		unit='s';
	case 2:
		if(intval <0) return 0;
		switch(tolower(unit)){
		case 'w':
			return (intval * (7 * 24 * 3600));
		case 'd':
			return (intval * (24 * 3600));
		case 'h':
			return (intval * 3600);
		case 'm':
			return (intval * 60);
		case 's':
			return (intval);
		}
	}
	return 0;
}

void zconfig_str_table(ZCONFIG *zc, ZCONFIG_STR *t){
	while(t->name){
		t->target[0]=zconfig_get_str(zc, t->name, t->defval);
		t++;
	}
}
void zconfig_int_table(ZCONFIG *zc, ZCONFIG_INT *t){
	while(t->name){
		t->target[0]=zconfig_get_int(zc, t->name, t->defval);
		t++;
	}
}
void zconfig_bool_table(ZCONFIG *zc, ZCONFIG_BOOL *t){
	while(t->name){
		t->target[0]=zconfig_get_bool(zc, t->name, t->defval);
		t++;
	}
}
void zconfig_time_table(ZCONFIG *zc, ZCONFIG_TIME *t){
	while(t->name){
		t->target[0]=zconfig_get_time(zc, t->name, t->defval);
		t++;
	}
}
