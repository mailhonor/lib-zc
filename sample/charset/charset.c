/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "libzc.h"

int main(int argc, char **argv)
{
    char *fn;
    zmmap_reader reader;
    char charset[128];

    zvar_progname = argv[0];
    if (argc != 2)
    {
        printf("USAGE: %s filename\n", zvar_progname);
        exit(1);
    }
    fn = argv[1];

    if (zmmap_reader_init(&reader, fn) < 0)
    {
        printf("open %s:%m", fn);
        exit(1);
    }

    if (zcharset_detect_chinese(reader.data, reader.len, charset) < 0)
    {
        printf("detect error\n");
    }
    else if (!*charset)
    {
        printf("not found, ASCII? or none-chinese language\n");
    }
    else
    {
        printf("%s\n", charset);
    }

    zmmap_reader_fini(&reader);

    return 0;
}
