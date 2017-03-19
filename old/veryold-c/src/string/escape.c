#include "zc.h"

int zstr_escape(char *src, char *dest, char *from, char *to, char *delete, char *terminate)
{
	char *new_dest = dest;
	char *_from, *_to, *_delete, *_terminate;
	char pc, ac, lc = '\0';
	char fc, tc;
	int _find;

	while (1) {
		pc = *src++;
		ac = pc;
		if (lc == '\\') {
			if (pc == '\0') {
				ac = '\0';
				*new_dest = 0;
				return (new_dest - dest);
			}

			_from = from;
			_to = to;
			while (1) {
				fc = *_from++;
				tc = *_to++;
				if (!fc) {
					break;
				}
				if (pc == fc) {
					ac = tc;
					break;
				}
			}
			*dest++ = ac;
			lc = '\0';
		} else if (pc == '\\') {
			lc = '\\';
		} else if (pc == '\0') {
			*new_dest = 0;
			return (new_dest - dest);
		} else {
			_delete = delete;
			_terminate = terminate;
			_find = 0;
			while (1) {
				fc = *_delete;
				if (!fc) {
					break;
				}
				if (pc == fc) {
					_find = 1;
					break;
				}
			}
			if (_find == 1) {
				continue;
			}
			while (1) {
				fc = *_terminate;
				if (!fc) {
					break;
				}
				if (pc == fc) {
					_find = 1;
					break;
				}
			}
			if (_find == 1) {
				*new_dest = 0;
				return (new_dest - dest);
			}
			*new_dest++ = pc;
		}
	}

	return 0;
}
