/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-09-07
 * ================================
 */

#include "../../zc.h"


static void mime_header_line_walk_test(const char *filename, void (*walk_fn)(zbuf_t *line))
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR open %s\n", filename);
        exit(1);
    }
    char buf[102400+10];
    zbuf_t *line = zbuf_create(0);
    zbuf_t *result = zbuf_create(0);

    while(1) {
        if (ferror(fp) || feof(fp)) {
            break;
        }
        zbuf_reset(line);
        int have_data = 0;
        while(1){
            size_t last_seek = ftell(fp);
            if (!fgets(buf, 102400, fp)) {
                break;
            }
            if (buf[0] == ' ' || buf[0] == '\t') {
                zbuf_puts(line, buf+1);
                zbuf_trim_right_rn(line);
                continue;
            }
            if (have_data) {
                fseek(fp, last_seek, SEEK_SET);
                break;
            } else {
                zbuf_puts(line, buf);
                zbuf_trim_right_rn(line);
                have_data = 1;
            }
        }
        if(walk_fn) {
            walk_fn(line);
        }
    }

    zbuf_free(line);
    zbuf_free(result);
    fclose(fp);
}
