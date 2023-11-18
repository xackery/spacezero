#include <AL/al.h>
#include <AL/alc.h>
#include <vorbis/vorbisfile.h>

/* Stream states */
#define SS_STOP 0
#define SS_PLAY 1
#define SS_PAUSE 2

/* Steam modes */
#define SM_ONCE 0
#define SM_LOOP 1

/* stream orders */

#define SO_NONE 0
#define SO_SETVOL 1
#define SO_PLAY 2
#define SO_PAUSE 3
#define SO_STOP 4

struct StreamedSound {
  OggVorbis_File* ogg_file;
  ALenum format;
  ALfloat rate;
  ALuint* streambuffers;
  ALuint* streamsource;
  int nbuffers;
  int buffersize;
  int mode;
  int state;
  int order;
  float value;
};

struct StreamedSound* StreamSound(char* filename, int mode);

int ExitStreamSound(void);
