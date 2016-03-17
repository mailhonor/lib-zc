#include "zc.h"

int zdict_parse_line(ZDICT * zd, char *buf, int len)
{
	int i, last_ch, now;
	char ch;
	ZBUF *zb, *zbk, *zbv;

	zbk = zbuf_create(128);
	zbv = zbuf_create(128);
	last_ch = -1;
	now = 1;
	zb = zbk;
	for (i = 0; i <= len; i++) {
		if (i == len) {
			ch = '\0';
		} else {
			ch = buf[i];
		}
		if (last_ch == '\\') {
			last_ch = -1;
			if (ch == ';' || ch == '=' || ch == '\\') {
				zbuf_put(zb, ch);
				continue;
			}
			zbuf_put(zb, '\\');
			zbuf_put(zb, ch);
			continue;
		}
		if (ch == '\\') {
			last_ch = ch;
			continue;
		}
		if (ch == '=') {
			if (now == 1) {
				zb = zbv;
				now = 2;
				continue;
			}
		}
		if (ch == ';' || ch == '\0') {
			if (ZBUF_LEN(zbv) == 0) {
				zbuf_strcpy(zbv, "yes");
			}
			if (ZBUF_LEN(zbk)) {
				ZBUF_TERMINATE(zbk);
				ZBUF_TERMINATE(zbv);
				zdict_enter_STR(zd, ZBUF_DATA(zbk), ZBUF_DATA(zbv));
			}
			ZBUF_RESET(zbk);
			ZBUF_RESET(zbv);
			zb = zbk;
			now = 1;
			continue;
		}
		zbuf_put(zb, ch);
	}

	zbuf_free(zbk);
	zbuf_free(zbv);

	return 0;
}
