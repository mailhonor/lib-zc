#include <zyc.h>

int main(int argc, char **argv)
{
#if 0
	char *a[]={"Hbug", "aaa", "bbb","ccc","zzz"},*key;
	int i,ret;
	ZMAPS *map;

	map=zmaps_create(" btree:test/hash.db,,static:ewqr1, ");
	map=zmaps_create(" ,,static:ewqr1, ");
	printf("map ok=%d\n", map?1:0);

	for(i=0;i<4;i++){
		key=a[i];
		ret=zmaps_lookup(map, key, 0);
		if(ret==0){
			printf("key:%s, result: none\n", key);
		}else if(ret == ZMAPS_LOOKUP_ERROR){
			printf("key:%s, result error: %s\n", key, zstr_str(zvar_maps_result));
		}else{
			printf("key:%s, result: %s\n", key, zstr_str(zvar_maps_result));
		}
	}
#endif
	fprintf(stderr, "\n\nThe program is obsoleting, use zyc_lookupmaps\n\n");
	return 0;
}
