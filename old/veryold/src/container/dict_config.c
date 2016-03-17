#include "zc.h"

ZDICT *zvar_default_config = 0;

static int inline _parse_line(char *filename, char *line_buf, int line_number, char **name, char **value)
{
	char *np;
	char *vp;
	int len;

	np = zstr_trim(line_buf);
	if (*np == 0) {
		return 0;
	}
	if (*np == '#') {
		return 0;
	}

	if (*np == '[') {
		len = strlen(np);
		if (np[len - 1] == ']') {
			np[len - 1] = 0;
			np++;
			np = zstr_trim(np);
			*name = np;
			*value = 0;
			return 2;
		}
	}

	vp = strchr(np, '=');
	if (!vp) {
		zlog_warning("zdict_load_file: missing '=' at line %d, file %s", line_number, filename);
		return 0;
	}
	*vp = 0;
	vp++;

	*name = zstr_trim(np);
	*value = zstr_trim(vp);

	return 1;
}

void zdict_config_load_file(ZDICT * zd, char *filename)
{
	FILE *fp;
	char line_buf[102401];
	int line_number = 0, ret;
	char *name, *value;
	ZDICT *___czd = zd;

	fp = fopen(filename, "r");
	if (!fp) {
		zlog_warning("zdonfig_load_file: fopen %s error (%m)", filename);
		return;
	}
	while (fgets(line_buf, 102400, fp)) {
		line_number++;
		name = value = 0;
		ret = _parse_line(filename, line_buf, line_number, &name, &value);
		if (ret == 1) {
			zdict_enter_STR(___czd, name, value);
		} else if (ret == 2) {
			___czd = zdict_config_create_child(zd, name);
		}
	}
	fclose(fp);
}

ZDICT *zdict_config_get_child(ZDICT * zd, char *name)
{
	ZDICT *czd;
	char child_name[300];

	child_name[0] = '=';
	strncpy(child_name + 1, name, 256);

	if (zdict_lookup(zd, child_name, (char **)(&czd))) {
		return czd;
	}

	return 0;
}

ZDICT *zdict_config_create_child(ZDICT * zd, char *name)
{
	ZDICT *___czd;
	ZDICT_NODE *ndn, *odn;
	char child_name[300];

	child_name[0] = '=';
	strncpy(child_name + 1, name, 256);

	if ((ndn = zdict_enter(zd, child_name, 0, &odn))) {
		___czd = zdict_create();
		ZDICT_UPDATE_VALUE(ndn, ___czd);
	} else {
		___czd = (ZDICT *) (ZDICT_VALUE(odn));
	}

	return ___czd;
}

static void _free(ZDICT_NODE * dn, void *ctx)
{
	if (ZDICT_KEY(dn)[0] == '=') {
		zdict_config_free((ZDICT *) (ZDICT_VALUE(dn)));
	} else {
		zstr_free(ZDICT_VALUE(dn));
	}
}

void zdict_config_free(ZDICT * zd)
{
	zdict_free(zd, _free, 0);
}

static void ___zdict_config_debug(ZDICT * zd, int level)
{
	ZDICT *czd;
	ZDICT_NODE *zn;
	char *key, *value;
	int have = 0;

	for (zn = zdict_first(zd); zn; zn = zdict_next(zn)) {
		have = 1;
		key = ZDICT_KEY(zn);
		value = ZDICT_VALUE(zn);

		if (key[0] != '=') {
			zlog_info("%s%s = %s", (level ? "        " : ""), key, value);
		} else {
			zlog_info("[%s]", key + 1);
			czd = (ZDICT *) value;
			___zdict_config_debug(czd, level + 1);
		}
	}
	if (!have) {
		zlog_info("no key=value");
	}
}

void zdict_config_debug(ZDICT * zd)
{
	___zdict_config_debug(zd, 0);
}
