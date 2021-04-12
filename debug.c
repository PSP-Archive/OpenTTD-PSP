/* $Id: debug.c 7182 2006-11-16 22:05:33Z peter1138 $ */

#include "stdafx.h"
#include <stdio.h>
#include <stdarg.h>
#include "openttd.h"
#include "console.h"
#include "debug.h"
#include "functions.h"
#include "string.h"

#ifdef PSP_NETDEBUG
int _debug_ai_level = 9;
int _debug_driver_level = 9;
int _debug_grf_level = 9;
int _debug_map_level = 9;
int _debug_misc_level = 9;
int _debug_ms_level = 9;
int _debug_net_level = 9;
int _debug_spritecache_level = 9;
int _debug_oldloader_level = 9;
int _debug_ntp_level = 9;
int _debug_npf_level = 9;
int _debug_yapf_level = 9;
int _debug_freetype_level = 9;
#else
int _debug_ai_level;
int _debug_driver_level;
int _debug_grf_level;
int _debug_map_level;
int _debug_misc_level;
int _debug_ms_level;
int _debug_net_level;
int _debug_spritecache_level;
int _debug_oldloader_level;
int _debug_ntp_level;
int _debug_npf_level;
int _debug_yapf_level;
int _debug_freetype_level;
#endif


#ifdef PSP_NETDEBUG
int dbgsock = 0;
#endif

void CDECL debug(const char *s, ...)
{
	va_list va;
	char buf[1024];

	va_start(va, s);
	vsnprintf(buf, lengthof(buf), s, va);
	va_end(va);

#ifdef PSP_NETDEBUG
    if (dbgsock != -1) {
		char buf2[1048];

	    sprintf(buf2, "dbg: %s\n", buf);
	    send(dbgsock, buf2, strlen(buf2), 0);
	}
#else
	fprintf(stderr, "dbg: %s\n", buf);
	IConsoleDebug(buf);
#endif
}

typedef struct DebugLevel {
	const char *name;
	int *level;
} DebugLevel;

#define DEBUG_LEVEL(x) { #x, &_debug_##x##_level }
	static const DebugLevel debug_level[] = {
	DEBUG_LEVEL(ai),
	DEBUG_LEVEL(driver),
	DEBUG_LEVEL(grf),
	DEBUG_LEVEL(map),
	DEBUG_LEVEL(misc),
	DEBUG_LEVEL(ms),
	DEBUG_LEVEL(net),
	DEBUG_LEVEL(spritecache),
	DEBUG_LEVEL(oldloader),
	DEBUG_LEVEL(ntp),
	DEBUG_LEVEL(npf),
	DEBUG_LEVEL(yapf),
	DEBUG_LEVEL(freetype)
	};
#undef DEBUG_LEVEL

void SetDebugString(const char *s)
{
	int v;
	char *end;
	const char *t;

	// global debugging level?
	if (*s >= '0' && *s <= '9') {
		const DebugLevel *i;

		v = strtoul(s, &end, 0);
		s = end;

		for (i = debug_level; i != endof(debug_level); ++i) *i->level = v;
	}

	// individual levels
	for (;;) {
		const DebugLevel *i;
		int *p;

		// skip delimiters
		while (*s == ' ' || *s == ',' || *s == '\t') s++;
		if (*s == '\0') break;

		t = s;
		while (*s >= 'a' && *s <= 'z') s++;

		// check debugging levels
		p = NULL;
		for (i = debug_level; i != endof(debug_level); ++i)
			if (s == t + strlen(i->name) && strncmp(t, i->name, s - t) == 0) {
				p = i->level;
				break;
			}

		if (*s == '=') s++;
		v = strtoul(s, &end, 0);
		s = end;
		if (p != NULL) {
			*p = v;
		} else {
			ShowInfoF("Unknown debug level '%.*s'", s - t, t);
			return;
		}
	}
}

/** Print out the current debug-level
 * Just return a string with the values of all the debug categorites
 * @return string with debug-levels
 */
const char *GetDebugString(void)
{
	const DebugLevel *i;
	static char dbgstr[100];
	char dbgval[20];

	memset(dbgstr, 0, sizeof(dbgstr));
	i = debug_level;
	snprintf(dbgstr, sizeof(dbgstr), "%s=%d", i->name, *i->level);

	for (i++; i != endof(debug_level); i++) {
		snprintf(dbgval, sizeof(dbgval), ", %s=%d", i->name, *i->level);
		ttd_strlcat(dbgstr, dbgval, sizeof(dbgstr));
	}

	return dbgstr;
}
