#include <zyc.h>
#include <signal.h>

char *proxy_daemon_dir=0;
char *maps_string=0;
char *query=0;

int usage(void){
	fprintf(stderr, "USAGE: \n"
			"%s [-proxy_daemon_dir proxy_daemon_dir] [-d or -v] -maps maps_strint -q query\n"
		       	"\t  It is must to define \"-proxydir\" when proxymap is used.\n"
		       	,zvar_program_name);
	exit(1);
}

int main(int argc, char **argv){
	int i, ret;
	char *arg;
	ZMAPS *maps;

	signal(SIGPIPE, SIG_IGN);

	zvar_program_name=argv[0];
	for(i=1;i<argc;i++){
		arg=argv[i];
		if(!strcmp(arg, "-proxy_daemon_dir")){
			proxy_daemon_dir = z_strdup(argv[i+1]);
			i++;
		}else if(!strcmp(arg, "-maps")){
			maps_string = z_strdup(argv[i+1]);
			i++;
		}else if(!strcmp(arg, "-q")){
			query = z_strdup(argv[i+1]);
			i++;
		}else if(!strcmp(arg, "-d")){
			zvar_debug=1;
		}else if(!strcmp(arg, "-v")){
			zvar_verbose=1;
		}else{
			usage();
		}
	}
	if(maps_string == 0 || query == 0){
		usage();
	}

	if(proxy_daemon_dir){
		zvar_dict_proxy_path=z_strdup(proxy_daemon_dir);
	}

	maps=zmaps_create(maps_string);
	if(maps==0){
		fprintf(stderr, "create maps error.");
		exit(1);
	}
	ret=zmaps_lookup(maps, query, ZMAPS_LOOKUP_RETURN_AFTER_ERROR);
	if(ret==0){
		fprintf(stdout, "N:\n");
	}else if(ret < 0){
		fprintf(stdout, "E: %s\n", ZSTR_STR(zvar_maps_result));
	}else{
		fprintf(stdout, "Y: [%s]\n", ZSTR_STR(zvar_maps_result));
	}
	return 0;
}

