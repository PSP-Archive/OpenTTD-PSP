/* $Id: null_v.c 2748 2005-07-29 16:40:29Z tron $ */

#include "../stdafx.h"
#include "../openttd.h"
#include "../gfx.h"
#include "../variables.h"
#include "../window.h"
#include "null_v.h"

static void* _null_video_mem = NULL;

static const char* NullVideoStart(const char* const* parm)
{
	_screen.width = _screen.pitch = _cur_resolution[0];
	_screen.height = _cur_resolution[1];
	_null_video_mem = malloc(_cur_resolution[0] * _cur_resolution[1]);
	return NULL;
}

static void NullVideoStop(void) { free(_null_video_mem); }

static void NullVideoMakeDirty(int left, int top, int width, int height) {}

static void NullVideoMainLoop(void)
{
	uint i;

	for (i = 0; i < 1000; i++) {
		GameLoop();
		_screen.dst_ptr = _null_video_mem;
		UpdateWindows();
	}
}

static bool NullVideoChangeRes(int w, int h) { return false; }
static void NullVideoFullScreen(bool fs) {}

const HalVideoDriver _null_video_driver = {
	NullVideoStart,
	NullVideoStop,
	NullVideoMakeDirty,
	NullVideoMainLoop,
	NullVideoChangeRes,
	NullVideoFullScreen,
};
