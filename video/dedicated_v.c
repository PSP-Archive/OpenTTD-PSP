/* $Id: dedicated_v.c 9348 2007-03-19 20:05:49Z rubidium $ */

#include "../stdafx.h"

#ifdef ENABLE_NETWORK
#ifndef PSP

#include "../openttd.h"
#include "../debug.h"
#include "../functions.h"
#include "../gfx.h"
#include "../network.h"
#include "../window.h"
#include "../console.h"
#include "../variables.h"
#include "../genworld.h"
#include "dedicated_v.h"

#ifdef BEOS_NET_SERVER
#include <net/socket.h>
#endif

#ifdef __OS2__
#	include <sys/time.h> /* gettimeofday */
#	include <sys/types.h>
#	include <unistd.h>
#	include <conio.h>

#	define INCL_DOS
#	include <os2.h>

#	define STDIN 0  /* file descriptor for standard input */

/**
 * Switches OpenTTD to a console app at run-time, instead of a PM app
 * Necessary to see stdout, etc. */
static void OS2_SwitchToConsoleMode(void)
{
	PPIB pib;
	PTIB tib;

	DosGetInfoBlocks(&tib, &pib);

	// Change flag from PM to VIO
	pib->pib_ultype = 3;
}
#endif

#ifdef UNIX
#	include <sys/time.h> /* gettimeofday */
#	include <sys/types.h>
#	include <unistd.h>
#	include <signal.h>
#	define STDIN 0  /* file descriptor for standard input */

/* Signal handlers */
static void DedicatedSignalHandler(int sig)
{
	_exit_game = true;
	signal(sig, DedicatedSignalHandler);
}
#endif

#ifdef WIN32
#include <windows.h> /* GetTickCount */
#include <conio.h>
#include <time.h>
#include <tchar.h>
static HANDLE _hInputReady, _hWaitForInputHandling;
static HANDLE _hThread; // Thread to close
static char _win_console_thread_buffer[200];

/* Windows Console thread. Just loop and signal when input has been received */
static void WINAPI CheckForConsoleInput(void)
{
	DWORD nb;
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	while (true) {
		ReadFile(hStdin, _win_console_thread_buffer, lengthof(_win_console_thread_buffer), &nb, NULL);
		/* Signal input waiting that input is read and wait for it being handled
		 * SignalObjectAndWait() should be used here, but it's unsupported in Win98< */
		SetEvent(_hInputReady);
		WaitForSingleObject(_hWaitForInputHandling, INFINITE);
	}
}

static void CreateWindowsConsoleThread(void)
{
	DWORD dwThreadId;
	/* Create event to signal when console input is ready */
	_hInputReady = CreateEvent(NULL, false, false, NULL);
	_hWaitForInputHandling = CreateEvent(NULL, false, false, NULL);
	if (_hInputReady == NULL || _hWaitForInputHandling == NULL) error("Cannot create console event!");

	_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckForConsoleInput, NULL, 0, &dwThreadId);
	if (_hThread == NULL) error("Cannot create console thread!");

	DEBUG(driver, 1) ("Windows console thread started...");
}

static void CloseWindowsConsoleThread(void)
{
	CloseHandle(_hThread);
	CloseHandle(_hInputReady);
	CloseHandle(_hWaitForInputHandling);
	DEBUG(driver, 1) ("Windows console thread shut down...");
}

#endif


static void *_dedicated_video_mem;

extern bool SafeSaveOrLoad(const char *filename, int mode, int newgm);
extern void SwitchMode(int new_mode);


static const char *DedicatedVideoStart(const char * const *parm)
{
	_screen.width = _screen.pitch = _cur_resolution[0];
	_screen.height = _cur_resolution[1];
	_dedicated_video_mem = malloc(_cur_resolution[0]*_cur_resolution[1]);

	_debug_net_level = 6;
	_debug_misc_level = 0;

#ifdef WIN32
	// For win32 we need to allocate a console (debug mode does the same)
	CreateConsole();
	CreateWindowsConsoleThread();
	SetConsoleTitle(_T("OpenTTD Dedicated Server"));
#endif

#ifdef __OS2__
	// For OS/2 we also need to switch to console mode instead of PM mode
	OS2_SwitchToConsoleMode();
#endif

	DEBUG(driver, 1)("Loading dedicated server...");
	return NULL;
}

static void DedicatedVideoStop(void)
{
#ifdef WIN32
	CloseWindowsConsoleThread();
#endif
	free(_dedicated_video_mem);
}

static void DedicatedVideoMakeDirty(int left, int top, int width, int height) {}
static bool DedicatedVideoChangeRes(int w, int h) { return false; }
static void DedicatedVideoFullScreen(bool fs) {}

#if defined(UNIX) || defined(__OS2__)
static bool InputWaiting(void)
{
	struct timeval tv;
	fd_set readfds;

	tv.tv_sec = 0;
	tv.tv_usec = 1;

	FD_ZERO(&readfds);
	FD_SET(STDIN, &readfds);

	/* don't care about writefds and exceptfds: */
	return select(STDIN + 1, &readfds, NULL, NULL, &tv) > 0;
}

static uint32 GetTime(void)
{
	struct timeval tim;

	gettimeofday(&tim, NULL);
	return tim.tv_usec / 1000 + tim.tv_sec * 1000;
}

#else

static bool InputWaiting(void)
{
	return WaitForSingleObject(_hInputReady, 1) == WAIT_OBJECT_0;
}

static uint32 GetTime(void)
{
	return GetTickCount();
}

#endif

static void DedicatedHandleKeyInput(void)
{
	static char input_line[200] = "";

	if (!InputWaiting()) return;

	if (_exit_game) return;

#if defined(UNIX) || defined(__OS2__)
	if (fgets(input_line, lengthof(input_line), stdin) == NULL) return;
#else
	/* Handle console input, and singal console thread, it can accept input again */
	strncpy(input_line, _win_console_thread_buffer, lengthof(input_line));
	SetEvent(_hWaitForInputHandling);
#endif

	/* XXX - strtok() does not 'forget' \n\r if it is the first character! */
	strtok(input_line, "\r\n"); // Forget about the final \n (or \r)
	{ /* Remove any special control characters */
		uint i;
		for (i = 0; i < lengthof(input_line); i++) {
			if (input_line[i] == '\n' || input_line[i] == '\r') // cut missed beginning '\0'
				input_line[i] = '\0';

			if (input_line[i] == '\0')
				break;

			if (!IS_INT_INSIDE(input_line[i], ' ', 256))
				input_line[i] = ' ';
		}
	}

	IConsoleCmdExec(input_line); // execute command
}

static void DedicatedVideoMainLoop(void)
{
	uint32 cur_ticks = GetTime();
	uint32 next_tick = cur_ticks + 30;

	/* Signal handlers */
#ifdef UNIX
	signal(SIGTERM, DedicatedSignalHandler);
	signal(SIGINT, DedicatedSignalHandler);
	signal(SIGQUIT, DedicatedSignalHandler);
#endif

	// Load the dedicated server stuff
	_is_network_server = true;
	_network_dedicated = true;
	_network_playas = PLAYER_SPECTATOR;
	_local_player = PLAYER_SPECTATOR;

	/* If SwitchMode is SM_LOAD, it means that the user used the '-g' options */
	if (_switch_mode != SM_LOAD) {
		StartNewGameWithoutGUI(GENERATE_NEW_SEED);
		SwitchMode(_switch_mode);
		_switch_mode = SM_NONE;
	} else {
		_switch_mode = SM_NONE;
		/* First we need to test if the savegame can be loaded, else we will end up playing the
		 *  intro game... */
		if (!SafeSaveOrLoad(_file_to_saveload.name, _file_to_saveload.mode, GM_NORMAL)) {
			/* Loading failed, pop out.. */
			DEBUG(net, 0)("Loading requested map failed. Aborting.");
			_networking = false;
		} else {
			/* We can load this game, so go ahead */
			SwitchMode(SM_LOAD);
		}
	}

	// Done loading, start game!

	if (!_networking) {
		DEBUG(net, 1)("Dedicated server could not be launched. Aborting.");
		return;
	}

	while (!_exit_game) {
		uint32 prev_cur_ticks = cur_ticks; // to check for wrapping
		InteractiveRandom(); // randomness

		if (!_dedicated_forks)
			DedicatedHandleKeyInput();

		cur_ticks = GetTime();
		if (cur_ticks >= next_tick || cur_ticks < prev_cur_ticks) {
			next_tick = cur_ticks + 30;

			GameLoop();
			_screen.dst_ptr = _dedicated_video_mem;
			UpdateWindows();
		}
		CSleep(1);
	}
}

const HalVideoDriver _dedicated_video_driver = {
	DedicatedVideoStart,
	DedicatedVideoStop,
	DedicatedVideoMakeDirty,
	DedicatedVideoMainLoop,
	DedicatedVideoChangeRes,
	DedicatedVideoFullScreen,
};

#endif // PSP
#endif /* ENABLE_NETWORK */
