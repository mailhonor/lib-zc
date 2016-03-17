/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-02-14
 * ================================
 */

#include "libzc.h"
#include <ctype.h>

int main(int argc, char **argv)
{
    zmail_parser_t *parser;
    char *eml_fn;
    zmmap_reader reader;
    zbuf_t *result;

    result = zbuf_create(1024000);

    eml_fn = argv[1];
    zmmap_reader_init(&reader, eml_fn);

    parser = zmail_parser_create(reader.data, reader.len);
    zmail_parser_run(parser);

    zmail_parser_show_json(parser, result);
    zbuf_terminate(result);
    printf("%s\n", ZBUF_DATA(result));

    zmail_parser_free(parser);
    zmmap_reader_fini(&reader);

    return 0;
}
