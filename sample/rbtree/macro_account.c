/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-21
 * ================================
 */

#include "zc.h"
#include "sysuser.h"

typedef struct myos_t myos_t;
typedef struct account_t account_t;
struct myos_t {
    zrbtree_t account_rbtree;
};
struct account_t {
    char *name;
    sysuser_t *attributes;
    zrbtree_node_t rbnode;
};

myos_t *myos_create(void)
{
    myos_t *myos;

    myos = (myos_t *) zcalloc(1, sizeof(myos_t));
    ZRBTREE_INIT(&(myos->account_rbtree), 0);

    return myos;
}

account_t *myos_add_account(myos_t * myos, char *name, void *attributes)
{
    account_t *n, *cmp_n;
    zrbtree_node_t *rn, *cmp_rn /* and return_rn */ ;
    int cmp_result;

    n = (account_t *) zcalloc(1, sizeof(account_t));
    n->name = zstrdup(name);
    n->attributes = (sysuser_t *) attributes;

    rn = &(n->rbnode);

    ZRBTREE_ATTACH_PART1(&(myos->account_rbtree), rn, cmp_rn);

    cmp_n = ZCONTAINER_OF(cmp_rn, account_t, rbnode);
    cmp_result = strcmp(n->name, cmp_n->name);

    ZRBTREE_ATTACH_PART2(&(myos->account_rbtree), rn, cmp_result, cmp_rn);

    if (rn != cmp_rn) {
        /*
         * Already exist.
         * rn is equal cmp_rn on the context.
         * We donot deal this condition in the testing.
         */
        zfree(n->name);
        zfree(n);
        return ZCONTAINER_OF(cmp_rn, account_t, rbnode);
    }
    return n;
}

account_t *myos_find_account(myos_t * myos, const char *name, void **value)
{
    account_t *cmp_n;
    zrbtree_node_t *cmp_rn /* and return_rn */ ;
    int cmp_result;

    ZRBTREE_LOOKUP_PART1(&(myos->account_rbtree), cmp_rn);

    cmp_n = ZCONTAINER_OF(cmp_rn, account_t, rbnode);
    cmp_result = strcmp(name, cmp_n->name);

    ZRBTREE_LOOKUP_PART2(&(myos->account_rbtree), cmp_result, cmp_rn);
    if (cmp_rn == 0) {
        return 0;
    }
    cmp_n = ZCONTAINER_OF(cmp_rn, account_t, rbnode);
    if (value) {
        *value = (void *)(cmp_n->attributes);
    }

    return cmp_n;
}

account_t *myos_delete_account(myos_t * myos, char *name, void **value)
{
    account_t *n;

    n = myos_find_account(myos, name, value);
    if (!n) {
        return 0;
    }
    ZRBTREE_DETACH(&(myos->account_rbtree), &(n->rbnode));
    zfree(n);

    return n;
}

void myos_free(myos_t * myos)
{
    zrbtree_node_t *rn;
    account_t *account;

    while ((rn = zrbtree_first(&(myos->account_rbtree)))) {
        zrbtree_detach(&(myos->account_rbtree), rn);
        account = ZCONTAINER_OF(rn, account_t, rbnode);
        zfree(account->name);
        zfree(account);
    }
    zfree(myos);
}

int main(int argc, char **argv)
{
    myos_t *myos;
    account_t *account;
    sysuser_t *user;
    int i;

    myos = myos_create();

    zinfo("load /etc/passwd");
    sysuser_load();

    for (i = 0; i < sysuser_count; i++) {
        myos_add_account(myos, sysuser_list[i].login_name, sysuser_list + i);
    }

    if (myos_find_account(myos, "daemon", (void **)&user)) {
        zinfo("Found user daemon, whose shell is %s", user->shell);
    } else {
        zinfo("Dit not find the user daemon");
    }

    zinfo("test MACRO of walk");
    ZRBTREE_WALK_FORWARD_BEGIN(&(myos->account_rbtree), rn) {
        account = ZCONTAINER_OF(rn, account_t, rbnode);
        zinfo("name: %s, home: %s", account->name, account->attributes->home);
    }
    ZRBTREE_WALK_FORWARD_END;

    zinfo("test walk ...............");

    myos_free(myos);

    sysuser_unload();

    return (0);
}
