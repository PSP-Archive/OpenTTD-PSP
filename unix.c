/* $Id: unix.c 10152 2007-06-13 20:48:11Z rubidium $ */

#include "stdafx.h"
#include "openttd.h"
#include "functions.h"
#include "window.h"
#include "string.h"
#include "table/strings.h"
#include "variables.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#ifdef USE_HOMEDIR
#include <pwd.h>
#endif

#if (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L) || defined(__GLIBC__)
	#define HAS_STATVFS
#endif

#ifdef HAS_STATVFS
#include <sys/statvfs.h>
#endif


#ifdef __MORPHOS__
#include <exec/types.h>
ULONG __stack = (1024*1024)*2; // maybe not that much is needed actually ;)

// The system supplied definition of SIG_IGN does not match
#undef SIG_IGN
#define SIG_IGN (void (*)(int))1
#endif /* __MORPHOS__ */

#ifdef __AMIGA__
#warning add stack symbol to avoid that user needs to set stack manually (tokai)
// ULONG __stack =
#endif

#if defined(__APPLE__)
	#if defined(WITH_SDL)
		//the mac implementation needs this file included in the same file as main()
		#include <SDL.h>
	#endif
#endif

bool FiosIsRoot(const char *path)
{
#if !defined(__MORPHOS__) && !defined(__AMIGAOS__)
	return path[1] == '\0';
#else
	/* On MorphOS or AmigaOS paths look like: "Volume:directory/subdirectory" */
	const char *s = strchr(path, ':');
	return s != NULL && s[1] == '\0';
#endif
}

void FiosGetDrives(void)
{
	return;
}

bool FiosGetDiskFreeSpace(const char *path, uint32 *tot)
{
	uint32 free = 0;

#ifdef HAS_STATVFS
	{
		struct statvfs s;

		if (statvfs(path, &s) != 0) return false;
		free = (uint64)s.f_frsize * s.f_bavail >> 20;
	}
#endif
	if (tot != NULL) *tot = free;
	return true;
}

bool FiosIsValidFile(const char *path, const struct dirent *ent, struct stat *sb)
{
	char filename[MAX_PATH];

#if defined(__MORPHOS__) || defined(__AMIGAOS__)
	/* On MorphOS or AmigaOS paths look like: "Volume:directory/subdirectory" */
	if (FiosIsRoot(path)) {
		snprintf(filename, lengthof(filename), "%s:%s", path, ent->d_name);
	} else if (path[strlen(path) - 1] == PATHSEPCHAR) { // PATHSEP is only one byte
		/* Paths with double PATHSEP like "directory//foobar.ext" won't work under MorphOS/AmigaOS
		 * (the extra slash is interpreted like ".." under UNIXes). UNIXes simply interpret double
		 * slash as single slash, so the adding of the PATHSEP further below could actually be
		 * removed. */
		snprintf(filename, lengthof(filename), "%s%s", path, ent->d_name);
	} else // XXX - only next line!
#endif
	snprintf(filename, lengthof(filename), "%s" PATHSEP "%s", path, ent->d_name);

	return stat(filename, sb) == 0;
}

bool FiosIsHiddenFile(const struct dirent *ent)
{
	return ent->d_name[0] == '.';
}

#if defined(__BEOS__) || defined(__linux__)
static void ChangeWorkingDirectory(char *exe)
{
	char *s = strrchr(exe, '/');
	if (s != NULL) {
		*s = '\0';
		chdir(exe);
		*s = '/';
	}
}
#endif

void ShowInfo(const char *str)
{
	fprintf(stderr, "%s\n", str);
}

void ShowOSErrorBox(const char *buf)
{
#if defined(__APPLE__)
	// this creates an NSAlertPanel with the contents of 'buf'
	// this is the native and nicest way to do this on OSX
	ShowMacDialog( buf, "See readme for more info\nMost likely you are missing files from the original TTD", "Quit" );
#else
	// all systems, but OSX
	fprintf(stderr, "\033[1;31mError: %s\033[0;39m\n", buf);
#endif
}

#ifdef WITH_COCOA
void cocoaSetWorkingDirectory(void);
void cocoaSetupAutoreleasePool(void);
void cocoaReleaseAutoreleasePool(void);
#endif

int CDECL main(int argc, char* argv[])
{
	int ret;

#ifdef WITH_COCOA
	cocoaSetupAutoreleasePool();
	/* This is passed if we are launched by double-clicking */
	if (argc >= 2 && strncmp(argv[1], "-psn", 4) == 0) {
		argv[1] = NULL;
		argc = 1;
		cocoaSetWorkingDirectory();
	}
#endif

	// change the working directory to enable doubleclicking in UIs
#if defined(__BEOS__) || defined(__linux__)
	ChangeWorkingDirectory(argv[0]);
#endif

	_random_seeds[1][1] = _random_seeds[1][0] = _random_seeds[0][1] = _random_seeds[0][0] = time(NULL);
	SeedMT(_random_seeds[0][1]);

	signal(SIGPIPE, SIG_IGN);

	ret = ttd_main(argc, argv);

#ifdef WITH_COCOA
	cocoaReleaseAutoreleasePool();
#endif

	return ret;
}

void DeterminePaths(void)
{
	char *s;

	_paths.game_data_dir = malloc(MAX_PATH);
	ttd_strlcpy(_paths.game_data_dir, GAME_DATA_DIR, MAX_PATH);
	#if defined SECOND_DATA_DIR
	_paths.second_data_dir = malloc(MAX_PATH);
	ttd_strlcpy(_paths.second_data_dir, SECOND_DATA_DIR, MAX_PATH);
	#endif

#if defined(USE_HOMEDIR)
	{
		const char *homedir = getenv("HOME");

		if (homedir == NULL) {
			const struct passwd *pw = getpwuid(getuid());
			if (pw != NULL) homedir = pw->pw_dir;
		}

		_paths.personal_dir = str_fmt("%s" PATHSEP "%s", homedir, PERSONAL_DIR);
	}

#else /* not defined(USE_HOMEDIR) */

	_paths.personal_dir = malloc(MAX_PATH);
	ttd_strlcpy(_paths.personal_dir, PERSONAL_DIR, MAX_PATH);

	// check if absolute or relative path
	s = strchr(_paths.personal_dir, '/');

	// add absolute path
	if (s == NULL || _paths.personal_dir != s) {
		getcwd(_paths.personal_dir, MAX_PATH);
		s = strchr(_paths.personal_dir, 0);
		*s++ = '/';
		ttd_strlcpy(s, PERSONAL_DIR, MAX_PATH);
	}

#endif /* defined(USE_HOMEDIR) */

	s = strchr(_paths.personal_dir, 0);

	// append a / ?
	if (s[-1] != '/') strcpy(s, "/");

	_paths.save_dir = str_fmt("%ssave", _paths.personal_dir);
	_paths.autosave_dir = str_fmt("%s/autosave", _paths.save_dir);
	_paths.scenario_dir = str_fmt("%sscenario", _paths.personal_dir);
	_paths.heightmap_dir = str_fmt("%sscenario/heightmap", _paths.personal_dir);
	_paths.gm_dir = str_fmt("%sgm/", _paths.game_data_dir);
	_paths.data_dir = str_fmt("%sdata/", _paths.game_data_dir);

	if (_config_file == NULL)
		_config_file = str_fmt("%sopenttd.cfg", _paths.personal_dir);

	_highscore_file = str_fmt("%shs.dat", _paths.personal_dir);
	_log_file = str_fmt("%sopenttd.log", _paths.personal_dir);

#if defined CUSTOM_LANG_DIR
	// sets the search path for lng files to the custom one
	_paths.lang_dir = malloc( MAX_PATH );
	ttd_strlcpy( _paths.lang_dir, CUSTOM_LANG_DIR, MAX_PATH);
#else
	_paths.lang_dir = str_fmt("%slang/", _paths.game_data_dir);
#endif

	// create necessary folders
	mkdir(_paths.personal_dir, 0755);
	mkdir(_paths.save_dir, 0755);
	mkdir(_paths.autosave_dir, 0755);
	mkdir(_paths.scenario_dir, 0755);
	mkdir(_paths.heightmap_dir, 0755);
}

bool InsertTextBufferClipboard(Textbuf *tb)
{
	return false;
}


// multi os compatible sleep function

#ifdef __AMIGA__
// usleep() implementation
#	include <devices/timer.h>
#	include <dos/dos.h>

	extern struct Device      *TimerBase    = NULL;
	extern struct MsgPort     *TimerPort    = NULL;
	extern struct timerequest *TimerRequest = NULL;
#endif // __AMIGA__

void CSleep(int milliseconds)
{
	#if !defined(__BEOS__) && !defined(__AMIGA__)
		usleep(milliseconds * 1000);
	#endif
	#ifdef __BEOS__
		snooze(milliseconds * 1000);
	#endif
	#if defined(__AMIGA__)
	{
		ULONG signals;
		ULONG TimerSigBit = 1 << TimerPort->mp_SigBit;

		// send IORequest
		TimerRequest->tr_node.io_Command = TR_ADDREQUEST;
		TimerRequest->tr_time.tv_secs    = (milliseconds * 1000) / 1000000;
		TimerRequest->tr_time.tv_micro   = (milliseconds * 1000) % 1000000;
		SendIO((struct IORequest *)TimerRequest);

		if (!((signals = Wait(TimerSigBit | SIGBREAKF_CTRL_C)) & TimerSigBit) ) {
			AbortIO((struct IORequest *)TimerRequest);
		}
		WaitIO((struct IORequest *)TimerRequest);
	}
	#endif // __AMIGA__
}

#ifdef WITH_ICONV

#include <iconv.h>
#include <errno.h>
#include "debug.h"

#define INTERNALCODE "UTF-8"

/** Try and try to decipher the current locale from environmental
 * variables. MacOSX is hardcoded, other OS's are dynamic. If no suitable
 * locale can be found, don't do any conversion "" */
static const char *GetLocalCode(void)
{
#if defined(__APPLE__)
	return "UTF-8-MAC";
#else
	/* Strip locale (eg en_US.UTF-8) to only have UTF-8 */
	const char *locale = GetCurrentLocale("LC_CTYPE");
	if (locale != NULL) locale = strchr(locale, '.');

	return (locale == NULL) ? "" : locale + 1;
#endif
}

/** FYI: This is not thread-safe.
 * convert between locales, which from and which to is set in the calling
 * functions OTTD2FS() and FS2OTTD(). You should NOT use this function directly
 * NOTE: iconv was added in OSX 10.3. 10.2.x will still have the invalid char
 * issues. There aren't any easy fix for this */
static const char *convert_tofrom_fs(iconv_t convd, const char *name)
{
	static char buf[1024];
	/* Work around buggy iconv implementation where inbuf is wrongly typed as
	 * non-const. Correct implementation is at
	 * http://www.opengroup.org/onlinepubs/007908799/xsh/iconv.html */
#if defined (__GLIBC__) || defined (__GNU_LIBRARY__)
	char *inbuf = (char*)name;
#else
	const char *inbuf = name;
#endif

	char *outbuf  = buf;
	size_t outlen = sizeof(buf) - 1;
	size_t inlen  = strlen(name);

	ttd_strlcpy(outbuf, name, sizeof(buf));

	iconv(convd, NULL, NULL, NULL, NULL);
	if (iconv(convd, &inbuf, &inlen, &outbuf, &outlen) == (size_t)(-1)) {
		DEBUG(misc, 0) ("[Iconv] Error converting '%s'. Errno %d", name, errno);
	}

	*outbuf = '\0';
	// FIX: invalid characters will abort conversion, but they shouldn't occur?
	return buf;
}

/** Convert from OpenTTD's encoding to that of the local environment
 * @param name pointer to a valid string that will be converted
 * @return pointer to a new stringbuffer that contains the converted string */
const char *OTTD2FS(const char *name)
{
	static iconv_t convd = (iconv_t)(-1);

	if (convd == (iconv_t)(-1)) {
		const char *env = GetLocalCode();
		convd = iconv_open(env, INTERNALCODE);
		if (convd == (iconv_t)(-1)) {
			DEBUG(misc, 0) ("[iconv] Conversion from codeset '%s' to '%s' unsupported", INTERNALCODE, env);
			return name;
		}
	}

	return convert_tofrom_fs(convd, name);
}

/** Convert to OpenTTD's encoding from that of the local environment
 * @param name pointer to a valid string that will be converted
 * @return pointer to a new stringbuffer that contains the converted string */
const char *FS2OTTD(const char *name)
{
	static iconv_t convd = (iconv_t)(-1);

	if (convd == (iconv_t)(-1)) {
		const char *env = GetLocalCode();
		convd = iconv_open(INTERNALCODE, env);
		if (convd == (iconv_t)(-1)) {
			DEBUG(misc, 0) ("[iconv] Conversion from codeset '%s' to '%s' unsupported", env, INTERNALCODE);
			return name;
		}
	}

	return convert_tofrom_fs(convd, name);
}

#else
const char *FS2OTTD(const char *name) {return name;}
const char *OTTD2FS(const char *name) {return name;}
#endif /* WITH_ICONV */
