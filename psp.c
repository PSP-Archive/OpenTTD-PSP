
/***************************************************************************
 *            psp.c
 *
 *  Sun Mar  12 18:41:24 2006
 *  Copyright  2005  Jaime Penalba Estebanez
 *  jpenalbae@gmail.com
 *
 *  This is a modified copy of unix.c for psp, thanks to the ottd team.
 *  Network code mostly taken from pspsdk examples and psppet wifi multi test,
 *  thanks to both, as without their research this wouldnt be possible.
 *
 *  Also thanks to SilentDragon and RacerII for their patch contributions
 *  at the psp port forums.
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "stdafx.h"
#include "openttd.h"
#include "functions.h"
#include "fios.h"
#include "fileio.h"
#include "sound.h"
#include "window.h"
#include "string.h"
#include "table/strings.h"
#include "hal.h"
#include "variables.h"
#include "network.h"
#include "debug.h"
#include "driver.h"
#include "hal.h"

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

/* SDK Includes */
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspthreadman.h>
#include <pspaudiolib.h>
#include <pspctrl.h>
#include <psptypes.h>
#include <psppower.h>
#include <pspwlan.h>
#include <pspgu.h>
#include <pspsdk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <psputility_netmodules.h>
#include <pspnet_resolver.h>
#include <psputility_netparam.h>
#include <pspkernel.h>

/* Networking */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

// Wireless Debug info
#define PSP_CONNECTION_ID 1
#define PSP_DEBUG_PORT 4180
#define PSP_DEBUG_HOST "192.168.0.2"

#define printf  pspDebugScreenPrintf


//#define BUF_WIDTH (512)
//#define SCR_WIDTH (480)
//#define SCR_HEIGHT (272)
//#define PIXEL_SIZE (4) /* change this if you change to another screenmode */
//#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
//#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2) /* zbuffer seems to be 16-bit? */

PSP_MODULE_INFO("OpenTTD", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
//PSP_HEAP_SIZE_MAX();
//PSP_HEAP_SIZE_KB(20480);
PSP_HEAP_SIZE_KB(19000);

extern int errno;

short _psp_supend_mode;

#ifdef PSP_NETDEBUG
extern int dbgsock;
#endif

#if defined(ENABLE_NETWORK)
int selected, cnum, pup, pdown;

struct psp_connections {
    int id;
    char name[128];
    char desc[128];
};

struct psp_connections _psp_connections[20];

int net_thread(SceSize args, void* argp); 
#endif

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


// Exit callback
int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
    return 0;
}

/* Power Callback */
int power_callback(int unknown, int pwrflags, void *common)
{
    /* check for power switch and suspending as one is manual and the other automatic */
    if (pwrflags & PSP_POWER_CB_POWER_SWITCH || pwrflags & PSP_POWER_CB_SUSPENDING) {
        _psp_supend_mode = 1;
        FioCloseAll();
    } else if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE) {
        _psp_supend_mode = 0;
    }

    return 0;
}

// Callback thread
int CallbackThread(SceSize args, void *argp)
{
    int cbid;

    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
    scePowerRegisterCallback(0, cbid);

    sceKernelSleepThreadCB();

    return 0;
}

// Sets up the callback thread and returns its thread id
int SetupCallbacks(void)
{
    int thid = 0;

    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if(thid >= 0)
    {
        sceKernelStartThread(thid, 0, 0);
    }

    return thid;
}


static int LanguageCompareFunc(const void *a, const void *b)
{
    return strcasecmp(*(const char* const *)a, *(const char* const *)b);
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
	puts(str);
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

#if defined (ENABLE_NETWORK)
/* Connect to an access point */
int connect_to_apctl(int config)
{
    int err, count = 0;
    int stateLast = -1;

    /* Connect using the first profile */
    err = sceNetApctlConnect(config);
    if (err != 0) {
        printf("sceNetApctlConnect returns %08X\n", err);
        return 0;
    }

    printf("Connecting...\n");
    while (1) {
        int state;
        err = sceNetApctlGetState(&state);
        if (err != 0) {
            printf("sceNetApctlGetState returns $%x\n", err);
            break;
        }
	
        if (state > stateLast) {
            printf("  connection state %d of 4\n", state);
            stateLast = state;
        } else
	       count++;

        if (state == 4)
            break;  // connected

        /* Check retry limit */
        if (count >= 500) {
            err = 1;
            break;
        }

        // wait a little before polling again
        sceKernelDelayThread(50*1000); // 50ms
    }

    if(err != 0) {
        printf("Not connected!\n");
        _broadcast_list[0] = 0;
        _network_available = false;
        return 0;
    }

    printf("Connected!\n");
    return 1;
}


/* Some parts taken from PSPpet wifi multitest - Thanks */
void PspGetAvailableConnections()
{
    int iNetIndex, cIndex;

    for (iNetIndex = 1; iNetIndex < 100; iNetIndex++) {
        char data[128];
        cIndex = iNetIndex - 1;

        if (sceUtilityCheckNetParam(iNetIndex) != 0)
            break;  // no more

        sceUtilityGetNetParam(iNetIndex, 0, (netData*) _psp_connections[cIndex].name);
        sceUtilityGetNetParam(iNetIndex, 1, (netData*) data);
        strcpy(_psp_connections[cIndex].desc, "SSID=");
        strcat(_psp_connections[cIndex].desc, data);

        sceUtilityGetNetParam(iNetIndex, 4, (netData*) data);
        if (data[0]) {
            // not DHCP
            sceUtilityGetNetParam(iNetIndex, 5, (netData*) data);
            strcat(_psp_connections[cIndex].desc, " IPADDR=");
            strcat(_psp_connections[cIndex].desc, data);
        } else
            strcat(_psp_connections[cIndex].desc, " DHCP");

    }

    cnum = cIndex;
    sprintf(_psp_connections[cIndex].name, "END");
}

/* Reads controlls and return 1 if x is pressed */
int pspReadControlls()
{
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);

    /* Handle button press */
    if (pad.Buttons != 0){
        /* Cross pressed */
        if (pad.Buttons & PSP_CTRL_CROSS)
            return 1;
        if ((pad.Buttons & PSP_CTRL_DOWN) && (selected < cnum - 1))
            pdown = 1;
        if ((pad.Buttons & PSP_CTRL_UP) && (selected > 0))
            pup = 1;
    }

    /* Handle button unpress */
    if (!(pad.Buttons & PSP_CTRL_DOWN) && (pdown == 1)) {
        pdown = 0;
        selected++;
    }
    if (!(pad.Buttons & PSP_CTRL_UP) && (pup == 1)) {
        pup = 0;
        selected--;
    }

    return 0;
}

/* Display menu to choose a connection */
int PspChooseConnection()
{
    int cIndex, done;
    PspGetAvailableConnections();
    done = 0;

    if (strcmp(_psp_connections[0].name, "END") == 0)
        return 0;


    printf("\nPlease choose a connection\n");

    do {
        cIndex = 0;
        pspDebugScreenSetXY(0, 4);
        do {
            if (selected == cIndex)
                pspDebugScreenSetTextColor(0x00ff0d0d);
            else
                pspDebugScreenSetTextColor(0x00ffffff);

            printf("%i.- %s\n", cIndex, _psp_connections[cIndex].name);
            printf("     Info: %s\n", _psp_connections[cIndex].desc);
            pspDebugScreenSetTextColor(0x00ffffff);
            cIndex++;
        } while (strcmp(_psp_connections[cIndex].name, "END") != 0);

        printf("\n");
        done = pspReadControlls();
    } while (done == 0);

    return selected + 1;
}


/* Load networking modules, and connect to the network */
int PspNetworkStartUp()
{
    int err, connection;
    connection = PspChooseConnection();

    do {

         sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
         sceUtilityLoadNetModule(PSP_NET_MODULE_INET);

         err = sceNetInit(0x20000, 0x20, 0x1000, 0x20, 0x1000); 
         if (err != 0)
            printf("Error at sceNetInit: %i\n", err);

         err = sceNetInetInit();
         if (err != 0)
            printf("Error at sceNetInetInit: %i\n", err);

         err = sceNetApctlInit(0x1800, 0x42);
         if (err != 0)
            printf("Error at sceNetApctlInit: %i\n", err);


        if(connect_to_apctl(connection)) {
            char szMyIPAddr[32];
            struct in_addr inaddr;

            if (sceNetApctlGetInfo(8, szMyIPAddr) != 0) {
                printf("Unknown Ip address\n");
                _broadcast_list[0] = 0;
                _network_available = false;
            } else {
                printf("Using IP: %s\n", szMyIPAddr);
                inet_aton(szMyIPAddr, &inaddr);
                _broadcast_list[0] = inaddr.s_addr;
                _broadcast_list[1] = 0;
                _network_available = true;
            }
        }
    } while(0);

    return 0;
}


/* Open the socket to the machine used for debug */
#  ifdef PSP_NETDEBUG
void StartNetworkLogging()
{
    int numbytes;
    char buf[1024];

    struct in_addr inaddr;
    struct sockaddr_in server;


    inet_aton(PSP_DEBUG_HOST, &inaddr);

    if ((dbgsock=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        /* llamada a socket() */
        printf("socket() error\n");
        return;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PSP_DEBUG_PORT);
    server.sin_addr = inaddr;
    bzero(&(server.sin_zero),8);

    if(connect(dbgsock, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
           /* llamada a connect() */
           printf("connect() error - errno: %i\n", errno);
           return;
    }

    sprintf(buf, "Abriendo conexion de debug\n");

    if ((numbytes=send(dbgsock, buf, strlen(buf), 0)) == -1){
        /* llamada a recv() */
        printf("Error en send() \n");
        return;
    }

}
#  endif //PSP_DEBUG
#endif //ENABLE_NETWORK


void dummy_test() {
	FILE *file;
	char buff[200];

        pspDebugScreenPrintf("Trying to open & read file\n");
	file = fopen("test", "r");
	fread(buff, 1, 10, file);
        pspDebugScreenPrintf("Content: %s\n", buff);
	fclose(file);


}

int CDECL main(int argc, char* argv[])
{
    int ret;

    pspDebugScreenInit();
    SetupCallbacks();

    /* Start Networking */
    if (sceWlanGetSwitchState() != 0) {
        PspNetworkStartUp();
    } else {
        printf("Network switch off, networking not enabled\n");
        _broadcast_list[0] = 0;
        _network_available = false;
#ifdef PSP_NETDEBUG
        dbgsock = -1;
#endif
    }

    //pspDebugInstallErrorHandler(sdl_psp_exception_handler);

    pspDebugScreenPrintf("Starting OpenTTD\n");
    scePowerSetClockFrequency(222,  222, 111);
    pspDebugScreenPrintf("Cpu speed reseted to 233Mhz, you can change it later\n");

    pspAudioInit();
    pspDebugScreenPrintf("Sound initialized.\n");
    pspDebugScreenPrintf("Loading please Wait...\n");
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

#if defined(PSP_NETDEBUG)
    if (_broadcast_list[0] != 0)
       StartNetworkLogging();
#endif

    _random_seeds[0][1] = _random_seeds[0][0] = time(NULL);
    SeedMT(_random_seeds[0][1]);

    signal(SIGPIPE, SIG_IGN);

    pspDebugScreenPrintf("Running main...\n");
    //sceKernelSleepThread();
    ret = ttd_main(0, NULL);

    sceKernelExitGame();
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
		sceKernelDelayThread(milliseconds * 1000);
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


bool FiosIsRoot(const char *path)
{
	/* On PSP paths look like: "ms0:/directory/subdirectory" */
	const char *s = strchr(path, '/');
	return s[1] == '\0';
}

void FiosGetDrives(void)
{
	return;
}

bool FiosGetDiskFreeSpace(const char *path, uint32 *tot)
{
	uint32 free = 0;
	unsigned int *pbuf;

    pbuf = (unsigned int *) malloc (sizeof(int) * 5);
	sceIoDevctl("ms0:", 0x02425818, &pbuf, sizeof(pbuf), 0, 0);
	free =  ((pbuf[1]*pbuf[3]*pbuf[4])/1024)/1024;

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
	} else // XXX - only next line!
#endif
	snprintf(filename, lengthof(filename), "%s" PATHSEP "%s", path, ent->d_name);

	if (stat(filename, sb) != 0) return false;

	return (ent->d_name[0] != '.'); // hidden file
}

bool FiosIsHiddenFile(const struct dirent *ent)
{
   return ent->d_name[0] == '.';
}


const char *FS2OTTD(const char *name) {return name;}
const char *OTTD2FS(const char *name) {return name;}


int module_stop(SceSize args, void *argp)
{
   (void) pspSdkInetTerm();
   return 0;
}

