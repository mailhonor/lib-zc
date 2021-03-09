#include <zyc.h>
#include <test.h>

int main()
{
	char *buf = "AA";
	char *buf2 = "B0C";
	ZSTR *ss;

#define debugme		zmsg_debug("line: %d, size:%d, len:%d, str:%s\n", __LINE__, ss->size,ss->len,ss->str)

	ss = zstr_create(3);
	debugme;
	zstr_strcats(ss,  buf, buf, buf, "sss", 0);
	debugme;
	zstr_strcats(ss,  buf2, "3",0);
	debugme;
	zstr_put(ss, 'D');
	debugme;
	zstr_put(ss, ' ');
	zstr_snprintf_append(ss, 33, "linux is good ssssssssssssssssssssssssssssssssaaaa aaa %s", __FILE__);
	debugme;
	zstr_sprintf_append(ss, " %X, %.30s",  128, " SSSSS linux ");
	zstr_sprintf_append(ss, " BADAFADFAF linux ");
	zstr_sprintf_append(ss, " BADAFADFAF linux ");
	zstr_sprintf_append(ss, " BADAFADFAF linux ");
	debugme;
	zstr_sprintf(ss, "linux is good BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB aaa %s", __FILE__);
	debugme;
	ss= zstr_fconcatenate("afasdfsd: %s", "good over");
	debugme;
	zstr_sprintf(ss, "%X, %.3s:%m",  128, " SSSSS linux ");
	debugme;
	return(0);
}
