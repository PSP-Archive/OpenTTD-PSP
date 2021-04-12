/* $Id: extmidi.c 4692 2006-05-02 19:09:49Z peter1138 $ */

#define TIMIDITYF 1

//#include "../stdafx.h"
#include "../openttd.h"
#include "../sound.h"
#include "../string.h"
#include "../variables.h"
#include "../debug.h"
#include "psp_timidity.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <timidity.h>
#include <pspaudiolib.h>

#define AUDIO_CHAN 2

#define MIDI_STOPED  0
#define MIDI_PLAYING 1


MidIStream *stream;
MidSongOptions options;
MidSong *song;

static int midiStatus = 0;
//static byte _psp_volume = 127;

uint32 songlength;
uint32 currentSongPosition;



static void audioOutCallback(void *buf, unsigned int _reqn)
{
    if (midiStatus == MIDI_PLAYING) {
        DEBUG(driver, 0)("Audio callback reading buffer\n");
        memset(buf, 0, _reqn*4);
        mid_song_read_wave(song, buf, _reqn*4);
    }  else {
        memset(buf, 0, _reqn*4);
    }
}


static const char* pspMidiStart(const char* const * parm)
{
	/* Init Timidity */
   if(mid_init("timidity.cfg") < 0 ) {
      DEBUG(driver, 0)("error initializing timidity\n");
      return NULL;
   }
   DEBUG(driver, 0)("Timidity successfully inizialited\n");

   options.rate = 44100;
   options.format = MID_AUDIO_S16LSB;
   options.channels = 2;
   options.buffer_size = 4096 / (16 * 2 / 8);
   //options.buffer_size = 44100;

   //pspAudioInit();
   pspAudioSetChannelCallback(AUDIO_CHAN, (void *)&audioOutCallback, NULL);
   pspAudioSetVolume(AUDIO_CHAN, 0x8000, 0x8000);

   return NULL;
}

static void pspMidiStop(void)
{
   if (midiStatus == MIDI_PLAYING) {
      if (song != NULL)
         mid_song_free(song);

      midiStatus = MIDI_STOPED;
      song = NULL;
   }

   mid_exit();
   DEBUG(driver, 0)("MIDI engine stopped\n");
}

static void pspMidiPlaySong(const char* filename)
{
    if (midiStatus == MIDI_PLAYING) {
        if (song != NULL)
           mid_song_free(song);

        midiStatus = MIDI_STOPED;
        song = NULL;
    }

    stream = mid_istream_open_file(filename);
    if (stream == NULL) {
      DEBUG(driver, 0)("Could not open music file\n");
      return NULL;
    }

    song = mid_song_load(stream, &options);
    mid_istream_close(stream);
    songlength = mid_song_get_total_time(song);

    if (song == NULL) {
      DEBUG(driver, 0)("Invalid MIDI file\n");
      return NULL;
    }
    DEBUG(driver, 0)("Valid MIDI file\n");

    mid_song_start(song);
    midiStatus = MIDI_PLAYING;
    DEBUG(driver, 0)("MIDI song started\n");
}

static void pspMidiStopSong(void)
{
    if (midiStatus == MIDI_PLAYING) {
        if (song != NULL)
           mid_song_free(song);

        midiStatus = MIDI_STOPED;
        song = NULL;
    }

    DEBUG(driver, 0)("MIDI song stopped\n");
}

static bool pspMidiIsPlaying(void)
{
    if(midiStatus == MIDI_PLAYING) {
        currentSongPosition = mid_song_get_time(song);
        if (currentSongPosition >= songlength) {
            if (song != NULL)
               mid_song_free(song);

            song = NULL;
            midiStatus = MIDI_STOPED;
            currentSongPosition = 0;
        }
    }

    if ( midiStatus == MIDI_PLAYING )
        return true;

    return false;
}

static void pspMidiSetVolume(byte vol)
{
   pspAudioSetVolume(AUDIO_CHAN, ((int)vol) << 8, ((int)vol) << 8);
   DEBUG(driver, 1) ("psp_timidity: set volume to %i (%i)", vol, ((int)vol)<<8);
}


const HalMusicDriver _psptimi_music_driver = {
	pspMidiStart,
	pspMidiStop,
	pspMidiPlaySong,
	pspMidiStopSong,
	pspMidiIsPlaying,
	pspMidiSetVolume,
};

