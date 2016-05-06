/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-29
 * ================================
 */

#include "libzc.h"

static void _get_logic_line(FILE * fp, char *filename, zbuf_t * logic_line, int *line_number)
{
    char line_buf[102400];
    char *np;
    int len;

    zbuf_reset(logic_line);
    while (1) {
        *line_number = *line_number + 1;
        if (!fgets(line_buf, 102400, fp)) {
            return;
        }
        np = ztrim(line_buf);
        if ((*np == 0)) {
            if (zbuf_len(logic_line)) {
                return;
            }
            continue;
        }
        if (*np == '#') {
            continue;
        }
        len = strlen(np);
        if (np[len - 1] == '\\') {
            zbuf_memcat(logic_line, np, len - 1);
            continue;
        } else {
            zbuf_memcat(logic_line, np, len);
            return;
        }
    }
}

static int inline _parse_line(char *filename, char *line_buf, int line_number, char **name, char **value)
{
    char *np;
    char *vp;

    np = ztrim(line_buf);
    if (*np == '#') {
        return 0;
    }

    vp = strchr(np, '=');
    if (!vp) {
        *name = ztrim(np);
        *value = "";
        zdebug("zconfig_load: missing '=' at line %d, file %s", line_number, filename);
        return 1;
    }
    *vp = 0;
    vp++;

    *name = ztrim(np);
    *value = ztrim(vp);

    return 1;
}

int zconfig_load(zconfig_t * cf, char *filename)
{
    int ret;
    FILE *fp;
    zbuf_t *logic_line;
    int line_number = 0;
    char *name, *value;

    fp = fopen(filename, "r");
    if (!fp) {
        zerror("zconfig_load: fopen %s error (%m)", filename);
        return -1;
    }

    logic_line = zbuf_create(102400);

    while (1) {
        _get_logic_line(fp, filename, logic_line, &line_number);
        if (zbuf_len(logic_line) == 0) {
            break;
        }
        zbuf_terminate(logic_line);

        name = value = 0;
        ret = _parse_line(filename, zbuf_data(logic_line), line_number, &name, &value);
        if (ret) {
            zconfig_add(cf, name, value);
        }
    }
    zbuf_free(logic_line);
    fclose(fp);

    return 0;
}
