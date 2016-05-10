/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-04-25
 * ================================
 */
#include "libzc.h"

static inline int utf8_len(char *buf, int len)
{
    unsigned char *ptr;
    int ret;
    ptr = (unsigned char *)buf;
    if (((*ptr) <= 0x7F)) {
        ret = 1;
    } else if (((*ptr) & 0xF0) == 0xF0) {
        ret = 4;
    } else if (((*ptr) & 0xE0) == 0xE0) {
        ret = 3;
    } else if (((*ptr) & 0xC0) == 0xC0) {
        ret = 2;
    } else {
        ret = 5;
    }

    return ret;
}

int main(int argc, char **argv)
{
    zmmap_reader freader;
    int count_list[1024000];
    int count_i = 0;
    int i;
    char *ps;
    zgrid_t *ws = zgrid_create();

    zmmap_reader_init(&freader, argv[1]);

    ps = freader.data;
    for (i = 0; i < freader.len;) {
        char word[128];
        ps = freader.data + i;
        int ulen = utf8_len(ps, 0);
        i = i + ulen;
        if (ulen == 1) {
            continue;
        }
        memcpy(word, ps, ulen);
        word[ulen] = 0;
        int *count;
        if (!zgrid_lookup(ws, word, (char **)&count)) {
            count = count_list + count_i;
            *count = 1;
            count_i++;
            zgrid_add(ws, word, count, 0);
        } else {
            *count = *count + 1;
        }
    }
    zgrid_node_t *node;
    ZGRID_WALK_BEGIN(ws, node) {
        int count = *((int *)zgrid_value(node));
        int i = 1;
        while (i-1 < count)
        {
            i = i * 2;
        }
        if (i > 128)
        {
            i=128;
        }

        printf("%s%c", zgrid_key(node), i);
    }
    ZGRID_WALK_END;

    return 0;
}
