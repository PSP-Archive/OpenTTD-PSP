/* $Id: macos.m 3745 2006-03-03 07:51:41Z tron $ */

#include <AppKit/AppKit.h>

#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <mach/machine.h>
#include <stdio.h>
#include "../../stdafx.h"
#include "../../openttd.h"
#include "../../newgrf.h"
#include "../../gfx.h"
#include "../../macros.h"
#include "../../string.h"

#ifndef CPU_SUBTYPE_POWERPC_970
#define CPU_SUBTYPE_POWERPC_970 ((cpu_subtype_t) 100)
#endif

/*
 * This file contains objective C
 * Apple uses objective C instead of plain C to interact with OS specific/native functions
 *
 * Note: TrueLight's crosscompiler can handle this, but it likely needs a manual modification for each change in this file.
 * To insure that the crosscompiler still works, let him try any changes before they are committed
 */

static char *GetOSString(void)
{
	static char buffer[175];
	const char* CPU;
	char OS[20];
	char newgrf[125];
	long sysVersion;
	extern const char _openttd_revision[];

	// get the hardware info
	host_basic_info_data_t hostInfo;
	mach_msg_type_number_t infoCount;

	infoCount = HOST_BASIC_INFO_COUNT;
	host_info(
		mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostInfo, &infoCount
	);

	// replace the hardware info with strings, that tells a bit more than just an int
	switch (hostInfo.cpu_subtype) {
#ifdef __POWERPC__
		case CPU_SUBTYPE_POWERPC_750:  CPU = "G3"; break;
		case CPU_SUBTYPE_POWERPC_7400:
		case CPU_SUBTYPE_POWERPC_7450: CPU = "G4"; break;
		case CPU_SUBTYPE_POWERPC_970:  CPU = "G5"; break;
		default:                       CPU = "Unknown PPC"; break;
#else
		/* it looks odd to have a switch for two cases, but it leaves room for easy
		 * expansion. Odds are that Apple will some day use newer CPUs than i686
		 */
		case CPU_SUBTYPE_PENTPRO: CPU = "i686"; break;
		default:                  CPU = "Unknown Intel"; break;
#endif
	}

	// get the version of OSX
	if (Gestalt(gestaltSystemVersion, &sysVersion) != noErr) {
		sprintf(OS, "Undetected");
	} else {
		int majorHiNib = GB(sysVersion, 12, 4);
		int majorLoNib = GB(sysVersion,  8, 4);
		int minorNib   = GB(sysVersion,  4, 4);
		int bugNib     = GB(sysVersion,  0, 4);

		sprintf(OS, "%d%d.%d.%d", majorHiNib, majorLoNib, minorNib, bugNib);
	}

	// make a list of used newgrf files
	if (_first_grffile != NULL) {
		char* n = newgrf;
		const GRFFile* file;

		for (file = _first_grffile; file != NULL; file = file->next) {
			n = strecpy(n, " ", lastof(newgrf));
			n = strecpy(n, file->filename, lastof(newgrf));
		}
	} else {
		sprintf(newgrf, "none");
	}

	snprintf(
		buffer, lengthof(buffer),
		"Please add this info: (tip: copy-paste works)\n"
		"CPU: %s, OSX: %s, OpenTTD version: %s\n"
		"NewGRF files:%s",
		CPU, OS, _openttd_revision, newgrf
	);
	return buffer;
}


#ifdef WITH_SDL

void ShowMacDialog(const char* title, const char* message, const char* buttonLabel)
{
	NSRunAlertPanel([NSString stringWithCString: title], [NSString stringWithCString: message], [NSString stringWithCString: buttonLabel], nil, nil);
}

#elif defined WITH_COCOA

void CocoaDialog(const char* title, const char* message, const char* buttonLabel);

void ShowMacDialog(const char* title, const char* message, const char* buttonLabel)
{
	CocoaDialog(title, message, buttonLabel);
}


#else

void ShowMacDialog(const char* title, const char* message, const char* buttonLabel)
{
	fprintf(stderr, "%s: %s\n", title, message);
}

#endif

void ShowMacAssertDialog(const char* function, const char* file, const int line, const char* expression)
{
	const char* buffer =
		[[NSString stringWithFormat:@
			"An assertion has failed and OpenTTD must quit.\n"
			"%s in %s (line %d)\n"
			"\"%s\"\n"
			"\n"
			"You should report this error the OpenTTD developers if you think you found a bug.\n"
			"\n"
			"%s",
			function, file, line, expression, GetOSString()] cString
		];
	NSLog(@"%s", buffer);
	ToggleFullScreen(0);
	ShowMacDialog("Assertion Failed", buffer, "Quit");

	// abort so that a debugger has a chance to notice
	abort();
}


void ShowMacErrorDialog(const char *error)
{
	const char* buffer =
		[[NSString stringWithFormat:@
			"Please update to the newest version of OpenTTD\n"
			"If the problem presists, please report this to\n"
			"http://bugs.openttd.org\n"
			"\n"
			"%s",
			GetOSString()] cString
		];
	ToggleFullScreen(0);
	ShowMacDialog(error, buffer, "Quit");
	abort();
}
