/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-11
 * ================================
 */

#include "libzc.h"
#include <ctype.h>

char hunman_buf[100];
char *hunman_size2(long a)
{
    char buf[300], *p = buf, ch;
    int len, m, i;
    int tl = 0;

    hunman_buf[0] = 0;
    sprintf(buf, "%ld", a);
    len =strlen(buf);
    m = len%3;

    while(1)
    {
        for(i=0;i<m;i++)
        {
            ch = *p++;
            if(ch=='\0')
            {
                goto over;
            }
            hunman_buf[tl++] = ch;
        }
        hunman_buf[tl++] = ',';
        m = 3;
    }

over:
    hunman_buf[tl] = 0;
    len = strlen(hunman_buf);
    if(len > 0)
    {
        if(hunman_buf[len-1] == ',')
        {
            hunman_buf[len -1] = 0;
        }
    }
    if(hunman_buf[0] == ',')
    {
        return hunman_buf + 1;
    }

    return hunman_buf;
}

int main(int argc, char **argv)
{
    zmail_parser_t *parser;
    char *eml_fn;
    int times, i;
    zmmap_reader reader;
    char *eml_data;
    long t;
    int size;

	if(argc != 3)
	{
		printf("USAGE: %s times eml_fn\n", argv[0]);
		return 0;
	}
    zmail_parser_only_test_parse = 1;

    times = atoi(argv[1]);
    eml_fn = argv[2];

    zmmap_reader_init(&reader, eml_fn);
    size = reader.len;
    eml_data = zmemdup(reader.data, reader.len);
    zmmap_reader_fini(&reader);

    printf("eml  : %s\n", eml_fn);
    printf("size : %d(byte)\n", size);
    printf("loop : %d\n", times);
    printf("total: %s(byte)\n", hunman_size2((long)size * times));
    t = ztimeout_set(0);
    for (i = 0; i < times; i++)
    {
        parser = zmail_parser_create(eml_data, size);
        zmail_parser_run(parser);
        zmail_parser_free(parser);
    }
    t = ztimeout_set(0) - t;
    printf("time : %ld.%03ld(second)\n", t/1000, t%1000);
    printf("%%second  : %s(byte)\n", hunman_size2((long)(((long)size * times)/((1.0 * t)/1000))));
    zfree(eml_data);


    return 0;
}
