#ifndef _GENERAL_
#define _GENERAL_

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

/* #define GDK_DISABLE_DEPRECATED */

/*  compile options: */
#define SOUND 1 /* compile with sound */
/*  --compile options: */

#define MODI(a) ((a) > 0 ? (a) : (-a))
#define PI 3.14159265

#define MINORSAVEVERSION \
  "0.85.12" /* the save file version must be greater than this.*/
#define MINOROPTIONSVERSION \
  "0.85.01" /* the options file must be greater than this.*/

#define MAXTEXTLEN 128

/* window properties */
#define DEFAULTWIDTH 1024 /* 900 1024  800 1024 1152 */
#define DEFAULTHEIGHT 550 /* 600  500 550  700  768 */
#define LXFACTOR 1024     /* normalization factor for surface planets */
#define LYFACTOR 550      /* normalization factor for surface planets */
#define PANEL_HEIGHT 20

/* game */
#define CLOSEDUNIVERSE 0
#define NUMSTARS 200
#define GRAVITATION_RANGE2 4000000 /* max. gravity reach */
#define G .1                       /* gravitation constant */
#define DT .42                     /* .35  .42 .35 .42 */
#define RESOURCEFACTOR \
  1.2                   /* velocity factor for getting resource from planets */
#define COSTFACTOR 1.0  /* multiplicative factor to ships cost */
#define DAMAGEFACTOR .7 /* (0.9) multiplicative factor to weapon damage */
#define COSTINC 1.8     /* increment of ships cost when level up */
#define MINLEVELPILOT \
  1 /* min level of a ship to have a survival pod, (only fighters)*/

#define PLANETSKNOWN 0
#define SLOWMACHINE 0

/* default options */
#define MAPAUTOCENTER 1
#define NUMPLAYERS 2
#define NUMGALAXIES 1
#define NUMPLANETS 30 /* num. planets in galaxy */
#define ULX 80000     /*  default universe size */
#define ULY 80000     /*  default universe size */
#define PLAYERNAME "player"

/* max and min option values */
#define MAXNUMPLAYERS 8
#define MINNUMPLAYERS 2
#define MAXNUMGALAXIES 10
#define MINNUMGALAXIES 1
#define MAXNUMPLANETS 200 /* num. planets in galaxy */
#define MINNUMPLANETS 2   /* num. planets in galaxy */
#define MAXULX 1000000    /*  max. universe size */
#define MINULX 10000      /*  min. universe size */
#define MAXNUMTEAMS MAXNUMPLAYERS

/* player control */
#define HUMAN 0
#define COMPUTER 1

/* mode games*/
#define LOCAL 0
#define SERVER 1
#define CLIENT 2

/* navigation modes */
#define ABSOLUTE 0
#define RELATIVE 1

#define RADAR_RANGE 2000
#define RADAR_RANGE2 4000000
#define SECTORSIZE 1410 /* sqrt(RADAR_RANGE2/2) */
#define MAXASTEROIDDISTANCE2 \
  784000000 /* max distance2 an asteroid can be seen (20 sectors )*/

/* FILES */
#define SAVEDIR ".spacezero"
#define SAVETMPFILE "/tmp/tmpspacezero"
#define SAVEFILE0 "save"
#define SAVEFILE1 "savecl"
#define SAVEFILENET "saven"
#define SAVEFILESINGLE SAVEFILE0
#define RECORDFILE "record"
#define OPTIONSFILE "options"
#define KEYBOARDFILE "keymap"
#define LETTERFREQUENCYFILE "letterfrequencytable"

/* planet properties */
#define MINPLANETMASS 35000
#define MAXPLANETMASS 160000
#define MINPLANETGOLD 2000
#define MAXPLANETGOLD 8000
#define MAXPLANETHEIGHT 0.3 * DEFAULTHEIGHT
#define FACTORLANDZONESIZE 0.2 * DEFAULTWIDTH
#define LANDZONEMINSIZE 120
#define LANDZONEMINX 175

/* miscelanea */
#define MINTTL -256 /* with less value ships are forgotten*/

/* Game parametres */

#define DEFAULT 0
#define GULX 1             /* universe size */
#define GULY 2             /* universe size */
#define GWIDTH 3           /* window size */
#define GHEIGHT 4          /* window size */
#define GPANEL 5           /* shell panel size */
#define GNET 6             /* TRUE : NET , FALSE : LOCAL */
#define GMODE 7            /* LOCAL, SERVER, CLIENT */
#define GCOOPERATIVE 8     /* Cooperative mode game */
#define GCOMPCOOPERATIVE 9 /* Computer cooperative mode game */
#define GQUEEN 10          /* Queen mode game */
#define GPIRATES 11        /* Create pirates or not */
#define GENEMYKNOWN 12     /* Enemies are known */
#define GNGALAXIES 13      /* number of galaxies */
#define GNPLAYERS 14       /* number of players */
#define GNPLANETS 15       /* number of planets */
#define GKPLANETS 16       /* TRUE FALSE planets known or unknown */
#define GMUSIC 17          /* TRUE FALSE music */
#define GSOUND 18          /* TRUE FALSE sound */
#define GMUSICVOL 19       /* 0..100 music volume */
#define GSOUNDVOL 20       /* 0..100 sound volume */
#define GPAUSED 21         /* TRUE FALSE game paused */
#define GQUIT 22           /* 0,1,2 really quit? */

/*    Error codes for all functions */

enum SZ_Error {
  /* Used in  Buyship() */
  SZ_OK,
  SZ_OBJNULL,
  SZ_UNKNOWNERROR,
  SZ_OBJNOTLANDED,
  SZ_NOTOWNPLANET,
  SZ_NOTENOUGHGOLD,
  SZ_NOTALLOWED,
  SZ_NOTIMPLEMENTED,
  SZ_NOTENOUGHROOM
};
/*   --Error codes for all functions */

/* messages among players */

struct TextMessage {
  int n;                 /* size of the message */
  int source;            /* source player */
  int dest;              /* dest player   */
  int id;                /* id of the message */
  int print;             /* it is printed */
  int time;              /* time of the text */
  int value;             /* relevance of the message */
  int duration;          /* duration of the text*/
  char text[MAXTEXTLEN]; /* text */
};

struct TextMessageList {
  struct TextMessage info;
  struct TextMessageList* next;
};

struct Game {        /* Game parametres */
  int width, height; /* window size */
  int panel_height;  /* shell height */
  int ulx, uly;      /* Universe size */
  int net;           /* TRUE : NET , FALSE : LOCAL */
  int mode;          /* LOCAL, SERVER, CLIENT */
  int cooperative;
  int compcooperative;
  int queen;
  int pirates;
  int enemyknown;
  int ngalaxies; /* number of galaxies */
  int nplayers;  /* number of players */
  int nplanets;  /* number of planets */
  int kplanets;  /* TRUE FALSE planets known or unknown */
  int paused;    /* TRUE FALSE game paused */
  int music;     /* music ON OFF */
  int sound;     /* sound ON OFF */
  int musicvol;  /* music volume */
  int soundvol;  /* sound volume */
  int quit;      /* 0,1,2 really quit? */
};

typedef struct {
  float x, y;
  float mod;
  float a;
} Vector;

typedef struct {
  int x, y;
} Point;

typedef struct {
  int x, y;
  int width, height;
} Rectangle;

typedef struct {
  Rectangle rect;
  int habitat; /* 0 means free space, >0:  planetid */
} Space;

#endif
