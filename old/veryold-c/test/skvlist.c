#include "zc.h"
#include "test_lib.h"

static ZDICT *dict;
static ZSKVLIST *kv;
static ZDICT_NODE *n;
static char ret_buf[1024];

int _reload(void)
{
	zskvlist_load(kv);
	strcpy(ret_buf, "reload OK");

	return 0;
}

int _put(char *buf)
{
	char *name, *value, *pe;

	name = buf;

	pe = strchr(buf, '=');
	if (!pe) {
		strcpy(ret_buf, "err: usage: \"put somekey=\" or \"put somekey=somevalue\"");
		return 0;
	}
	*pe = 0;
	value = pe + 1;

	name = zstr_trim(name);
	value = zstr_trim(value);
	if (*value) {
		zskvlist_enter(kv, name, value);
	} else {
		zskvlist_delete(kv, name);
	}

	strcpy(ret_buf, "OK");

	return 0;
}

int _list(void)
{
	dict = kv->dict;
	for (n = zdict_first(dict); n; n = zdict_next(n)) {
		fprintf(stdout, "%s = %s\n", n->key, (char *)(n->value));
	}

	return 0;
}

int main(int argc, char **argv)
{
	char *dn = "data/skvlist";
	char buf[1024];

	kv = zskvlist_create(dn);
	zskvlist_load(kv);

	int _exit = 0;
	while (fgets(buf, 1000, stdin)) {
		ret_buf[0] = 0;
		if (!strncasecmp(buf, "exit", 4)) {
			strcpy(ret_buf, "BYE");
			_exit = 1;
		} else if (!strncasecmp(buf, "reload", 6)) {
			_reload();
		} else if (!strncasecmp(buf, "put", 3)) {
			_put(buf + 3);
		} else if (!strncasecmp(buf, "list", 4)) {
			_list();
		} else {
			strcpy(ret_buf, "cmd noneexists");
		}
		fprintf(stdout, "%s\n", ret_buf);
		if (_exit) {
			break;
		}
	}

	return 0;
}
