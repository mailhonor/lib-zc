/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-09-29
 * ================================
 */

#include "zc.h"


/*
 * 1字节 0xxxxxxx 
 * 2字节 110xxxxx 10xxxxxx 
 * 3字节 1110xxxx 10xxxxxx 10xxxxxx 
 * 4字节 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 
 * 5字节 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
 * 6字节 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
 */

static void show_2()
{
    unsigned char cb[3];
    int i, j;
    cb[2] = 0;

    for (i = 0XC0; i < 0XE0; i ++) {
        cb[0] = i;
        for (j = 0X80; j < 0XC0; j ++) {
            cb[1] = j;
            printf("0X%02X%02X %s\n", i, j, cb);
        }
    }
}

static void show_3()
{
    unsigned char cb[4];
    int i, j, k;
    cb[3] = 0;

    for (i = 0XE0; i < 0XF0; i ++) {
        cb[0] = i;
        for (j = 0X80; j < 0XC0; j ++) {
            cb[1] = j;
            for (k = 0X80; k < 0XC0; k ++) {
                cb[2] = k;
                printf("0X%02X%02X%02X %s\n", i, j, k, cb);
            }
        }
    }
}

static void show_4()
{
    unsigned char cb[5];
    int i, j, k, l;
    cb[4] = 0;

    for (i = 0XF0; i < 0XF8; i ++) {
        cb[0] = i;
        for (j = 0X80; j < 0XC0; j ++) {
            cb[1] = j;
            for (k = 0X80; k < 0XC0; k ++) {
                cb[2] = k;
                for (l = 0X80; l < 0XC0; l ++) {
                    cb[3] = l;
                    printf("0X%02X%02X%02X%02X %s\n", i, j, k, l, cb);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    show_2();
    show_3();
    show_4();
    return 0;
}

