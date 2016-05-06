/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "libzc.h"
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

int myos_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    account_t *a1, *a2;

    a1 = ZCONTAINER_OF(n1, account_t, rbnode);
    a2 = ZCONTAINER_OF(n2, account_t, rbnode);

    return strcmp(a1->name, a2->name);
}

myos_t *myos_create(void)
{
    myos_t *myos;

    myos = (myos_t *) zcalloc(1, sizeof(myos_t));
    zrbtree_init(&(myos->account_rbtree), myos_cmp);

    return myos;
}

account_t *myos_add_account(myos_t * myos, char *name, void *attributes)
{
    account_t *n;

    n = (account_t *) zcalloc(1, sizeof(account_t));
    n->name = zstrdup(name);
    n->attributes = (sysuser_t *) attributes;

    zrbtree_attach(&(myos->account_rbtree), &(n->rbnode));

    return n;
}

account_t *myos_find_account(myos_t * myos, char *name, void **value)
{
    account_t account;
    zrbtree_node_t *node;

    account.name = name;
    node = zrbtree_lookup(&(myos->account_rbtree), &(account.rbnode));
    if (!node) {
        return 0;
    }

    if (value) {
        *value = ZCONTAINER_OF(node, account_t, rbnode)->attributes;
    }

    return ZCONTAINER_OF(node, account_t, rbnode);
}

account_t *myos_delete_account(myos_t * myos, char *name, void **value)
{
    account_t *n;

    n = myos_find_account(myos, name, value);
    if (!n) {
        return 0;
    }
    zrbtree_detach(&(myos->account_rbtree), &(n->rbnode));

    zfree(n->name);
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

void walk_fn(zrbtree_node_t * n, void *ctx)
{
    zinfo("name: %s", ZCONTAINER_OF(n, account_t, rbnode)->name);
}

int main(int argc, char **argv)
{
    myos_t *myos;
    account_t *account;
    zrbtree_node_t *rn;
    sysuser_t *user = 0;
    int i;

    zvar_progname = argv[0];

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
        zinfo("name: %s", account->name);
    }
    ZRBTREE_WALK_FORWARD_END;

    zinfo("test walk ...............");
    zrbtree_walk(&(myos->account_rbtree), walk_fn, 0);

    myos_free(myos);

    sysuser_unload();

    return (0);
}
