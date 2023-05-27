/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2019-11-01
 * ================================
 */

#include "zc.h"

typedef struct TTT TTT;
struct TTT {
    TTT *prev_123;
    TTT *next_456;
    int a;
};

int main(int argc, char **argv)
{
    TTT *head_111 = 0;
    TTT *tail_222 = 0;

    TTT *n;

    n = (TTT *)malloc(sizeof(TTT));
    n->a = 1;
    ZMLINK_APPEND(head_111, tail_222, n, prev_123, next_456);

    n = (TTT *)malloc(sizeof(TTT));
    n->a = 2;
    ZMLINK_APPEND(head_111, tail_222, n, prev_123, next_456);

    n = (TTT *)malloc(sizeof(TTT));
    n->a = 8;
    ZMLINK_PREPEND(head_111, tail_222, n, prev_123, next_456);


    n = head_111;
    ZMLINK_DETACH(head_111, tail_222, n, prev_123, next_456);
    printf("%d\n", n->a);
    free(n);

    n = head_111;
    ZMLINK_DETACH(head_111, tail_222, n, prev_123, next_456);
    printf("%d\n", n->a);
    free(n);

    n = tail_222;
    ZMLINK_DETACH(head_111, tail_222, n, prev_123, next_456);
    printf("%d\n", n->a);
    free(n);

    return 0;
}
