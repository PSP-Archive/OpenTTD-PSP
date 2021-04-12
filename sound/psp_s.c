/* $Id: psp_s.c 3552 2006-02-05 09:56:04Z tron $ */

#include "../stdafx.h"

#ifdef PSP

#include "../openttd.h"
#include "../driver.h"
#include "../mixer.h"
#include "psp_s.h"
#include <pspaudiolib.h>

#define AUDIO_CHAN 1


static void audioOutCallback(void *buf, unsigned int _reqn)
{
   MxMixSamples(buf, _reqn);
}

static const char *PSPSoundStart(const char * const *parm)
{
   pspAudioSetChannelCallback(AUDIO_CHAN, (void *)&audioOutCallback, NULL);
   pspAudioSetVolume(AUDIO_CHAN, 0x8000, 0x8000);
   return NULL;
}

static void PSPSoundStop(void)
{
   pspAudioEndPre();
   pspAudioEnd();
}

const HalSoundDriver _psp_sound_driver = {
   PSPSoundStart,
   PSPSoundStop,
};

#endif //PSP
