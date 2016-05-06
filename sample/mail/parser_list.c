/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-01-07
 * ================================
 */

#include "libzc.h"

int main(int argc, char **argv)
{
    zmail_parser_t *parser;
    zmmap_reader reader;
    char buf[1024];
    char *p;
    FILE *fp;
    int count = 0;

    fprintf(stderr, "\n");
    sprintf(buf, "find %s -type f", argv[1]);
    fp = popen(buf, "r");
    while (fgets(buf, 1000, fp)) {
        p = strchr(buf, '\n');
        if (p) {
            *p = 0;
        } else {
            continue;
        }
        fprintf(stderr, "\r%d", count++);
        printf("FN: %s\n", buf);
        zmmap_reader_init(&reader, buf);
        parser = zmail_parser_create(reader.data, reader.len);
        zmail_parser_run(parser);
        zmail_parser_show(parser);
        zmail_parser_free(parser);
        zmmap_reader_fini(&reader);
    }
    fclose(fp);

    return 0;
}
