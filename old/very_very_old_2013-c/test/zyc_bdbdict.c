#include <zyc.h>
#include <db.h>
#include <signal.h>

char *out_filename=0;
char *input_filename=0;
char *type=0;

int main(int argc, char **argv){
	int ret;

	signal(SIGPIPE, SIG_IGN);
	zvar_program_name = argv[0];

	if(argc < 3 || argc> 4){
		fprintf(stderr, "USAGE: %s hash/btree input_text_file [out_bdb_file]\n"
				"\tDefault value of out_bdb_file is input_text_file.db\n", zvar_program_name);
		return 1;
	}

	type = argv[1];
	input_filename = argv[2];
	if(argc==4){
		out_filename=argv[3];
	}else{
		out_filename=z_strdup(ZSTR_STR(zstr_concatenate(input_filename, ".cf", 0)));
	}

	FILE *fp;
	char rbuf[10240];
	ZARGV *zar;
	DB *db_p=NULL;
	DBTYPE dbtype;

	dbtype=DB_HASH;
	if(!strcasecmp(type, "btree")){
		dbtype=DB_BTREE;
	}

	unlink(out_filename);
	ret = db_create(&db_p, NULL, 0);
	ret = db_p->open(db_p, NULL, out_filename, NULL, dbtype, DB_CREATE, 0);
	if(ret){
		fprintf(stderr, "open db :%s :%m\n", out_filename);
		return 1;
	}

	zar =  zargv_create(5);
	fp = fopen(input_filename, "r");
	if(fp == 0){
		fprintf(stderr, "can not open :%s :%m", input_filename);
		return 1;
	}

	DBT key, data;
	while(!feof(fp)){
		if(!fgets(rbuf, 10240, fp))
			break;
		zargv_reset(zar);
		zargv_split_append(zar, rbuf, " \t\r\n");
		if(ZARGV_LEN(zar) < 2){
			continue;
		}

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));
		key.data = zar->argv[0];
		key.size = strlen(zar->argv[0]);
		data.data = zar->argv[1];
		data.size = strlen(zar->argv[1]);

		db_p->put(db_p, NULL, &key, &data, 0);
	}
	db_p->close(db_p, 0);
	return 0;
}

