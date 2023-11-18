#ifndef _SOUND_
#define _SOUND_

#include <AL/al.h>
#include <AL/alc.h>
// #include <AL/alut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>

/* sound modes */
#define SDEFAULT 0
#define SLOOP 1

/* sound */
#define NUM_SOUNDS 5
#define NUM_SOURCES 5
#define NUM_BUFFERS 10

/* sound names */
#define MUSIC 0
#define FIRE0 1
#define EXPLOSION0 2
#define THRUST 3
#define CRASH 4

#define SSTOP 0
#define SPLAY 1
#define SPAUSE 2

/* sound actions */

#define VOLSET 0
#define VOLGET 1
#define VOLINC 2

struct Sourcetable {
  ALuint source;
  ALuint buffer;
  int sid;
};

int Sound(int mode, int sid);
int InitSound(void); /* Program init */
int ExitSound(void);
int PlaySound(int sid, int mode, float vol);
int Wait2FinishSound(void);
int CleanBuffers(void);
int StopSound(int sid);
float SetMasterVolume(float vol, int action);
float SetSoundVolume(float vol, int action);
float SetMusicVolume(float vol, int action);

#endif
