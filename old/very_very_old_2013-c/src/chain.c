#include "zyc.h"

ZCHAIN *zchain_create(void){
	ZCHAIN *r;

	r=(ZCHAIN *)z_malloc(sizeof(ZCHAIN));
	memset(r,0,sizeof(ZCHAIN));
	return r;
}

void zchain_free(ZCHAIN *zc, void(*free_fn)(void *, char *), char *ptr){
	ZCHAIN_NODE *n, *next;

	for(n=zc->head;n;n=next){
		next=n->next;
		if(free_fn){
			free_fn(n->value, ptr);
		}
		z_free(n);
	}
	z_free(zc);
}

ZCHAIN_NODE *zchain_insert_before(ZCHAIN *zc, char *value, ZCHAIN_NODE *before){
	ZCHAIN_NODE *prev, *nc;

	if(before == 0) {
		return zchain_push(zc, value);
	}
	if(before == zc->head){
		return zchain_unshift(zc, value);
	}

	nc=(ZCHAIN_NODE *)z_malloc(sizeof(ZCHAIN_NODE));
	nc->value=value;

	prev=before->prev;
	prev->next=before->prev=nc;
	nc->prev=prev;
	nc->next=before;

	zc->len ++;

	return nc;
}

ZCHAIN_NODE *zchain_push(ZCHAIN *zc, char *value){
	ZCHAIN_NODE *nc;

	nc=(ZCHAIN_NODE *)z_malloc(sizeof(ZCHAIN_NODE));
	nc->value=value;

	nc->next=0;
	if(zc->len==0){
		zc->head=zc->tail=nc;
		nc->prev=0;
	}else{
		zc->tail->next=nc;
		nc->prev=zc->tail;
		zc->tail=nc;
	}

	zc->len++;

	return nc;
}

ZCHAIN_NODE *zchain_unshift(ZCHAIN *zc, char *value){
	ZCHAIN_NODE *nc;

	nc=(ZCHAIN_NODE *)z_malloc(sizeof(ZCHAIN_NODE));
	nc->value=value;

	nc->prev=0;
	if(zc->len==0){
		zc->head=zc->tail=nc;
		nc->prev=0;
	}else{
		zc->head->prev=nc;
		nc->next=zc->head;
		zc->head=nc;
	}

	zc->len++;

	return nc;
}

ZCHAIN_NODE *zchain_detach(ZCHAIN *zc, ZCHAIN_NODE *n){
	ZCHAIN_NODE *prev, *next;

	prev=n->prev;
	next=n->next;
	if(prev){
		prev->next=next;
	}else{
		prev->next=0;
		zc->head=next;
	}
	if(next){
		next->prev=prev;
	}else{
		next->prev=0;
		zc->tail=prev;
	}

	zc->len--;

	return n;
}

ZCHAIN_NODE *zchain_remove(ZCHAIN *zc, ZCHAIN_NODE *n){
	ZCHAIN_NODE *r;

	r=zchain_detach(zc, n);
	z_free(r);

	return r;
}

ZCHAIN_NODE *zchain_pop(ZCHAIN *zc, char **value){
	ZCHAIN_NODE *r;

	r=zc->tail;
	if(r==0) return 0; 

	*value=r->value;
	r=zchain_detach(zc, r);
	z_free(r);

	return r;
}

ZCHAIN_NODE *zchain_shift(ZCHAIN *zc, char **value){
	ZCHAIN_NODE *r;

	r=zc->head;
	if(r==0) return 0; 

	*value=r->value;
	r=zchain_detach(zc, r);
	z_free(r);

	return r;
}
