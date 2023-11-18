#include "spacezero.h"
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "ai.h"
#include "clock.h"
#include "functions.h"
#include "graphics.h"
#include "locales.h"
#include "objects.h"
#include "planetnames.h"
#include "players.h"
#include "randomnamegen.h"
#include "save.h"
#include "sectors.h"
#include "shell.h"
#include "sound.h"
#include "spacecomm.h"
// #include "snow.h"

extern int g_objid; /* id of the objects */
extern struct Global glocal;
extern GtkWidget* d_a;
extern GtkWidget* win_main;
extern GdkFont* gfont;
extern GdkPixmap* pixmap;
extern GdkPixmap* pixmap_stat;
extern GdkGC* gcolors[];

int gdrawmenu = TRUE;
struct Draw gdraw;

int actual_player, actual_player0;

int record = 0;
int nav_mode = RELATIVE;

int p_time = 0;
int g_nobjsend = 0;
int g_nshotsend = 0;
int g_nobjmaxsend = 0;
int g_nobjtype[6] = {0, 0, 0, 0, 0, 0};
int gameover = FALSE;
int observeenemies = FALSE;

char version[64] = {"0.86.01"};
char copyleft[] = "";
char TITLE[64] = "SpaceZero  ";
char last_revision[] = {"Nov 2022"};

Object* ship_c; /* ship controled by keyboard */
Object* cv;     /* coordinates center */
int fobj[4];
sem_t sem_barrier, sem_barrier1;

int g_cont;
time_t gtime0;
double fps; /* frames / second*/

int order2thread;

int* cell;
static struct timeval init_time;

extern struct Buffer buffer1, buffer2; /* buffers used in comm. */

extern struct HeadObjList listheadobjs; /* list of all objects */
struct HeadObjList listheadplanets;     /* list of all planets */
struct HeadObjList* listheadcontainer; /* lists of objects that contain objects:
                                          free space and planets*/
struct HeadObjList* listheadkplanets;  /* lists of planets known by players */
struct HeadObjList listheadplayer;     /* list of objects of each player */

struct CharListHead gameloglist; /* list of all game messages */
struct Window windowgamelog;

struct TextMessageList listheadtext;
extern struct Parametres param;
Vector r_rel;

struct Habitat habitat;

char clientname[MAXTEXTLEN];

/*  sound */
int soundenabled = TRUE;
/* -- sound */

char* savefile;
char* recordfile;
char* optionsfile;
char* keyboardfile;

struct MenuHead* menuhead;

/*void signal_handler(int ,siginfo_t *,void *);*/
void int_handler(int);
void segfault_handler(int);

int main(int argc, char* argv[]) {
  /*
    version 2
    TODO reorganized this
  */

  GtkWidget* drawing_area;

  char title[64] = "";
  int state;
  struct sigaction sa;
  int width, height;

  srand(time(NULL));

  /******** signals ***********/
  /*
    sa.sa_sigaction=signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=SA_SIGINFO;
    if(sigaction(SIGNAL0,&sa,NULL)){
    perror("sigaction");
    exit(-1);
    }
  */

  sa.sa_handler = int_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(-1);
  }

  /* sa.sa_handler=segfault_handler; */
  /* sigemptyset(&sa.sa_mask); */
  /* sa.sa_flags=0; */
  /* if(sigaction(SIGSEGV,&sa,NULL)){ */
  /*   perror("sigaction"); */
  /*   exit(-1); */
  /* } */

  /******** --signals ***********/

  sem_init(&sem_barrier, 0, 0);
  sem_init(&sem_barrier1, 0, 0);

  MemUsed(MRESET, 0);

  Keystrokes(RESET, NULL, NULL);

  PrintWarnings(version);
  optionsfile = CreateOptionsFile();
  recordfile = CreateRecordFile();
  keyboardfile = CreateKeyboardFile();

  /*********** read options file and checking command line options
   * *************/
  state = Arguments(argc, argv, optionsfile);
  if (state) {
    Usage(version, last_revision);
    exit(-1);
  }

  /****** checking file options *********/
  if (CheckArgs()) {
    fprintf(stderr, "ERROR in arguments, exiting...\n");
    exit(-1);
  }

  PrintArguments("Game arguments:");

  /* locales*/
  GetLocales(param.lang);

  if (GameParametres(GET, GNET, 0)) {
    printf("NET: yes\n");
  } else {
    printf("NET: no\n");
  }

  /***** set game options ******/

  GameParametres(SET, GULX, param.ul);
  GameParametres(SET, GCOOPERATIVE, param.cooperative);
  GameParametres(SET, GCOMPCOOPERATIVE, param.compcooperative);
  GameParametres(SET, GQUEEN, param.queen);
  GameParametres(SET, GPIRATES, param.pirates);
  GameParametres(SET, GENEMYKNOWN, param.enemyknown);
  GameParametres(SET, GNGALAXIES, param.ngalaxies);
  GameParametres(SET, GNPLAYERS, param.nplayers);
  GameParametres(SET, GNPLANETS, param.nplanets);

  /***** user defined keys ****/

  printf("Setting default user keys\n");
  SetDefaultUserKeys();
  printf("Loading user keys\n");
  LoadUserKeys(keyboardfile);

  Clock(0, CL_CLEAR);
  Clock(0, CL_START);

  /********** Graphics initialization **********/
  gtk_init(&argc, &argv);

  MakeTitle(param, title);
  GetGeom(param.geom, &width, &height);
  printf("W: %d H: %d \n", width, height);
  drawing_area = InitGraphics(title, optionsfile, width, height, param);

  printf("W: %d H: %d \n", drawing_area->allocation.width,
         drawing_area->allocation.height);

  SetDefaultKeyValues(1);

  menuhead = CreateMenu();
  menuhead->active = TRUE;

  if (param.fontlist) {
    printf("==============\n");
    printf("system font list:\n");
    PrintFontNames(10000);
    printf("==============\n");
  }

  /********** --Graphics initialization *********/

#if SOUND
  /********* sound initialization *********/
  soundenabled = TRUE;
  if (InitSound() != 0) {
    fprintf(stderr, "Error initializing sound, sound disabled Error id:%d\n",
            state);
    soundenabled = FALSE;
    param.sound = 0;
    param.music = 0;
    GameParametres(SET, GMUSIC, param.sound);
    GameParametres(SET, GSOUND, param.music);
    Play(NULL, -1, 0); /* disable sound */
  }

  if (soundenabled == TRUE) {
    float master = 0;
    float music = 0, sound = 0;

    if (param.soundvol > param.musicvol) {
      master = (float)param.soundvol / 100;
      sound = 1.0;
      music = (float)param.musicvol / param.soundvol;
    } else {
      master = (float)param.musicvol / 100;
      music = 1.0;
      sound = (float)param.soundvol / param.musicvol;
    }

    SetMasterVolume(master, VOLSET);

    SetSoundVolume(sound, VOLSET);
    PlaySound(MUSIC, SLOOP, 1);
    if (param.music) {
      SetMusicVolume(music, VOLSET);
    } else {
      Sound(SSTOP, MUSIC);
      printf("Music is off\n");
    }
  }

  /********* --sound initialization *********/
#endif

  /******** what stuff to draw *********/
  gdraw.main = FALSE;
  gdraw.menu = TRUE;
  gdraw.map = FALSE;
  gdraw.shiplist = FALSE;
  gdraw.stats = FALSE;
  gdraw.order = FALSE;
  gdraw.info = FALSE;
  gdraw.crash = FALSE;
  gdraw.volume = FALSE;

  /******** --what stuff to draw *********/

  /***** game log initialization *****/

  gameloglist.n = 0;
  gameloglist.max = 200;
  gameloglist.first = 0;
  gameloglist.next = NULL;

  windowgamelog.type = 0;
  windowgamelog.active = 0;
  windowgamelog.x = 0;
  windowgamelog.y = 0;
  windowgamelog.width = 0;
  windowgamelog.height = 0;
  windowgamelog.scrollbar.active = 1;
  windowgamelog.scrollbar.width = 15;
  windowgamelog.scrollbar.pos = 0;
  windowgamelog.scrollbar.n = 0;
  windowgamelog.data = &gameloglist;

  gtk_timeout_add((int)(DT * 50), MenuLoop,
                  (gpointer)drawing_area); /* 42 DT=0.42 in general.h*/

  gtk_timeout_add((int)(DT * 100), MainLoop,
                  (gpointer)drawing_area); /* 42 DT=0.42 in general.h*/

  gettimeofday(&init_time, NULL);
  gtk_main();

  /********* game resume **********/
  {
    struct Player* player = NULL;
    if (GameParametres(GET, GNPLAYERS, 0) > 0) {
      player = GetPlayers();
      printf("\ntotal points: %d record: %d\n", player[1].points, record);
    }
    printf("******************************************************\n");
    printf("%sversion %s  %s\n", TITLE, version, last_revision);
    printf(
        "Please, send bugs and suggestions to: mrevenga at users dot "
        "sourceforge dot net\n");
    printf("Homepage:  http://spacezero.sourceforge.net/\n");

    SaveRecord(recordfile, player, record);
  }
  return 0;
} /*  --main */

gint MenuLoop(gpointer data) {
  struct MenuHead* actualmenu;
  GdkRectangle update_rect;
  GtkWidget* drawing_area = (GtkWidget*)data;
  int x, y;
  int width, height;
  char point[128];

  int status;
  static int cont = 0;
  static GdkFont *font, *fontmenu;
  static int swfont = 0;
  int swexit = 0;
  int swsaveoptions = 0;

  if (gdraw.menu == FALSE)
    return (TRUE);

  if (cont == 0) {
    SetDefaultKeyValues(1);
  }
  cont++;

  if ((cont % 20) == 0) {
    gdrawmenu = TRUE;
  }

  if (!gdrawmenu)
    return (TRUE);
  gdrawmenu = FALSE;

  key_eval();

  width = GameParametres(GET, GWIDTH, 0);
  height = GameParametres(GET, GHEIGHT, 0);

  if (swfont == 0) {
    font = InitFontsMenu("");
    swfont++;
  }
  if (height < 400)
    fontmenu = gfont;
  else
    fontmenu = font;

  /* clear window */

  gdk_draw_rectangle(pixmap, GetColor(BLACK), TRUE, 0, 0,
                     drawing_area->allocation.width,
                     drawing_area->allocation.height);

  /* printf something */

  DrawStars(pixmap, nav_mode, 0, 0);

  x = width / 2;
  y = height / 9;
  sprintf(point, "SpaceZero");
  DrawMessageBox(pixmap, font, point, x, y, MBOXDEFAULT);

  menuhead->active = TRUE;
  actualmenu = SelectMenu(menuhead);
  status = UpdateMenu(menuhead, actualmenu);

  x = width / 8;
  y = (1.0 / 3) * height;

  XPrintMenuHead(pixmap, fontmenu, actualmenu, x, y);
  /* show window */

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.width = drawing_area->allocation.width;
  update_rect.height = drawing_area->allocation.height;
  gtk_widget_draw(drawing_area, &update_rect); /*  deprecated */

  switch (status) {
    case 0:
      break;
    case ITEM_start:
      swsaveoptions = 1;
      swexit = 1;
      break;
    case ITEM_quit:
      SetGameParametres(param);
      GameParametres(SET, GQUIT, 2);
      gdraw.menu = FALSE;
      gdraw.main = TRUE;
      Keystrokes(RESET, NULL, NULL);
      SetDefaultKeyValues(1);
      return (TRUE);
      break;
    case ITEM_default:
      printf("loading default \n");
      SetDefaultParamValues();
      SetDefaultUserKeys();
      break;
    case ITEM_server:
      param.server = TRUE;
      param.client = FALSE;
      swsaveoptions = 1;
      swexit = 1;
      printf("start server....\n");
      break;
    case ITEM_client:
      param.server = FALSE;
      param.client = TRUE;
      swexit = 1;
      printf("start client....\n");
      break;
    case MENUESC:
      break;
    default:
      break;
  }

  if (param.menu == FALSE)
    swexit++;

  if (swexit) {
    char title[64] = "";
    gdraw.menu = FALSE;
    gdraw.main = TRUE;
    Keystrokes(RESET, NULL, NULL);
    printf("saving game param...\n");
    SetGameParametres(param);
    MakeTitle(param, title);
    gtk_window_set_title(GTK_WINDOW(win_main), title);
    if (swsaveoptions) {
      printf("saving options...\n");
      SaveParamOptions(optionsfile, &param);
      SaveUserKeys(keyboardfile);
    }
    /* activating sound ??*/

    if (soundenabled == TRUE) {
      float master = 0;
      float music = 0, sound = 0;

      if (param.soundvol > param.musicvol) {
        master = (float)param.soundvol / 100;
        sound = 1.0;
        music = (float)param.musicvol / param.soundvol;
      } else {
        master = (float)param.musicvol / 100;
        music = 1.0;
        sound = (float)param.soundvol / param.musicvol;
      }

      SetMasterVolume(master, VOLSET);
      SetSoundVolume(sound, VOLSET);
      if (param.music) {
        SetMusicVolume(music, VOLSET);
      } else {
        Sound(SSTOP, MUSIC);
        printf("Music is off\n");
      }
    }

    printf("exiting menu...\n");
  }
  return (TRUE);
}

gint MainLoop(gpointer data) {
  /*
    Main gtk loop. executed 24 times by second.

  */
  int i, j;
  int drawmap = FALSE;
  static int cont = 0;
  float x0, y0;
  static int lasttimepirates = 2000;
  static int lasttimeasteroids = 0;
  int timenow;
  Object* cv0; /* coordinates center */
  GtkWidget* drawing_area = (GtkWidget*)data;
  GdkRectangle update_rect;
  static int paused = 0;
  static int swpaused = 0;
  int gwidth, gheight;
  static int ulx, uly;
  int proc;
  int swcomm = 0; /* TRUE if in time step has communication */
  int swmess = 0; /* show a message */
  char point[128];
  char pointmess[128];
  int loadsw = 0;
  static int sw = 0;
  int nx, ny;

  double fpst = 0;
  static double fpstt = 0;
  static int fpscont = 0;
  static struct timeval time0, time1;

  struct Player* plyr;
  struct Keys* key;

  static int stat_sw = 0;
  static char shellcad[MAXTEXTLEN] = "";

  if (gdraw.main == FALSE)
    return (TRUE);

  gettimeofday(&time0, NULL);
  timenow = GetTime();
  Clock(2, CL_CLEAR);
  Clock(2, CL_START);

  if (gtime0 == 0) {
    gtime0 = time(NULL);
  }

  if (!sw) { /* firsttime */
    savefile = CreateSaveFile(param.server, param.client);

    fprintf(stdout, "save file: %s\n", savefile);
    fprintf(stdout, "record file: %s\n", recordfile);
    fprintf(stdout, "options file: %s\n", optionsfile);

    printf("init game vars...\n");
    InitGameVars(); /* */

    printf("... init game vars\n");
    if (GameParametres(GET, GQUIT, 0) == 2) {
      Quit(NULL, NULL);
    }

    gfont = InitFonts(param.font);
    if (gfont == NULL) {
      GameParametres(SET, GPANEL, PANEL_HEIGHT);
    } else {
      GameParametres(SET, GPANEL, 2 * gdk_text_height(gfont, "pL", 2));
    }
    GameParametres(
        SET, GHEIGHT,
        GameParametres(GET, GHEIGHT, 0) - GameParametres(GET, GPANEL, 0));

    gwidth = GameParametres(GET, GWIDTH, 0);
    gheight = GameParametres(GET, GHEIGHT, 0);

    GameParametres(GET, GPANEL, 0);
#ifndef GTK12
    gtk_window_resize(GTK_WINDOW(win_main), gwidth,
                      gheight + GameParametres(GET, GPANEL, 0));
#endif
    GameParametres(
        SET, GHEIGHT,
        drawing_area->allocation.height - GameParametres(GET, GPANEL, 0));
    GameParametres(SET, GWIDTH, drawing_area->allocation.width);

    gdraw.menu = FALSE;

    /* statistics */
    InitStatistics();

    /* snow */

    CreateSnow(LXFACTOR, LYFACTOR);

    sw++;
  }

  plyr = GetPlayers();

  /***** fps *****/
  Clock(0, CL_STOP);
  fpst = Clock(0, CL_READ);
  fpstt += fpst;
  fpscont++;
  Clock(0, CL_CLEAR);
  Clock(0, CL_START);
  fps = fpscont / fpstt;
  if (fpscont == 20) {
    fpscont = 10;
    fpstt /= 2;
  }

  /***** --fps *****/

  ulx = GameParametres(GET, GULX, 0);
  uly = GameParametres(GET, GULY, 0);

  gwidth = GameParametres(GET, GWIDTH, 0);
  gheight = GameParametres(GET, GHEIGHT, 0);

  proc = GetProc();

  /*****CELLON*****/

  nx = GameParametres(GET, GULX, 0) / 2000;
  ny = GameParametres(GET, GULY, 0) / 2000;
  for (i = 0; i < nx * ny; i++) {
    cell[i] = 0;
  }
  UpdateCell(&listheadobjs, cell);

  /*****--CELLON*****/

  key_eval();

  /* Create some lists */
  {
    int pmodified = 0;
    static int lasttimeupdate = 0;

    if (lasttimeupdate > 60) {
      lasttimeupdate = 0;
      pmodified++;
    }

    for (i = 0; i < GameParametres(GET, GNPLAYERS, 0) + 2; i++) {
      if (plyr[i].status == PLAYERMODIFIED) {
        pmodified++;
      }
    }

    if (listheadplayer.update) {
      DestroyObjList(&listheadplayer);
      listheadplayer.list = NULL;
      listheadplayer.n = 0;
      CreatePlayerList(listheadobjs, &listheadplayer, actual_player);
      listheadplayer.update = 0;
    }

    if (pmodified) {
      lasttimeupdate = 0;
      /* update createplayerlist only if necesary HERE  9.44% CPU*/
      /*
        the list must be update when a ship is created or destroyed : mandatory
        if habitat changes. no mandatory
        HERE actualizar al cambiar habitat
      */

      for (i = 0; i < GameParametres(GET, GNPLANETS, 0) + 1; i++) {
        DestroyObjList(&listheadcontainer[i]);
        listheadcontainer[i].list = NULL;
        listheadcontainer[i].n = 0;
      }

      for (i = 0; i < GameParametres(GET, GNPLAYERS, 0) + 2; i++) {
        DestroyObjList(&listheadkplanets[i]);
        listheadkplanets[i].list = NULL;
        listheadkplanets[i].n = 0;
      }
      CreateContainerLists(&listheadobjs, listheadcontainer);
      CreatekplanetsLists(&listheadobjs, listheadkplanets);

      for (i = 0; i < GameParametres(GET, GNPLAYERS, 0) + 2; i++) {
        if (plyr[i].status >= PLAYERACTIVE)
          plyr[i].status = PLAYERACTIVE;
      }
    }
    lasttimeupdate++;
  }
  /* --Create some lists */

  /* messages */
  swmess = 0;

  /* GAME OVER */

  if (GameParametres(GET, GPAUSED, 0) == FALSE) {
    /*    if(GameOver(&listheadplayer,players,actual_player)){
          gameover=TRUE;
          observeenemies=TRUE;
          }
    */
    if (plyr[actual_player].status == PLAYERDEAD) {
      gameover = TRUE;
      observeenemies = TRUE;
    } else {
      gameover = FALSE;
      /* observeenemies=FALSE; */
    }
  }

  if (gameover == TRUE) {
    sprintf(pointmess, "GAME OVER");
    swmess++;
  }

  /* game paused */
  if (GameParametres(GET, GPAUSED, 0) == TRUE) {
    sprintf(pointmess, "PAUSED    (press p to continue)");
    swmess++;
  }

  /* game quit */
  if (GameParametres(GET, GQUIT, 0) == 1) {
    sprintf(pointmess, "Really QUIT?  ( y/n )");
    swmess++;
  }

  if (swmess) {
    DrawMessageBox(pixmap, gfont, pointmess, gwidth / 2, 0.3 * gheight,
                   MBOXBORDER);
  }

  if (GameParametres(GET, GPAUSED, 0) == TRUE) {
    int x, y, width, height;
    int textw, texth;

    /* update window */
    if (swmess) {
      if (gfont != NULL) {
        textw = gdk_text_width(gfont, pointmess, strlen(pointmess));
        texth = gdk_text_height(gfont, pointmess, strlen(pointmess));
      } else {
        texth = 12;
        textw = 12;
      }
      width = textw + 2 * texth + 1;
      height = 2 * texth;

      x = gwidth / 2 - width / 2;
      y = 0.3 * gheight - height / 2;
      if (x < 10)
        x = 10;

      update_rect.x = x;
      update_rect.y = y;
      update_rect.width = width;
      update_rect.height = height;
      gtk_widget_draw(drawing_area, &update_rect);
    }
    if (swpaused >= NETSTEP) {
      return (TRUE); /* savepause */
    }
    if (GameParametres(GET, GNET, 0) == TRUE) {
      sprintf(point, "Game PAUSED");
      SendTextMessage(point);
    }
    swpaused++;
  } else {
    swpaused = 0;
  }

  /****************** if not paused *****************/

  drawmap = FALSE;
  if (!(cont % 2))
    drawmap = TRUE;

  /* ai */

  for (i = 1; i < GameParametres(GET, GNPLAYERS, 0) + 2; i++) {
    if (plyr[i].control == COMPUTER && proc == plyr[i].proc) {
      ControlCenter(&listheadobjs, &plyr[i]);
    }
  }

  UpdateObjs(); /* new positions */
  for (i = 1; i < GameParametres(GET, GNPLAYERS, 0) + 2; i++) {
    if (proc == plyr[i].proc) {
      plyr[i].ttl--;
      if (plyr[i].ttl <= 0)
        plyr[i].modified = SENDPLAYERMOD;
    }
  }

  for (i = 0; i < GameParametres(GET, GNPLANETS, 0) + 1; i++) {
    Collision(&listheadcontainer[i]); /* interact among objects */
  }

  GetGold();

  /**** create pirates *****/
  if (GameParametres(GET, GPIRATES, 0) == TRUE) {
    if ((plyr[GameParametres(GET, GNPLAYERS, 0) + 1].nplanets < 5)) {
      if (proc ==
          plyr[GameParametres(GET, GNPLAYERS, 0) + 1].proc) { /*  Send TO ai */
        if (lasttimepirates > timenow) {
          lasttimepirates = timenow;
        }
        if (timenow - lasttimepirates > 2000) { /* 2000 */
          if (((20000.0 * rand()) / RAND_MAX) <= 1) {
            char text[MAXTEXTLEN];
            float level = 0;
            int np = 4;
            level = timenow / 30000.0; /* increase every 20 min */
            lasttimepirates = timenow;
            x0 = ulx * Random(-1) - ulx / 2;
            y0 = uly * Random(-1) - uly / 2;
            np = 4 + (int)level;
            np = np > 12 ? 12 : np;
            CreatePirates(&listheadobjs, np, x0, y0, level);

            snprintf(text, MAXTEXTLEN, "%s: %d %d",
                     GetLocale(L_PIRATESATSECTOR), (int)(x0 / SECTORSIZE),
                     (int)(y0 / SECTORSIZE));

            if (!Add2TextMessageList(&listheadtext, text, 0, -1, 0, 100, 0)) {
              Add2CharListWindow(&gameloglist, text, 0, &windowgamelog);
            }
            if (GameParametres(GET, GNET, 0) == TRUE) {
              SendTextMessage(text);
            }
          }
        }
      }
    }
  }
  /**** --create pirates *****/

  /**** create asteroids *****/
  if (proc ==
      plyr[GameParametres(GET, GNPLAYERS, 0) + 1].proc) { /*  Send TO ai */

    lasttimeasteroids =
        lasttimeasteroids > timenow ? timenow : lasttimeasteroids;
    if (timenow - lasttimeasteroids > 3000) {
      if (((11000.0 * rand()) / RAND_MAX) <= 1) { /* every 10 minutes */
        char text[MAXTEXTLEN];
        float factor;

        factor = (float)GameParametres(GET, GULX, 0) / 100000.0;
        factor *= factor;
        factor = 2 * (factor + 1);
        lasttimeasteroids = timenow;
        for (i = 0; i < factor; i++) {
          x0 = ulx * Random(-1) - ulx / 2;
          y0 = uly * Random(-1) - uly / 2;
          /* Only create asteroids close to a ship */
          if (Distance2NearestShip(&listheadobjs, -1, x0, y0) <
              MAXASTEROIDDISTANCE2) { /* 20 sectors */
            CreateAsteroids(&listheadobjs, 6, x0, y0);

            for (j = 1; j < GameParametres(GET, GNPLAYERS, 0) + 2; j++) {
              if (Distance2NearestShipLessThan(&listheadobjs, j, x0, y0,
                                               MAXASTEROIDDISTANCE2)) {
                snprintf(text, MAXTEXTLEN, "%s: %d %d",
                         GetLocale(L_ASTEROIDSATSECTOR), (int)(x0 / SECTORSIZE),
                         (int)(y0 / SECTORSIZE));
                if (!Add2TextMessageList(&listheadtext, text, 0, j, 0, 100,
                                         0)) {
                  Add2CharListWindow(&gameloglist, text, 1, &windowgamelog);
                }
                if (GameParametres(GET, GNET, 0) == TRUE) {
                  SendTextMessage(text);
                }
              }
            }
          }
        }
      }
    }
  }

  /**** --create asteroids *****/

  if (swpaused == TRUE) {
    printf("PAUSED\n");
    fflush(NULL);
  }

  /*** check for cargo objects ****/
  /*   CargoCheck(&listheadobjs,cv); */
  /*** --check for cargo objects ****/

  /* synchronization with comm threads */

  swcomm = FALSE;
  if (GameParametres(GET, GNET, 0) == TRUE) {
    if (!(cont % NETSTEP)) {
      CargoCheck(&listheadobjs, cv);
      CheckModifiedPre(&listheadobjs, proc);
      Setttl0(&listheadobjs);

      NetComm(); /* net communication */

      CheckModifiedPost(&listheadobjs, proc);
      Setttl(&listheadobjs, -1);
      swcomm = TRUE;
    }
  } else {
    swcomm = TRUE;
  }

  if (swcomm == TRUE) {
    GetPoints(&listheadobjs, proc, plyr); /* points and experience */
  }

  if (GameParametres(GET, GKPLANETS, 0) == FALSE) {
    UpdateSectors(listheadobjs);
  }

  key = GetKeys();

  if (key->load == TRUE && swcomm == TRUE) {
    key->load = FALSE;
    key->save = FALSE;

    DestroyObjList(
        &listheadplayer); /* HERE verify with DestroyAllObjs() in execload() */
    listheadplayer.n = 0;
    listheadplayer.list = NULL;

    if (ExecLoad(savefile) == 0) {
      printf("EXECLOAD() done\n");
      loadsw = 1;
      plyr = GetPlayers();
      p_time = timenow;
      gameover = FALSE;
      observeenemies = FALSE;

      CreatePlanetList(listheadobjs, &listheadplanets);
      /*      Density(); */

      { /* deleting the message list */
        struct TextMessageList* lh;
        lh = &listheadtext;
        while (lh->next != NULL) {
          lh->next->info.duration = 0;
          lh = lh->next;
        }
      }
      DestroyCharList(&gameloglist);
      CheckGame("Checking game after load...", 1);
      printf("done\n");
    }
    CreatePlayerList(listheadobjs, &listheadplayer, actual_player);
    listheadplayer.update = 0;
    PrintGameOptions();
    /* print teams */
    PrintTeams();
  } /* if(key->load==TRUE && swcomm==TRUE){ */

  if (key->save == TRUE && swcomm == TRUE) { /* savepause */
    char text[MAXTEXTLEN];
    key->load = FALSE;
    key->save = FALSE;

    { /* cargo lists */
      struct ObjList* ls;
      int cargomass = 0;
      ls = listheadobjs.list;
      i = 0;
      while (ls != NULL) {
        if (ls->obj->cargo.n > 0) {
          cargomass = ls->obj->cargo.mass;
          CargoBuild(ls->obj);
          ls->obj->cargo.mass = cargomass;
        }
        ls = ls->next;
      }
    } /* --cargo lists */

    CheckGame("Checking game before save...", 1);
    printf("done\n");

    if (ExecSave(listheadobjs, savefile) == 0) {
      snprintf(text, MAXTEXTLEN, "%s.", GetLocale(L_GAMESAVED));
      if (!Add2TextMessageList(&listheadtext, text, 0, -1, 0, 100, 0)) {
        Add2CharListWindow(&gameloglist, text, 0, &windowgamelog);
      }
      if (GameParametres(GET, GNET, 0) == TRUE) {
        SendTextMessage(text);
      }
    } else {
      fprintf(stderr, "Error in MainLoop(): I can't open %s\n", savefile);
    }
  }

  if (cv != NULL) {
    if (nav_mode == RELATIVE) {
      r_rel.x = cv->x;
      r_rel.y = cv->y;
    } else {
      if (cv->x - r_rel.x > gwidth / 2)
        r_rel.x += gwidth;
      if (cv->x - r_rel.x < -gwidth / 2)
        r_rel.x -= gwidth;

      if (cv->y - r_rel.y > gheight / 2)
        r_rel.y += gheight;
      if (cv->y - r_rel.y < -gheight / 2)
        r_rel.y -= gheight;
    }
  }

  /* Drawing window */

  /*  what to draw */
  if (key->esc == TRUE) {
    key->f5 = FALSE;
    key->f6 = FALSE;
    key->f7 = FALSE;
    gdraw.shiplist = FALSE;
    gdraw.gamelog = FALSE;
    gdraw.stats = FALSE;
  }

  if (key->f5 == TRUE) {
    key->f7 = FALSE;
    gdraw.shiplist = TRUE;
    gdraw.gamelog = FALSE;
  } else {
    gdraw.shiplist = FALSE;
  }
  if (key->f6 == TRUE) {
    gdraw.stats = TRUE;
  } else {
    gdraw.stats = FALSE;
  }

  if (key->f7 == TRUE) {
    windowgamelog.active = TRUE;
    gdraw.gamelog = TRUE;
    gdraw.shiplist = FALSE;
  } else {
    gdraw.gamelog = FALSE;
    windowgamelog.active = FALSE;
    windowgamelog.scrollbar.n = 0;
  }
  if (gdraw.shiplist || gdraw.gamelog) {
    gdraw.info = FALSE;
  } else {
    gdraw.info = TRUE;
  }

  if (key->mright) {
    key->order.state = TRUE;
    key->g = TRUE;
  }

  if (key->ctrl == TRUE && key->w == TRUE) {
    key->order.state = TRUE;
  }

  if (key->order.state == TRUE) {
    gdraw.order = TRUE;
  }
  if (key->map.state == TRUE) {
    if (!(Shell(1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == 2 &&
          Shell(2, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == WRITE)) {
      gdraw.map = gdraw.map == TRUE ? FALSE : TRUE;
    }
    key->map.state = FALSE;
  }
  if (cv == NULL)
    gdraw.map = FALSE;

  /*--  what to draw */

  /* game window*/
  if (paused == 0) {
    /* clear window */
    if (gdraw.map == TRUE) {
      if (drawmap) {
        gdk_draw_rectangle(pixmap, GetColor(BLACK), TRUE, 0, 0,
                           drawing_area->allocation.width,
                           drawing_area->allocation.height);
      }
    } else {
      gdk_draw_rectangle(pixmap, GetColor(BLACK), TRUE, 0, 0,
                         drawing_area->allocation.width,
                         drawing_area->allocation.height);
    }
    /* --clear window */

    /* drawing sound volume  */

    if (gdraw.volume == TRUE) {
      static int contvolume = 100;
      int x, y;
      if (contvolume == 0)
        contvolume = 100;
      contvolume--;
      if (contvolume == 0)
        gdraw.volume = FALSE;
      x = gwidth - 65;
      y = gheight - 15;
      DrawBarBox(pixmap, GetColor(RED), GetColor(RED), x, y, 60, 10,
                 SetMasterVolume(0, VOLGET));
    }

    if (gdraw.map == TRUE) {
      if (drawmap) {
        DrawMap(pixmap, actual_player, listheadobjs, cv, ulx);
      }
    } else {
      if (cv != NULL) {
        switch (habitat.type) {
          case H_SPACE:
            DrawStars(pixmap, nav_mode, r_rel.x, r_rel.y);
            DrawRadar(pixmap, cv, &listheadobjs, gdraw.crash);
            break;
          case H_PLANET:
            DrawPlanetSurface(pixmap, habitat.obj->planet,
                              gcolors[plyr[habitat.obj->player].color]);

            /* snow */
            if (cv->in != NULL) {
              if (cv->in->id == 3) {
                UpdateFlakes(cv->in->planet);
                DrawFlakes(pixmap, GetColor(WHITE), gwidth, gheight);
              }
            }
            break;
          case H_SHIP:
            /**TODO draw ship ***/
            DrawSpaceShip(pixmap, cv, &listheadobjs);
            /*****/
            break;
          default:
            /* draw nothing */
            break;
        }
      }

      DrawObjs(pixmap, &listheadobjs, habitat, cv, r_rel);

      if (cv != NULL) {
        if (cv->accel > 0) {
          Play(cv, THRUST, 1.0 * cv->accel / cv->engine.a_max);
        }
      }
    }

    if (gdraw.crash)
      gdraw.crash = 0;

    if (gdraw.shiplist == TRUE) {
      DrawPlayerList(pixmap, actual_player, &listheadplayer, cv,
                     loadsw || !(timenow % 20));
    }

    if (gdraw.gamelog == TRUE) {
      DrawWindow(pixmap, gfont, GetColor(WHITE), 10, 10, 1, &windowgamelog);
    }

    DrawInfo(pixmap, cv, &gdraw, &listheadobjs, &listheadtext);
  } /* if(paused==0) */

  if (!(cont % 24)) { /* statistics */
    UpdateStatistics();
    /* PrintStatistics(); */
  }

  /**** draw stats ****/
  if (gdraw.stats == TRUE) {
    Rectangle rect;
    int x;
    stat_sw = 1;
    rect.x = 5;
    rect.y = 5;
    rect.width = 200;
    rect.height = 100;
    x = DrawGameStatistics(pixmap, plyr) - 5;

    rect.height = 100;
    rect.width = x - 5;
    if (rect.width > 195)
      rect.width = 195;
    if (rect.width < 0)
      rect.width = 0;

    Statistics_Draw(pixmap, &rect);

  } else {
    if (stat_sw == 1) {
      if (pixmap) {
        gdk_pixmap_unref(pixmap);
      }
      pixmap = gdk_pixmap_new(d_a->window, d_a->allocation.width,
                              d_a->allocation.height, -1);
      stat_sw = 0;
    }
  }
  /**** --draw stats ****/

  if (swmess) {
    DrawMessageBox(pixmap, gfont, pointmess, gwidth / 2, 0.3 * gheight,
                   MBOXBORDER);
  }

  /* Draw Shell */

  if (gdraw.order == TRUE) { /* QWERTY */
    gdk_draw_line(pixmap, GetColor(WHITE), 0, gheight, gwidth, gheight);

    cv0 = cv; /* coordinates center */
    ShellTitle(2, NULL, NULL, NULL, NULL, 0, 0);
    Shell(0, pixmap, GetColor(GREEN), gfont, &listheadobjs,
          &plyr[actual_player], key, &cv, shellcad);
    DrawString(pixmap, gfont, GetColor(GREEN), 10,
               GameParametres(GET, GHEIGHT, 0) +
                   GameParametres(GET, GPANEL, 0) / 2 + 4,
               shellcad);
    if (key->order.state == FALSE)
      gdraw.order = FALSE;
    if (cv != NULL) {
      if (cv->mode == SOLD) {
        cv = SelectObjInObj(&listheadplayer, cv->in->id, cv->player);
        if (cv != NULL) {
          cv->selected = TRUE;
          if (cv->in == cv0->in) {
            habitat.type = cv->habitat;
            habitat.obj = cv->in;
          } else {
            cv = cv0;
          }
        }
      }

      if (cv0 != cv) { /* if center coordinates changes */
        if (cv != NULL) {
          habitat.type = cv->habitat;
          habitat.obj = cv->in;
          if (cv->type == PLANET) {
            habitat.type = H_PLANET;
            habitat.obj = cv;
          }
        }
      }
    }
  } else {
    gdk_draw_line(pixmap, GetColor(RED), 0, gheight, gwidth, gheight);
  }

  /* --Draw Shell */

  /*Selection box */
  SelectionBox(pixmap, GetColor(GREEN), &cv, 0);
  /* --Selection box */

  /* show window */

  if (gdraw.map == TRUE) {
    if (drawmap) {
      update_rect.x = 0;
      update_rect.y = 0;
      update_rect.width = drawing_area->allocation.width;
      update_rect.height = drawing_area->allocation.height;
    }
  } else {
    update_rect.x = 0;
    update_rect.y = 0;
    update_rect.width = drawing_area->allocation.width;
    update_rect.height = drawing_area->allocation.height;
  }

  gtk_widget_draw(drawing_area, &update_rect); /*  deprecated */

  if (GameParametres(GET, GPAUSED, 0) == TRUE) {
    paused = 1;
  } else {
    paused = 0;
  }

  /* --Drawing window */

  if (GameParametres(GET, GPAUSED, 0) == FALSE) {
    IncTime();
  }

  loadsw = 0;

  if (GameParametres(GET, GQUIT, 0) == 2 && swcomm == TRUE) {
    Quit(NULL, NULL);
  }

  if (swcomm) {
    /*** check for cargo objects ****/
    CargoCheck(&listheadobjs, cv);
    /*** --check for cargo objects ****/
    if (RemoveDeadObjs(&listheadobjs, cv) == NULL) {
      cv = NULL;
    }
  }

  if (IsInObjList(&listheadobjs, ship_c) == 0) {
    ship_c = NULL;
  }

  if (cv != NULL) {
    habitat.type = cv->habitat;
    habitat.obj = cv->in;
    if (cv->type == PLANET) {
      habitat.type = H_PLANET;
      habitat.obj = cv;
    }
  }

  /* Check for GAMEOVER */

  for (i = 1; i < GameParametres(GET, GNPLAYERS, 0) + 1; i++) {
    if (!((cont + i) % 30) && plyr[i].status >= PLAYERACTIVE) {
      if (GameOver(&listheadobjs, plyr, i)) {
        plyr[i].status = PLAYERDEAD;
        plyr[i].level = 0;
      }
    }
  }

  cont++;
  {
    static double t = 0, tt = 0;
    static int c = 0;
    Clock(2, CL_STOP);
    t = Clock(2, CL_READ);
    tt += t;
    c++;
    if (c == 20) {
      tt = 0;
      c = 0;
    }
  }

  gettimeofday(&time1, NULL);

  return (TRUE);

} /* MainLoop */

gint Quit(GtkWidget* widget, gpointer gdata) {
  /*

   */

  sem_close(&sem_barrier);
  sem_close(&sem_barrier1);

#if SOUND
  printf("Sound Closed\n");
  ExitSound();
  printf("Sound Closed\n");
#endif

  DestroyAllObj(&listheadobjs);
  listheadobjs.list = NULL;
  listheadobjs.n = 0;
  ship_c = NULL;
  cv = NULL;

  QuitGraphics(widget, gdata);
  printf("Graphics Closed\n");
  return FALSE;
}

void key_eval(void) {
  /*
    version 01 13May11
    Evaluate all the key and mouse press

  */
  struct ObjList* ls;
  Object* obj;
  static int swtab = 0;
  static int swtab0 = 0;
  int proc;
  int keyf = -1;
  int navcontrol = TRUE; /* FALSE nav mode, TRUE order mode */
  int x, y;

  struct Player* plyr;
  struct Keys* key;

  plyr = GetPlayers();
  key = GetKeys();
  if (gdraw.menu == TRUE) {
    return;
  }
  proc = GetProc();

  /* QUIT game */

  if (key->ctrl == TRUE && key->q == TRUE) {
    GameParametres(SET, GQUIT, 1); /*  ask: Really Quit? */
  }

  if (GameParametres(GET, GQUIT, 0) == 1) {
    if (key->n == TRUE || key->esc == TRUE) {
      GameParametres(SET, GQUIT, 0);
      key->ctrl = FALSE;
      key->q = FALSE;
      key->n = FALSE;
      key->esc = FALSE;
    }
    if (key->y == TRUE) {
      GameParametres(SET, GQUIT, 2); /* Quit game */
      GameParametres(SET, GPAUSED, FALSE);
    }
    return;
  }

  if (key->esc == TRUE) {
    key->alt = FALSE;
    key->ctrl = FALSE;
    Shell(0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL);              /* reset shell() */
    key->order.state = FALSE; /* aqui01 salir de order mode */
  }

  /* game paused */
  if (key->p == TRUE && GameParametres(GET, GPAUSED, 0) == TRUE) {
    GameParametres(SET, GPAUSED, FALSE);
    key->ctrl = FALSE;
    key->p = FALSE;
  }
  if (key->ctrl == TRUE && key->p == TRUE &&
      GameParametres(GET, GPAUSED, 0) == FALSE) {
    GameParametres(SET, GPAUSED, TRUE);
    key->ctrl = FALSE;
    key->p = FALSE;
    Shell(0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL);              /* reset shell() */
    key->order.state = FALSE; /* aqui01 salir de order mode */
    navcontrol = TRUE;
  }

  if (GameParametres(GET, GPAUSED, 0) == TRUE) {
    Sound(SPAUSE, MUSIC);
  } else {
    Sound(SPLAY, MUSIC);
  }

  /* mouse */
  if (key->mleft == TRUE) {
  }
  if (key->mright == TRUE) {
  }
  if (key->mdclick == TRUE) {
    key->mdclick = FALSE;
  }

  if (key->mleft == TRUE) {
    if (WindowFocus(&windowgamelog)) {
      MousePos(GET, &x, &y);
      ActWindow(&windowgamelog);
      key->mleft = FALSE;
    }
  }

  /* --mouse */

  /* sound */

  if (key->ctrl == TRUE && (key->plus == TRUE || key->minus == TRUE)) {
    if (key->plus == TRUE) {
      SetMasterVolume(0.025, VOLINC);
      gdraw.volume = TRUE;
    }
    if (key->minus == TRUE) {
      SetMasterVolume(-0.025, VOLINC);
      gdraw.volume = TRUE;
    }

    GameParametres(SET, GSOUNDVOL, param.soundvol);
  }
  /* --sound */

  navcontrol = TRUE;
  if (key->order.state == TRUE)
    navcontrol = FALSE;

  /* in map view don't control the ship*/
  if (gdraw.map == TRUE) {
    key->accel.state = key->turnleft.state = key->turnright.state =
        key->fire.state = FALSE;
    key->automode.state = key->manualmode.state = FALSE;
  }

  switch (navcontrol) {
    case FALSE: /* order mode */
      break;
    case TRUE: /* Nav mode */
      /* f1 f2 f3 f4 */
      keyf = -1;
      if (key->f1)
        keyf = 0;
      if (key->f2)
        keyf = 1;
      if (key->f3)
        keyf = 2;
      if (key->f4)
        keyf = 3;
      key->f1 = key->f2 = key->f3 = key->f4 = FALSE;

      if (keyf >= 0) {
        if (key->ctrl == TRUE) {
          key->ctrl = FALSE;
          if (cv != NULL)
            fobj[keyf] = cv->id;
        } else {
          if ((obj = SelectObj(&listheadobjs, fobj[keyf])) != NULL) {
            if (cv != NULL)
              cv->selected = FALSE;
            cv = obj;
            habitat.type = cv->habitat;
            habitat.obj = cv->in;
            SelectionBox(NULL, NULL, &cv, 2);
            cv->selected = TRUE;
          }
        }
      }
      /*-- f1 f2 f3 f4 */

      /* observe enemies */

      if (observeenemies == TRUE) {
        if (key->f9 == TRUE || key->f10 == TRUE) {
          if (cv != NULL) {
            plyr[actual_player].cv = cv->id;
          } else {
            plyr[actual_player].cv = 0;
          }
          if (key->f9 == TRUE) {
            key->f9 = FALSE;
            actual_player--;
            if (actual_player < 1) {
              actual_player = GameParametres(GET, GNPLAYERS, 0) + 1;
            }
          }
          if (key->f10 == TRUE) {
            key->f10 = FALSE;
            actual_player++;
            if (actual_player > GameParametres(GET, GNPLAYERS, 0) + 1) {
              actual_player = 1;
            }
          }

          DestroyObjList(&listheadplayer);
          listheadplayer.n = 0;
          listheadplayer.list = NULL;
          CreatePlayerList(listheadobjs, &listheadplayer, actual_player);
          listheadplayer.update = 0;

          cv = SelectObj(&listheadplayer, plyr[actual_player].cv);
          if (cv == NULL) {
            cv = NextCv(&listheadplayer, cv, actual_player);
          }
          if (cv != NULL) {
            habitat.type = cv->habitat;
            habitat.obj = cv->in;
            SelectionBox(NULL, NULL, &cv, 2);
          }
        }
      } /*-- observe enemies */

      if (ship_c != NULL) {
        if (ship_c->type != SHIP) {
          key->trace = FALSE;
          ship_c->trace = FALSE;
        }
        if (ship_c->subtype != EXPLORER && ship_c->subtype != FIGHTER &&
            ship_c->subtype != QUEEN && ship_c->subtype != FREIGHTER) {
          key->trace = FALSE;
          ship_c->trace = FALSE;
        }

        if (key->trace == TRUE) {
          if (ship_c->trace == TRUE)
            ship_c->trace = FALSE;
          else
            ship_c->trace = TRUE;
          key->trace = FALSE;
        }

        if (ship_c->ai == 0 && proc == plyr[ship_c->player].proc) {
          if (key->down == TRUE) {
            ship_c->accel -= .08;
            if (ship_c->accel < 0)
              ship_c->accel = 0;
          }

          /**** ship movement *****/

          if (cv == ship_c) {
            if (ship_c->gas > 0 && gdraw.map == FALSE) {
              if (cv->type == SHIP && cv->subtype == PILOT) {
                key->accel.state = key->turnleft.state = key->turnright.state =
                    key->fire.state = FALSE;
              }
              if (key->accel.state == TRUE) {
                if (ship_c->mode == LANDED && fabs(ship_c->a - PI / 2) > .35) {
                  ship_c->accel = 0;
                } else {
                  ship_c->accel += ship_c->engine.a;
                  if (ship_c->accel > ship_c->engine.a_max)
                    ship_c->accel = ship_c->engine.a_max;
                }
              } else {
                ship_c->accel = 0;
              }
              if (ship_c->mode == LANDED && ship_c->vy > 0) {
                ship_c->mode = NAV;
              }
              if (key->turnleft.state == TRUE &&
                  ship_c->gas > ship_c->engine.gascost) {
                ship_c->ang_a += ship_c->engine.ang_a;
                if (ship_c->ang_a > ship_c->engine.ang_a_max)
                  ship_c->ang_a = ship_c->engine.ang_a_max;
              }
              if (key->turnright.state == TRUE &&
                  ship_c->gas > ship_c->engine.gascost) {
                ship_c->ang_a -= ship_c->engine.ang_a;
                if (ship_c->ang_a < -ship_c->engine.ang_a_max)
                  ship_c->ang_a = -ship_c->engine.ang_a_max;
              }
              if (key->turnleft.state == FALSE &&
                  key->turnright.state == FALSE) {
                ship_c->ang_a = 0;
              }
              if (key->fire.state == TRUE && ship_c->gas > 2) {
                if (!ship_c->weapon->cont1) {
                  ChooseWeapon(ship_c);
                  if (proc == plyr[ship_c->player].proc) {
                    if (FireCannon(&listheadobjs, ship_c, NULL) == 0) {
                      Play(ship_c, FIRE0, 1);
                    }
                  }
                }
              }
            }
          }
          /**** --ship movement *****/
        }
      } /* if(ship_c!=NULL) */

      if (cv != NULL) {
        if (key->ctrl && key->n == TRUE) {
          if (nav_mode == RELATIVE) {
            nav_mode = ABSOLUTE;
            r_rel.x = cv->x;
            r_rel.y = cv->y;
          } else {
            nav_mode = RELATIVE;
          }
          key->n = FALSE;
        }

        if (actual_player == actual_player0) {
          if (key->automode.state == TRUE) {
            /* pressing 'down' the selected ship goes to automatic mode */
            if (GameParametres(GET, GQUIT, 0) == 0) {
              if (cv->type == SHIP &&
                  (cv->subtype == FIGHTER || cv->subtype == EXPLORER ||
                   cv->subtype == QUEEN || cv->subtype == FREIGHTER ||
                   cv->subtype == TOWER)) {
                ship_c = cv;
                ship_c->ai = 1;
                key->manualmode.state = FALSE;
                key->automode.state = FALSE;
              }
            }
          }

          if (key->manualmode.state == TRUE && cv->type == SHIP &&
              /* pressing a movement key the spaceship goes to manual mode */
              (cv->subtype == FIGHTER || cv->subtype == EXPLORER ||
               cv->subtype == QUEEN || cv->subtype == FREIGHTER ||
               cv->subtype == TOWER)) {
            ship_c = cv;
            if (ship_c->ai)
              ship_c->weapon = &ship_c->weapon0;
            ship_c->ai = 0;

            key->manualmode.state = FALSE;
            key->automode.state = FALSE;
          }

          if (key->number[1] == TRUE) {
            if (cv->weapon0.type != CANNON0)
              cv->weapon = &cv->weapon0;
            key->number[1] = FALSE;
          }
          if (key->number[2] == TRUE) {
            if (cv->weapon1.type != CANNON0)
              cv->weapon = &cv->weapon1;
            key->number[2] = FALSE;
          }
          if (key->number[3] == TRUE) {
            if (cv->weapon1.type != CANNON0)
              cv->weapon = &cv->weapon2;
            key->number[3] = FALSE;
          }
        }
      }

      break;
    default:
      break;
  }

  if (!(key->Pagedown | key->Pageup | key->home | key->tab)) {
    swtab = 0;
    swtab0 = 0;
  }

  if (key->Pagedown | key->Pageup | key->home | key->tab) {
    Shell(0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL);              /* reset shell() */
    key->order.state = FALSE; /* aqui01 salir de order mode */
    navcontrol = TRUE;
    key->p = FALSE;

    if (key->tab == TRUE) {
      if (swtab == 0) {
        if (cv != NULL)
          cv->selected = FALSE;
        if (key->ctrl == TRUE) {
          cv = PrevCv(&listheadplayer, cv, actual_player);
        } else {
          cv = NextCv(&listheadplayer, cv, actual_player);
        }
      }
      swtab++;
      if (swtab0)
        if (swtab > 1)
          swtab = 0;
      if (swtab > 6) {
        swtab = 0;
        swtab0 = 1;
      }
    }

    if (key->Pagedown | key->Pageup | key->home) {
      if (swtab == 0) {
        if (cv != NULL)
          cv->selected = FALSE;
        if (key->Pagedown == TRUE) {
          cv = NextPlanetCv(&listheadplayer, cv, actual_player);
          key->Pagedown = FALSE;
        }
        if (key->Pageup == TRUE) {
          cv = PrevPlanetCv(&listheadplayer, cv, actual_player);
          key->Pageup = FALSE;
        }
        if (key->home == TRUE) {
          key->home = FALSE;
          cv = FirstShip(&listheadplayer, cv, actual_player);
        }
      }
      swtab++;
      if (swtab0)
        if (swtab > 1)
          swtab = 0;
      if (swtab > 6) {
        swtab = 0;
        swtab0 = 1;
      }
    }

    if (cv != NULL) {
      habitat.type = cv->habitat;
      habitat.obj = cv->in;
      SelectionBox(NULL, NULL, &cv, 2);
      cv->selected = TRUE;

      if (nav_mode == ABSOLUTE) {
        while (cv->x - r_rel.x > GameParametres(GET, GWIDTH, 0) / 2) {
          r_rel.x += GameParametres(GET, GWIDTH, 0);
        }
        while (cv->x - r_rel.x < -GameParametres(GET, GWIDTH, 0) / 2) {
          r_rel.x -= GameParametres(GET, GWIDTH, 0);
        }
        while (cv->y - r_rel.y > GameParametres(GET, GHEIGHT, 0) / 2) {
          r_rel.y += GameParametres(GET, GHEIGHT, 0);
        }
        while (cv->y - r_rel.y < -GameParametres(GET, GHEIGHT, 0) / 2) {
          r_rel.y -= GameParametres(GET, GHEIGHT, 0);
        }
      }
    }
  }

  /* save and load game */
  if (key->ctrl == TRUE && key->s == TRUE) {
    if (GameParametres(GET, GMODE, 0) == CLIENT) {
      key->save = FALSE;
    } else {
      key->save = TRUE;
      key->s = FALSE;
    }
  }

  if (key->ctrl == TRUE && key->l == TRUE) {
    if (GameParametres(GET, GMODE, 0) == CLIENT) {
      key->load = FALSE;
    } else {
      key->load = TRUE;
      key->l = FALSE;
    }
  }
  /*-- save and load game */

  /* alt modes. Cargo */

  if (gdraw.order == FALSE && key->alt == TRUE) {
    int n = 0;
    int subtype = -1;
    if (key->s == TRUE) { /* SATELLITE off*/
      subtype = SATELLITE;
      key->s = FALSE;
    }
    if (key->p == TRUE) { /* PILOT off*/
      subtype = PILOT;
      key->p = FALSE;
    }

    if (subtype != -1) {
      /*
        take off one object from selected ships
      */
      ls = listheadobjs.list;
      while (ls != NULL) { /* HERE create a selected list */
        if (ls->obj->selected == TRUE) {
          obj = CargoGet(ls->obj, SHIP, subtype);
          if (obj != NULL) {
            n++;
            obj->habitat = ls->obj->habitat;
            obj->in = ls->obj->in;
            obj->mode = NAV;
            obj->planet = NULL;
            obj->ai = 1;
            obj->x = ls->obj->x + ls->obj->radio * cos(ls->obj->a - PI);
            obj->y = ls->obj->y + ls->obj->radio * sin(ls->obj->a - PI);
            obj->vx = ls->obj->vx - 0.5 + Random(-1);
            obj->vy = ls->obj->vy - 0.5 + Random(-1);

            if (GameParametres(GET, GNET, 0) == TRUE) {
              SetModified(obj, SENDOBJALL);
              if (obj->modified != SENDOBJALL) {
                printf("OBJ EJECT mod: %d\n", obj->modified);
              }
            }
          }
        }
        ls = ls->next;
      }
    }
  }
  /* alt modes. Cargo */
}

void UpdateShip(Object* obj) {
  /*
     Calculate the new coordinates of the ship *obj:
     calculate gravity forces
     update position and velocity
     recharge armor and fuel
     repair ship
     learning and experience
     check collision with planets
  */

  float vx, vy, v, vmax2;
  float cosa, sina;
  float rx, ry;
  float vr, vo;
  float fx, fy;
  float fx0, fy0;
  float dtim;
  float a, factor;
  float g = 1.2 / 250000;
  int time;
  int n;
  int proc;
  float b = 0.05; /* air resistance */
  float bx, by;
  int mass;
  struct Player* plyr;
  plyr = GetPlayers();

  proc = GetProc();

  mass = obj->mass + obj->cargo.mass;

  if (obj->habitat == H_SHIP) {
    if (obj->in != NULL) {
      obj->x = obj->in->x;
      obj->x0 = obj->in->x0;
      obj->y = obj->in->y;
      obj->y0 = obj->in->y0;
      obj->vx = obj->in->vx;
      obj->vy = obj->in->vy;
    } else {
      if (GameParametres(GET, GNET, 0) == FALSE) {
        fprintf(stderr,
                "ERROR UpdateShip(): habitat=H_SHIP in = NULL id: %d (pid:%d) "
                "t:%d st: %d player: %d\n",
                obj->id, obj->pid, obj->type, obj->subtype, obj->player);
        exit(-1);
      }
    }
    return;
  }

  vx = obj->vx;
  vy = obj->vy;
  dtim = DT / mass;
  fx0 = fy0 = 0;
  bx = by = 0;
  /***** Forces 1*****/
  switch (obj->habitat) {
    case H_SPACE:
      PlanetAtraction(&fx0, &fy0, obj->x, obj->y, mass);
      if (obj->subtype == SATELLITE) {
        if (fx0 > 0.0000001) {
          obj->life = LIFESATELLITE;
        }
      }

      break;
    case H_PLANET:
      if (obj->mode == LANDED) {
        fy0 = 0;
      } else { /* air resistance, */
        v = sqrt(obj->vx * obj->vx + obj->vy * obj->vy);
        b *= (obj->in->mass / 120000 + 0.75); /* density */
        bx = -b * v * obj->vx;                /* wind HERE TODO +3; */
        by = -b * v * obj->vy;
        fy0 = -g * mass * (float)obj->in->mass + by;
        fx0 = bx;
      }
      break;
    case H_SHIP:
      fx0 = fy0 = 0;
      if (obj->in == NULL) {
        fprintf(
            stderr,
            "ERROR UpdateShip(hship): in = NULL in id: %d (%d) t:%d st: %d\n",
            obj->id, obj->pid, obj->type, obj->subtype);
        return;
      }
      break;
    default:
      fprintf(stderr,
              "ERROR UpdateShip(): Habitat unknown: %d in = NULL in id: %d "
              "(%d) t:%d st: %d\n",
              obj->habitat, obj->id, obj->pid, obj->type, obj->subtype);
      exit(-1);
      break;
  }

  /*** first step ****/

  obj->x0 = obj->x;
  obj->y0 = obj->y;

  if (obj->mode == NAV) {
    obj->x += (vx + .5 * (fx0)*dtim) * DT;
    obj->y += (vy + .5 * (fy0)*dtim) * DT;
  }

  /*** forces 2 ****/

  fx = fy = 0;
  switch (obj->habitat) {
    case H_SPACE:
      PlanetAtraction(&fx, &fy, obj->x, obj->y, mass);
      break;
    case H_PLANET:
      if (obj->mode == LANDED) {
        fy = 0;
      } else {
        fy = -g * mass * (float)obj->in->mass + by;
        fx = bx;
      }
      break;
    default:
      fx = fy = 0;
      break;
  }

  /***** gas cost *****/
  if (proc == plyr[obj->player].proc ||
      (obj->type == PROJECTILE && obj->subtype == MISSILE)) {
    if (obj->engine.a > 0) {
      if (obj->accel > 0) {
        obj->gas -= 10 * obj->engine.gascost * obj->accel / obj->engine.a_max;
        if (obj->gas < 0) {
          obj->gas = 0;
          obj->accel = 0;
        }
      }
    }

    if (obj->engine.ang_a_max && obj->ang_a != 0) {
      obj->gas -=
          fabs(obj->engine.gascost * obj->ang_a / obj->engine.ang_a_max);
      if (obj->gas < 0) {
        obj->gas = 0;
        obj->ang_a = 0;
      }
    }
  }

  /***** step 2 *****/
  if (obj->accel) {
    vx += (.5 * (fx + fx0) + cos(obj->a) * obj->accel) * dtim;
    vy += (.5 * (fy + fy0) + sin(obj->a) * obj->accel) * dtim;
  } else {
    vx += (.5 * (fx + fx0)) * dtim;
    vy += (.5 * (fy + fy0)) * dtim;
  }

  if (obj->mode == LANDED) {
    if (vy <= 0) {
      vy = 0;
      vx = 0;
    } else {
      if (obj->accel > 0) {
        obj->mode = NAV;
      }
    }
  }

  /***** if max vel is reached, reescaling *****/

  if (obj->type == SHIP) {
    vmax2 = obj->engine.v2_max * (1 - 0.4375 * (obj->state < 25)) +
            (obj->level * 50) * (obj->accel > 0);
    if (obj->subtype == PILOT)
      vmax2 = 15 * 15;
  } else {
    if (obj->engine.type > ENGINE0) {
      vmax2 = obj->engine.v2_max;
    } else {
      vmax2 = (VELMAX2);
    }
  }

  if (vx * vx + vy * vy > vmax2) {
    factor = sqrt(vmax2 / (vx * vx + vy * vy));
    if (factor > 1) {
      fprintf(stderr, "ERROR factor>1\n");
      printf("\tobj_enginev2max: %d %d\n", obj->engine.v2_max,
             (obj->state < 25));
    }
    vx *= factor;
    vy *= factor;
  }

  /***** angular velocity *****/

  if (obj->ang_a != 0) {
    obj->ang_v += obj->ang_a * 100. * dtim;
    if (obj->ang_v > obj->engine.ang_v_max)
      obj->ang_v = obj->engine.ang_v_max;
    if (obj->ang_v < -obj->engine.ang_v_max)
      obj->ang_v = -obj->engine.ang_v_max;
  } else {
    if (obj->type == SHIP &&
        obj->subtype != PILOT) /* HERE TODO add artifact to engine*/
      obj->ang_v *= 0.5;
  }

  obj->a += (obj->ang_v + 0.5 * obj->ang_a * 100. * dtim) * DT;

  if (obj->mode == LANDED) { /* max angle of landed ships */
    float maxang = 0;
    switch (obj->subtype) {
      case EXPLORER:
      case FIGHTER:
      case QUEEN:
        maxang = PI / 4;
        break;
      case FREIGHTER:
      case PILOT:
        maxang = 0;
        break;
      default:
        maxang = PI / 2 - PI / 40;
        break;
    }
    if (obj->a > PI / 2 + maxang) {
      obj->a = PI / 2 + maxang;
      obj->ang_a = obj->ang_v = 0;
    }
    if (obj->a < PI / 2 - maxang) {
      obj->a = PI / 2 - maxang;
      obj->ang_a = obj->ang_v = 0;
    }
  }
  if (obj->a > PI)
    obj->a -= 2 * PI;
  if (obj->a < -PI)
    obj->a += 2 * PI;
  obj->vx = vx;
  obj->vy = vy;
  obj->fx0 = fx0;
  obj->fy0 = fy0;
  obj->fx = fx;
  obj->fy = fy;

  /***** recharging armor and fuel ******/

  if (obj->weapon0.cont1)
    obj->weapon0.cont1--;
  if (obj->weapon1.cont1)
    obj->weapon1.cont1--;
  if (obj->weapon2.cont1)
    obj->weapon2.cont1--;

  if (proc == plyr[obj->player].proc) {
    time = GetTime();
    /* refuel in space */
    if (obj->type == SHIP && obj->subtype != PILOT && obj->habitat == H_SPACE) {
      obj->gas += 0.05;
      if (obj->gas > obj->gas_max)
        obj->gas = obj->gas_max;
      if (obj->state > 0 && obj->state < 25) {
        obj->state += 0.005;
      }
    }

    /* ammunition in space */
    if (!(time % 100)) {
      if (obj->type == SHIP && obj->subtype == SATELLITE &&
          obj->habitat == H_SPACE) {
        if (obj->gas > 0.1 * obj->gas_max && obj->state > 25 &&
            obj->weapon0.n < obj->weapon0.max_n) {
          obj->weapon0.n++;
        }
      }
    }
    /*
     * repair, refuel and learning on a planet
     */

    if (obj->type == SHIP && obj->mode == LANDED &&
        obj->subtype != PILOT) { /* HERE TODO add propertie to ships, pilots. */

      /* Cargo FREIGHTERS */
      if (!(time % 10)) {
        if (obj->subtype == FREIGHTER) {
          if (obj->cargo.mass < obj->cargo.capacity) {
            obj->cargo.mass += 1;
          }
        }
      }
      /* --Cargo FREIGHTERS */

      /* repair and refuel  */
      if (plyr[obj->player].gold < 5) {
        n = 5 * Random(-1); /* HERE depends on number of damaged objects */
      } else {
        n = 0;
      }

      /* first repair, then gas and last ammunition */
      if (plyr[obj->player].gold > 0 && n == 0) {
        if (obj->state < 100) {
          if (obj->state > 0) {
            obj->state += 0.05;
          }
          if (obj->state > 100)
            obj->state = 100;
          plyr[obj->player].gold -= .125; /* total cost 250  */
          plyr[obj->player].goldweapon += .125;
        }

        if (obj->state > 50) {
          if (obj->gas < obj->gas_max) {
            obj->gas += 2;
            if (obj->gas > obj->gas_max)
              obj->gas = obj->gas_max;
            plyr[obj->player].gold -= .2; /* total cost 100 */
            plyr[obj->player].goldweapon += .2;
          }
          if (obj->gas > 0.5 * obj->gas_max) {
            if (!(time % 4)) {
              if (obj->weapon0.n < obj->weapon0.max_n) {
                if (plyr[obj->player].gold > obj->weapon0.projectile.unitcost) {
                  obj->weapon0.n++;
                  plyr[obj->player].gold -= obj->weapon0.projectile.unitcost;
                  plyr[obj->player].goldweapon +=
                      obj->weapon0.projectile.unitcost;
                }
              }
            }
            if (!(time % 240)) {
              if (obj->weapon1.n < obj->weapon1.max_n) {
                if (plyr[obj->player].gold > obj->weapon1.projectile.unitcost) {
                  plyr[obj->player].gold -= obj->weapon1.projectile.unitcost;
                  plyr[obj->player].goldweapon +=
                      obj->weapon1.projectile.unitcost;
                  obj->weapon1.n++;
                }
              }
            }
            if (!(time % 14)) {
              if (obj->weapon2.n < obj->weapon2.max_n) {
                if (plyr[obj->player].gold > obj->weapon2.projectile.unitcost) {
                  plyr[obj->player].gold -= obj->weapon2.projectile.unitcost;
                  plyr[obj->player].goldweapon +=
                      obj->weapon2.projectile.unitcost;
                  obj->weapon2.n++;
                }
              }
            }
          }
        }
      }

      /***** -- repair and refuel *****/

      /***** Learning, experience *****/
      if (obj->type == SHIP && obj->state > 99 &&
          obj->subtype != PILOT) { /* HERE TODO pilots has no ship for training.
                                      must depend of pilot properties. */
        int mlevel = 0;
        float incexp;

        mlevel = obj->in->level;
        obj->cdata->tmlevel = time;
        obj->cdata->mlevel = mlevel;
        if (obj->cdata->mlevel - obj->level > 2) {
          incexp = 0.02 * (obj->cdata->mlevel - obj->level);
          incexp = incexp > 0.06 ? 0.06 : incexp;
          Experience(obj, incexp);
        }
      }

      /***** --Learning, experience *****/

      if (obj->subtype == FIGHTER && obj->level >= MINLEVELPILOT) {
        obj->items = obj->items | ITSURVIVAL; /* create a survival pod */
      }

      /* player level */
      if (proc == plyr[obj->player].proc) { /* ship is landed */
        if ((obj->level > plyr[obj->player].gmaxlevel)) {
          plyr[obj->player].gmaxlevel = obj->level;
        }
      }
    } /*if(obj->mode==LANDED) */

    /*
     * -- repair, refuel and learning
     */
  }

  /***** boundaries at planet *****/
  if (obj->habitat == H_PLANET) {
    if (obj->x < 0)
      obj->x += LXFACTOR;
    if (obj->x > LXFACTOR)
      obj->x -= LXFACTOR;

    if (obj->y > LYFACTOR) { /* its out of planet */
      plyr[obj->player].status = PLAYERMODIFIED;
      listheadplayer.update = 1;

      if (proc == plyr[obj->player].proc ||
          (obj->type == PROJECTILE && obj->subtype == MISSILE)) {
        a = -PI * (2 * obj->x / (float)LXFACTOR + 0.5);
        cosa = cos(a);
        sina = sin(a);
        obj->x = obj->in->planet->x + 2 * obj->in->planet->r * cosa;
        obj->y = obj->in->planet->y + 2 * obj->in->planet->r * sina;

        vo = -obj->vx;
        vr = obj->vy;
        obj->vx = vr * cosa - vo * sina;
        obj->vy = vr * sina + vo * cosa;

        rx = obj->x - obj->in->planet->x;
        ry = obj->y - obj->in->planet->y;

        a = atan2(ry, rx);

        obj->a = a - PI / 2 + obj->a;

        obj->habitat = H_SPACE;
        obj->planet = NULL;
        obj->in = NULL;
        if (GameParametres(GET, GNET, 0) == TRUE) {
          SetModified(obj, SENDOBJAALL); /*on getting out of planet */
          obj->ttl = 0;
        }
        if (obj == cv) {
          habitat.type = H_SPACE;
          habitat.obj = NULL;
          SelectionBox(NULL, NULL, &cv, 1);
        }
      }
    }
  }
  return;
}

void UpdateAsteroid(Object* obj) {
  /*
     Calculate the new coordinates of the asteroid *obj
     Only gravitational forces.
  */

  float vx, vy, vmax2;
  float cosa, sina;
  float rx, ry;
  float vr, vo;
  float fx, fy;
  float fx0, fy0;
  float dtim;
  float a, factor;
  float g = 1.2 / 250000;
  int proc;
  struct Player* plyr;
  plyr = GetPlayers();

  if (obj->type != ASTEROID)
    return;

  proc = GetProc();

  vx = obj->vx;
  vy = obj->vy;
  dtim = DT / obj->mass;

  fx0 = fy0 = 0;

  if (obj->habitat != H_SPACE) {
    if (obj->mode == NAV) {
      if (obj->habitat == H_PLANET) {
        fy0 = -g * obj->mass * (float)obj->in->mass;
      } else {
        printf("in = NULL in id: %d\n", obj->id);
        return;
      }
    }
  }

  obj->x0 = obj->x;
  obj->y0 = obj->y;

  obj->x += obj->vx * DT + .5 * fx0 * DT * dtim;
  obj->y += obj->vy * DT + .5 * fy0 * DT * dtim;

  fx = fy = 0;
  if (obj->habitat == H_SPACE) {
    PlanetAtraction(&fx, &fy, obj->x, obj->y, obj->mass);
  } else {
    fy = -g * obj->mass * (float)obj->in->mass;
  }

  vx += .5 * (fx + fx0) * dtim;
  vy += .5 * (fy + fy0) * dtim;

  /* if max vel is reached, reescaling */

  vmax2 = (VELMAX * VELMAX / 16.0);

  if (vx * vx + vy * vy > vmax2) {
    factor = sqrt(vmax2 / (vx * vx + vy * vy));
    if (factor > 1) {
      fprintf(stderr, "ERROR factor>1\n");
      printf("\tobj_enginev2max: %d %d\n", obj->engine.v2_max,
             (obj->state < 25));
    }
    vx *= factor;
    vy *= factor;
  }

  obj->a += DT * obj->ang_v;

  if (obj->a > PI)
    obj->a -= 2 * PI;
  if (obj->a < -PI)
    obj->a += 2 * PI;

  obj->vx = vx;
  obj->vy = vy;
  obj->fx0 = obj->fx;
  obj->fy0 = obj->fy;

  if (obj->habitat == H_PLANET) {
    if (obj->x < 0)
      obj->x += LXFACTOR;
    if (obj->x > LXFACTOR)
      obj->x -= LXFACTOR;

    if (obj->y > LYFACTOR) { /* its out of planet */
      if (proc == plyr[obj->player].proc) {
        a = obj->x * (2 * PI) / (float)LXFACTOR - PI;
        cosa = cos(a);
        sina = sin(a);
        obj->x = obj->in->planet->x + 2 * obj->in->planet->r * cosa;
        obj->y = obj->in->planet->y + 2 * obj->in->planet->r * sina;

        vo = -obj->vx;
        vr = obj->vy;
        obj->vx = vr * cosa - vo * sina;
        obj->vy = vr * sina + vo * cosa;

        rx = obj->x - obj->in->planet->x;
        ry = obj->y - obj->in->planet->y;

        a = atan2(ry, rx);

        /*      obj->a=obj->a-a+PI/2;  */
        obj->a = a - PI / 2 + obj->a;

        obj->habitat = H_SPACE;
        obj->planet = NULL;
        obj->in = NULL;
        if (GameParametres(GET, GNET, 0) == TRUE) {
          if (GetProc() == plyr[obj->player].proc) {
            SetModified(obj, SENDOBJAALL); /*on getting out of planet */
            obj->ttl = 0;
          }
        }
        if (obj == cv) {
          habitat.type = H_SPACE;
          habitat.obj = NULL;
        }
      }
    }
  }
  return;
}

void Collision(struct HeadObjList* lh) {
  /*
    version 02.(200211) without CollisionList
    Calculate the collision between objects
  */

  struct ObjList *ls1, *ls2;
  Object *obj1, *obj2, *objt1, *objt2;
  Object *obj, *pnt, *shot;
  Object* nobj;
  Segment s;
  float radio2, r, r2, rx, ry;
  float r02, r0x, r0y;
  float damage;
  float v;
  float a, b;
  char text[MAXTEXTLEN];
  int i, j;
  int gkplanets, gnet, gnplayers;
  int proc;
  int crashsw = 0;
  int time = 0;
  int shocks = 0;
  int swnextobj1 = 0;
  struct Player* plyr;
  plyr = GetPlayers();

  gkplanets = GameParametres(GET, GKPLANETS, 0);
  gnet = GameParametres(GET, GNET, 0);
  gnplayers = GameParametres(GET, GNPLAYERS, 0);

  proc = GetProc();
  time = GetTime();
  /* among objs and ship */
  ls1 = lh->list;

  /* among objs */
  while (ls1 != NULL) {
    swnextobj1 = 0;
    obj1 = ls1->obj;
    shocks = 1; /* can collide with other objects */

    if (obj1->state <= 0) {
      ls1 = ls1->next;
      continue;
    }

    switch (obj1->type) {
      case SHIP:

        if (gnet == TRUE && obj1->ttl < MINTTL) {
          ls1 = ls1->next;
          continue;
        }

        if (obj1->habitat == H_SHIP) {
          ls1 = ls1->next;
          continue;
        }
        if (obj1->subtype == PILOT && obj1->mode == LANDED) {
          ls1 = ls1->next;
          continue;
        }

        break;
      case PROJECTILE:
        /* explosion only collide with terrain */
        if (obj1->subtype == EXPLOSION) {
          if (obj1->habitat != H_PLANET) {
            ls1 = ls1->next;
            continue;
          } else {
            shocks = 0;
          }
        }
        break;
      case TRACKPOINT:
      case TRACE:
        ls1 = ls1->next;
        continue;
        break;
      default:
        break;
    }

    if (shocks) {
      ls2 = ls1->next;

      if (obj1->type == PLANET) {
        while (ls2 != NULL && ls2->obj->type == PLANET) {
          ls2 = ls2->next;
        }
      }

      /* double loop */
      /* collision between two objects */
      while (ls2 != NULL) {
        /* explosion only collide with terrain */
        if (obj1->type == PROJECTILE && obj1->subtype == EXPLOSION) {
          break;
        }

        if (obj1->state <= 0) {
          ls2 = ls2->next;
          continue;
        }
        if (obj1->habitat != ls2->obj->habitat) {
          ls2 = ls2->next;
          continue;
        }

        obj2 = ls2->obj;
        switch (obj2->type) {
          case SHIP:
            if (gnet == TRUE) {
              if (obj2->ttl < MINTTL) {
                ls2 = ls2->next;
                continue;
              }
            }

            if (obj2->habitat == H_SHIP) {
              ls2 = ls2->next;
              continue;
            }
            if (obj2->subtype == PILOT && obj2->mode == LANDED) {
              ls2 = ls2->next;
              continue;
            }
            break;
          case PROJECTILE:
            /* explosion only collide with terrain */
            if (obj2->subtype == EXPLOSION) {
              ls2 = ls2->next;
              continue;
            }

            if (obj1->type == PROJECTILE) {
              /* missiles collides with the others projectiles */
              if (obj1->subtype != MISSILE && obj2->subtype != MISSILE) {
                ls2 = ls2->next;
                continue;
              }
            }
            break;
          case PLANET:
            if (obj1->type == PLANET) {
              ls2 = ls2->next;
              continue;
            }
            break;
          case ASTEROID:
            if (obj1->type == ASTEROID) {
              ls2 = ls2->next;
              continue;
            }
            break;
          case TRACKPOINT:
          case TRACE:
            ls2 = ls2->next;
            continue;
            break;
          default:
            break;
        }

        /***** same team, two ships or shots *****/
        if (plyr[obj1->player].team == plyr[obj2->player].team) {
          if ((obj1->type != PLANET && obj2->type != PLANET)) {
            /* collision between pilot and other object */
            if ((obj1->type == SHIP && obj1->subtype == PILOT) ||
                (obj2->type == SHIP && obj2->subtype == PILOT)) {
              if (obj1->subtype == PILOT && obj2->subtype == PILOT) {
                ls2 = ls2->next;
                continue;
              }

              /* pilot-projectile don't collide */
              if (obj1->type == PROJECTILE || obj2->type == PROJECTILE) {
                ls2 = ls2->next;
                continue;
              }

              /* pilot-satellite don't collide */
              if (obj1->subtype == SATELLITE || obj2->subtype == SATELLITE) {
                ls2 = ls2->next;
                continue;
              }
            } else {
              ls2 = ls2->next;
              continue;
            }
          }
        }
        /***** --same team, two ships or shots *****/

        if (obj2->state <= 0) {
          ls2 = ls2->next;
          continue;
        }

        if (obj1->in != obj2->in) {
          ls2 = ls2->next;
          continue;
        }

        if (obj1->parent == obj2 || obj2->parent == obj1) {
          ls2 = ls2->next;
          continue;
        }
        if (gnet == TRUE) {
          if (plyr[obj1->player].proc == plyr[obj2->player].proc) {
            if (proc != plyr[obj1->player].proc) {
              ls2 = ls2->next;
              continue;
            }
          }
        }

        if (plyr[obj1->player].team == plyr[obj2->player].team) {
          if (!(obj1->type == PLANET || obj2->type == PLANET)) {
            if (obj1->subtype != PILOT && obj2->subtype != PILOT) {
              ls2 = ls2->next;
              continue;
            }
          }
        }

        radio2 = (obj1->radio + obj2->radio) * (obj1->radio + obj2->radio);

        rx = obj2->x - obj1->x;
        ry = obj2->y - obj1->y;
        r2 = rx * rx + ry * ry;

        r0x = obj2->x0 - obj1->x0;
        r0y = obj2->y0 - obj1->y0;
        r02 = 0.25 * ((rx + r0x) * (rx + r0x) + (ry + r0y) * (ry + r0y));

        /***** Experience in combat, among SHIPS *****/
        if (r2 < 1000000) {
          if (obj1->type == SHIP && obj2->type == SHIP) {
            if (plyr[obj2->player].team != plyr[obj1->player].team) {
              if (obj1->habitat == obj1->habitat) {
                if (obj1->subtype != PILOT && obj2->subtype != PILOT) {
                  int il;
                  float factor, points;
                  /* obj1 */
                  if (proc == plyr[obj1->player].proc) {
                    il = obj2->level - obj1->level;
                    factor = 0.001;
                    if (il > 3)
                      il = 3;
                    if (il == -1)
                      factor /= 2;
                    if (il < -1)
                      factor = 0;
                    if (factor > 0) {
                      points = (il + 2) * (il + 2) * factor * (obj2->level + 1);
                      obj1->pexperience += points;
                    }
                  }
                  /* obj2 */
                  if (proc == plyr[obj2->player].proc) {
                    il = obj1->level - obj2->level;
                    if (il > 3)
                      il = 3;
                    factor = 0.001;
                    if (il == -1)
                      factor /= 2;
                    if (il < -1)
                      factor = 0;
                    if (factor > 0) {
                      points = (il + 2) * (il + 2) * factor * (obj1->level + 1);
                      obj2->pexperience += points;
                    }
                  }
                }
              }
            }
          }
        }
        /***** --Experience in combat *****/

        /***** planets and SHIPS *****/
        if (gkplanets == FALSE) {
          /*	if(r2<RADAR_RANGE2){  planet discovered  */
          if ((obj1->type == PLANET && obj2->type == SHIP) ||
              (obj2->type == PLANET && obj1->type == SHIP)) {
            if (obj1->type == PLANET) {
              obj = obj2;
              pnt = obj1;
            } else {
              obj = obj1;
              pnt = obj2;
            }
            if (!((time + obj->id) % 20)) {
              if (proc == plyr[obj->player].proc &&
                  pnt->player != obj->player) {
                if (r2 < obj->radar * obj->radar) { /* planet near */
                  if (IsInIntList((plyr[obj->player].kplanets), pnt->id) == 0) {
                    /* planet discovered */
                    Experience(obj, 40); /* Experience for discover a planet */
                    if (gnet == TRUE) {
                      struct NetMess mess;
                      mess.id = NMPLANETDISCOVERED;
                      mess.a = obj->id; /*  */
                      mess.b = pnt->id; /*  */
                      NetMess(&mess, NMADD);
                    }
                    for (i = 0; i <= gnplayers + 1; i++) {
                      if (plyr[obj->player].team == plyr[i].team ||
                          (GameParametres(GET, GENEMYKNOWN, 0) &&
                           plyr[obj->player].team > 1)) {
                        plyr[i].kplanets =
                            Add2IntList((plyr[i].kplanets), pnt->id);

                        if (plyr[obj->player].team == plyr[i].team) {
                          snprintf(text, MAXTEXTLEN, "(%d) %s %d %s", obj->pid,
                                   GetLocale(L_PLANET), pnt->id,
                                   GetLocale(L_DISCOVERED));
                          if (!Add2TextMessageList(&listheadtext, text, obj->id,
                                                   obj->player, 0, 100, 0)) {
                            Add2CharListWindow(&gameloglist, text, 0,
                                               &windowgamelog);
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        /***** --planets and SHIPS *****/

        if (r2 < radio2 || r02 < radio2) {
          if (obj1->type == PROJECTILE || obj2->type == PROJECTILE) {
            if (obj1->subtype == LASER || obj2->subtype == LASER) {
              if (obj1->subtype == LASER) {
                obj = obj2;
                shot = obj1;
              } else {
                obj = obj1;
                shot = obj2;
              }
              r = (obj->x - shot->x) * sin(shot->a) -
                  (obj->y - shot->y) * cos(shot->a);
              r2 = r * r;
              if (r2 > obj->radio * obj->radio) {
                ls2 = ls2->next;
                continue;
              }
            }
          }

          /*****
                objects and planets
          *****/

          if ((obj1->type == PLANET && obj2->type != PLANET) ||
              (obj2->type == PLANET && obj1->type != PLANET)) {
            /* inner planet */
            if (obj1->type == PLANET) {
              obj = obj2;
              pnt = obj1;
            } else {
              obj = obj1;
              pnt = obj2;
            }
            plyr[obj->player].status = PLAYERMODIFIED;
            listheadplayer.update = 1;

            if (proc == plyr[obj->player].proc ||
                (obj->type == PROJECTILE && obj->subtype == MISSILE)) {
              obj->habitat = H_PLANET;
              obj->planet = NULL;
              obj->in = pnt;

              /*entering planet */
              if (gnet == TRUE) {
                SetModified(obj, SENDOBJAALL);
                obj->ttl = 0; /* send now */
              }
              if (obj == cv) {
                habitat.type = H_PLANET;
                habitat.obj = pnt;
                SelectionBox(NULL, NULL, &cv, 1);
              }
              /* initial conditions at planet */
              a = atan2(-pnt->y + obj->y, -pnt->x + obj->x);
              b = atan2(obj->vy, obj->vx) - a;
              obj->x = LXFACTOR - (a + PI / 2) / (2 * PI) * LXFACTOR;
              obj->y = LYFACTOR;
              v = sqrt(obj->vx * obj->vx + obj->vy * obj->vy);

              obj->vx = -v * sin(b);
              obj->vy = v * cos(b);

              obj->a = obj->a - a + PI / 2;
              if (obj->a > PI)
                obj->a -= 2 * PI;
              if (obj->a < -PI)
                obj->a += 2 * PI;
              if (obj->actorder.id != -1)
                obj->actorder.time = 0;
            }
          }

          /*****
                --objects and planets
          *****/

          /* among ships and projectiles */
          if ((obj1->type != PLANET && obj2->type != PLANET)) {
            /*****
                   between ship and pilots, same team
                   pilot rescued by an allied ship.
            ******/
            if ((plyr[obj1->player].team == plyr[obj2->player].team) &&
                (obj1->type == SHIP && obj2->type == SHIP) &&
                (obj1->subtype == PILOT || obj2->subtype == PILOT) &&
                (obj1->subtype != PILOT || obj2->subtype != PILOT) &&
                (obj1->habitat == obj2->habitat)) {
              if (obj1->subtype == PILOT) {
                objt1 = obj1;
                objt2 = obj2;
              } else {
                objt1 = obj2;
                objt2 = obj1;
              }

              /* objt1 is the pilot */
              if (proc == plyr[objt1->player].proc) {
                if (objt2->subtype == EXPLORER || objt2->subtype == FIGHTER ||
                    objt2->subtype == QUEEN || objt2->subtype == FREIGHTER) {
                  if (CargoAdd(objt2, objt1)) {
                    objt1->in = objt2;
                    objt1->habitat = H_SHIP;

                    if (objt1->player == actual_player) {
                      printf("%s %d %s %d\n", GetLocale(L_PILOT), objt1->pid,
                             GetLocale(L_RESCUEDBY), objt2->pid);
                    }

                    if (gnet == TRUE) {
                      if (proc == plyr[objt1->player].proc) {
                        SetModified(objt1, SENDOBJALL);
                      }
                    }
                    /* pilot is captured: next obj1 */
                    if (obj1 == objt1) {
                      swnextobj1 = 1;
                      break;
                    } /* obj1 don't collide more */
                  }
                  /* next obj2 */
                  break;
                }
              }
            }
            /***** --between ship and pilot same team *****/

            for (i = 0; i < 2; i++) {
              if (i == 0) {
                objt1 = obj1;
                objt2 = obj2;
              }
              if (i == 1) {
                objt1 = obj2;
                objt2 = obj1;
              }

              damage = objt2->damage * (1 - objt1->shield);
              objt1->state -= damage;

              if (objt1->type == SHIP) {
                /*receive an impact */

                Play(objt1, CRASH, 1);
                if (objt1 == cv) {
                  gdraw.crash = 1;
                }

                Explosion(&listheadobjs, cv, objt1, 1);

                if (objt1->state > 0) {
                  if (proc == plyr[objt1->player].proc) {
                    Experience(
                        objt1,
                        damage / 2); /* experience for receive an impact*/
                  }

                  if (objt1->player == actual_player && cv != objt1) {
                    snprintf(text, MAXTEXTLEN, "(%c %d) %s", Type(objt1),
                             objt1->pid, GetLocale(L_HELP));
                    if (!Add2TextMessageList(&listheadtext, text, objt1->id,
                                             objt1->player, 0, 100, 2)) {
                      Add2CharListWindow(&gameloglist, text, 1, &windowgamelog);
                    }
                  }
                }
              }
              if (objt1->type == SHIP || objt1->type == ASTEROID ||
                  (objt1->type == PROJECTILE &&
                   objt1->subtype == MISSILE)) { /* HERE */
                if (gnet == TRUE) {
                  if (proc == plyr[objt1->player].proc) {
                    SetModified(objt1, SENDOBJAALL);
                    objt1->ttl = 0;
                  }
                }
              }

              /***** ship destroyed *****/
              if (objt1->state <= 0) {
                switch (objt1->type) {
                  case SHIP:
                    break;

                  case ASTEROID:
                    if (objt1->subtype < ASTEROID3) {
                      if (proc == plyr[objt1->player].proc) {
                        for (j = 0; j < 3; j++) {
                          nobj = NewObj(
                              ASTEROID, objt1->subtype + 1, objt1->x, objt1->y,
                              objt1->vx + 10.0 * rand() / RAND_MAX - 5,
                              objt1->vy + 10.0 * rand() / RAND_MAX - 5, CANNON0,
                              ENGINE0, 0, NULL, objt1->in);
                          nobj->a = PI / 2;
                          nobj->ai = 0;
                          nobj->in = objt1->in;
                          nobj->habitat = objt1->habitat;
                          nobj->mode = NAV;
                          Add2ObjList(&listheadobjs, nobj);
                        }
                      }
                    }
                    break;
                  default:
                    break;
                }

                switch (objt2->type) {
                  case SHIP:
                    if (objt2->subtype != PILOT) {
                      objt1->sw = objt2->id;
                    }
                    break;
                  case PROJECTILE:
                    if (objt2->parent != NULL) {
                      objt1->sw = objt2->parent->id;
                    }
                    break;
                  default:
                    break;
                }

                if (gnet == TRUE) {
                  if (proc == plyr[objt1->player].proc) {
                    SetModified(objt1, SENDOBJKILL);
                    objt1->ttl = 0;
                  }
                }
              }
              /***** --ship destroyed *****/

            } /* for(i=0;i<2;i++){ */
          }
          /* --among ships and projectiles */
        } /*  if(r2<radio2 || r02<radio2){ */
        else {
          if (r2 < 4 * radio2 && 0) {
            /* Avoid collision */ /* TODO */
          }
        }

        ls2 = ls2->next;
      } /* while ls2!=NULL */ /* --double loop*/
      if (swnextobj1) {
        swnextobj1 = 0;
        ls1 = ls1->next;
        continue;
      }
    } /* if(shocks) */

    /*****
          collision with planet terrain
    *****/
    crashsw = 0;
    if (obj1->mode != LANDED && obj1->habitat == H_PLANET &&
        obj1->y < LYFACTOR / 2) {
      if (proc == plyr[obj1->player].proc || obj1->type == PROJECTILE) {
        if (!GetSegment(&s, obj1->in->planet, obj1->x, obj1->y)) {
          /* if(obj1->y - obj1->radio +1< s.y0 && obj1->y - obj1->radio +1<
           * s.y1){ */
          if (obj1->y - obj1->radio <= s.y0 && obj1->y - obj1->radio <= s.y1) {
            if (obj1->type != SHIP) { /* destroy object */
              obj1->state = 0;
              obj1->sw = 0;

              if (gnet == TRUE) {
                if (proc == plyr[obj1->player].proc) {
                  SetModified(obj1, SENDOBJKILL);
                  obj1->ttl = 0;
                }
              }
            }

            if (obj1->type == SHIP) {
              switch (s.type) {
                case TERRAIN: /* destroy ship */
                  crashsw = 1;
                  break;
                case LANDZONE:
                  if (obj1->subtype == SATELLITE) {
                    crashsw = 1;
                  }
                  if ((fabs(obj1->vx) > 2 || /* ship crashed */
                       fabs(obj1->vy) > 5 || fabs(obj1->a - PI / 2) > .35)) {
                    crashsw = 1;
                  }
                  if (crashsw == 0) { /***** ship has landed *****/
                    if (obj1->vy < 0) {
                      obj1->mode = LANDED;
                      if (obj1->cargo.hlist !=
                          NULL) { /* if has a rescued pilot, eject him */
                        if (CargoEjectObjs(obj1, SHIP, PILOT)) {
                          listheadplayer.update = 1;
                        }
                      }
                      /* FREIGHTER Cargo Goods*/
                      if (obj1->subtype == FREIGHTER &&
                          obj1->oriid != obj1->in->id) {
                        int mass = 0;
                        float d, factor;
                        Object *objori, *objdest;

                        mass = CargoGetMass(obj1); /* mass of objects */

                        /* Adding gold to planet */

                        obj1->destid = obj1->in->id;
                        objori = SelectObj(&listheadobjs, obj1->oriid);
                        objdest = SelectObj(&listheadobjs, obj1->destid);
                        if (objori == NULL || objdest == NULL) {
                          d = 5000;
                        } else {
                          d = sqrt((objori->x - objdest->x) *
                                       (objori->x - objdest->x) +
                                   (objori->y - objdest->y) *
                                       (objori->y - objdest->y));
                        }

                        if (d > GameParametres(GET, GULX, 0) / 10) {
                          factor = 4 * d / 20000;
                          Experience(obj1,
                                     40); /* Experience for complete a travel */
                        } else {
                          factor = 2 * d / 20000;
                          Experience(obj1,
                                     20); /* Experience for complete a travel */
                        }
                        obj1->ntravels++;
                        obj1->in->planet->gold +=
                            (obj1->cargo.mass - mass) * factor;
                        obj1->cargo.mass = mass;
                        obj1->destid = obj1->oriid; /* change dest  */
                        obj1->oriid = obj1->in->id;
                      }
                      /* --FREIGHTER Cargo Goods*/

                      if (gnet == TRUE) {
                        if (proc == plyr[obj1->player].proc) {
                          SetModified(obj1, SENDOBJAALL);
                          obj1->ttl = 0;
                        }
                      }
                      obj1->vx = 0;
                      obj1->vy = 0;
                      obj1->ang_a = 0;
                      obj1->ang_v = 0;
                      obj1->y = obj1->y0 = s.y0 + obj1->radio;
                      if (obj1->subtype == PILOT) {
                        obj1->a = 0;
                      }
                    }

                    /***** conquering a planet *****/
                    switch (IsPlanetEmpty(obj1->in, obj1)) {
                      case 0: /* planet totally empty */
                        if (plyr[obj1->in->player].team !=
                            plyr[obj1->player].team) {
                          char text[MAXTEXTLEN];
                          Experience(obj1,
                                     50); /* Experience for conquest a planet */
                          if (plyr[obj1->in->player].team != 1) { /* HERE ??? */
                            GetInformation(&plyr[obj1->player],
                                           &plyr[obj1->in->player], obj1);
                          }
                          snprintf(text, MAXTEXTLEN, "%s %d %s",
                                   GetLocale(L_PLANET), obj1->in->id,
                                   GetLocale(L_LOST));
                          if (!Add2TextMessageList(
                                  &listheadtext, text, obj1->in->id,
                                  obj1->in->player, 0, 100, 2)) {
                            Add2CharListWindow(&gameloglist, text, 0,
                                               &windowgamelog);
                          }
                          if (gnet == TRUE &&
                              proc != plyr[obj1->in->player].proc) {
                            struct NetMess mess;
                            mess.id = NMPLANETLOST;
                            mess.a = obj1->in->player; /* player that conquer */
                            mess.b = obj1->in->id;     /* id of the planet */
                            NetMess(&mess, NMADD);
                          }
                        }

                        obj1->in->player = obj1->player;
                        obj1->in->sw = 1;

                        if (gnet == TRUE) {
                          if (proc == plyr[obj1->in->player].proc) {
                            SetModified(obj1->in, SENDOBJPLANET);
                            obj1->in->ttl = 0;
                            plyr[obj1->player].modified = SENDPLAYERMOD;
                            obj1->ttl = 0;
                          }
                        }
                        break;

                      case 1: /* only allies landed */
                        break;
                      case 2: /* enemies landed */
                        break;
                      default:
                        break;
                    }
                  } /*****if(crashsw==0) --else ship has landed *****/
                  break;
                default:
                  break;
              }
            } /* if SHIP*/
          } /* if(obj1->y - obj1->radio < s.y0 && obj1->y - obj1->radio < s.y1){
             */
        }   /* if GetSegment()*/
      }     /* if(proc==plyr[obj1->player].proc || obj1->type==PROJECTILE){ */

      if (crashsw) {
        obj1->state = 0;
        obj1->sw = 0;

        if (gnet == TRUE) {
          if (proc == plyr[obj1->player].proc) {
            SetModified(obj1, SENDOBJKILL);
            obj1->ttl = 0;
          }
        }
      }

    } /*      if(obj1->mode!=LANDED && obj1->habitat==H_PLANET ) */

    /* --collision with planet terrain */

    ls1 = ls1->next;
  } /* while(ls1!=NULL) */

  return;
} /* --Collission() */

int UpdateObjs(void) {
  /*
    Update the coordinates of the objects.
  */
  static int cont = 0;
  int i;
  int n = 0;
  struct ObjList* ls;
  Object *obj, *nobj;
  int gnet;
  int proc;
  struct Player* plyr;
  plyr = GetPlayers();

#if CLOSEDUNIVERSE
  int gulx, guly;
  gulx = GameParametres(GET, GULX, 0);
  guly = GameParametres(GET, GULY, 0);
#endif

  gnet = GameParametres(GET, GNET, 0);
  proc = GetProc();
  cont++;

  if (cont > 1000)
    cont = 0;

  ls = listheadobjs.list;

  if (listheadobjs.n == 0)
    return (0);

  /* update planets */
  while (ls != NULL) {
    obj = ls->obj;
    if (obj->type == PLANET) {
      /* planets create some gold */
      obj->planet->A = obj->planet->reggold;
      if (!(cont % 50))
        obj->level = 0; /* reset level */
    }
    ls = ls->next;
  }

  /* --update planets */

  /* update players*/
  for (i = 0; i < GameParametres(GET, GNPLAYERS, 0) + 2; i++) {
    plyr[i].maxlevel = 0;
  }
  /* --update players */

  ls = listheadobjs.list;
  while (ls != NULL) {
    obj = ls->obj;
    obj->pexperience = 0;

    if (obj->durable == TRUE) {
      switch (obj->subtype) {
        case PILOT:
          if (obj->habitat == H_SPACE) {
            obj->life--;
          } else {
            obj->life = LIFEPILOT;
          }
          break;
        case SATELLITE:
          if (obj->habitat == H_SPACE) {
            obj->life--;
          }
          break;
        default:
          obj->life--;
          break;
      }
      if (obj->life <= 0) {
        obj->life = 0;
        obj->state = 0;
        obj->sw = 0;
      }
    }

    if (obj->life > 0) { /* && proc==players[obj->player].proc){ */

      switch (obj->type) {
        case PROJECTILE:
          switch (obj->subtype) {
            case MISSILE:
              aimissile(&listheadobjs, obj);
              UpdateShip(obj);
              break;
            case EXPLOSION:
              UpdateShip(obj);
              break;
            default:
              obj->x0 = obj->x;
              obj->y0 = obj->y;

              obj->x += obj->vx * DT;
              obj->y += obj->vy * DT;
              break;
          }
          break;
        case SHIP:
          if (obj->trace == TRUE && obj->mode != LANDED) {
            if (obj->vx * obj->vx + obj->vy * obj->vy > .25) {
              Object* objin = obj->in;

              if (objin != NULL) { /* don't create trace inside a ship */
                if (obj->in->type != PLANET) {
                  objin = NULL;
                }
              }
              nobj = NewObj(TRACE, SHIP0, obj->x, obj->y, 0, 0, CANNON0,
                            ENGINE0, obj->player, obj, objin);
              Add2ObjList(&listheadobjs, nobj);
            }
          }

          switch (obj->subtype) {
            case EXPLORER:
            case FIGHTER:
            case QUEEN:
            case FREIGHTER:
            case TOWER:
              /* level of a planet is the maxlevel of an object in it */
              if (obj->habitat == H_PLANET) {
                if (plyr[obj->player].team == plyr[obj->in->player].team) {
                  if (obj->level > obj->in->level)
                    obj->in->level = obj->level;
                }
              }
              if (obj->level > plyr[obj->player].maxlevel) {
                plyr[obj->player].maxlevel = obj->level;
                if (plyr[obj->player].maxlevel > plyr[obj->player].gmaxlevel) {
                  plyr[obj->player].maxlevel = plyr[obj->player].gmaxlevel;
                }
              }
            case SATELLITE:
            case PILOT:
              if (proc == plyr[obj->player].proc) {
                ai(&listheadobjs, obj, actual_player);
              }
              UpdateShip(obj);

              break;
            default:
              g_print("ERROR (UpdateObjs 1) %d %d \n", obj->type, obj->subtype);
              exit(-1);
              break;
          }
          break;
        case ASTEROID:
          UpdateAsteroid(obj);
          break;

        case TRACKPOINT:
        case TRACE:
          break;
        case PLANET:
          break;

        default:
          g_print("ERROR (UpdateObjs 2) %d\n", obj->type);
          exit(-1);
          break;
      }
#if CLOSEDUNIVERSE
      if (obj->x > gulx)
        obj->x -= gulx;
      if (obj->x < 0)
        obj->x += gulx;
      if (obj->y > guly)
        obj->y -= guly;
      if (obj->y < 0)
        obj->y += guly;
#endif
    } /* if (obj->life>0) */

    if (obj->life <= 0) {
      obj->state = 0;
      obj->sw = 0;
    }

    if (gnet == TRUE) {
      obj->ttl--;
      if (obj->life <= 0) {
        if (proc == plyr[obj->player].proc) {
          SetModified(obj, SENDOBJKILL);
          obj->ttl = 0;
        }
      }
    }
    ls = ls->next;
    n++;
    if (n > listheadobjs.n) {
      g_print("ERROR: n: %d  N: %d\n", n, listheadobjs.n);
      return (-1);
    }
  } /* while(ls!=NULL) */
  return (n);
}

int CheckPlanetDistance(struct HeadObjList* lh, float x, float y, int d) {
  /*
    The distance between two planets must be greater than 600
    returns:
    1 if there are a near planet
    0 if not
  */

  struct ObjList* ls;
  float d2 = d * d;

  ls = lh->list;
  while (ls != NULL) {
    if (ls->obj->type == PLANET) {
      if ((ls->obj->x - x) * (ls->obj->x - x) +
              (ls->obj->y - y) * (ls->obj->y - y) <
          d2) {
        return (1);
      }
    }
    ls = ls->next;
  }
  return (0);
}

void CreateUniverse(int ulx,
                    int uly,
                    struct HeadObjList* lheadobjs,
                    char** ptnames) {
  /*
    Create Galaxies and planets.
  */

  int i, j;
  float x, y;
  float x0, y0;
  float r, rg, ru; /* radio galaxy and universe */
  float d2, d2min, d2limit;
  Object* obj;
  int n, np = 0;
  int nplanetspergalaxy = 1;
  int nplanets;
  int ngalaxies = 1;
  int valid;
  Point* gpos;
  char* datadir;
  char frectable[128];
  FILE* fp;
  int namegen = FALSE;

  /* HERE check this equation. galaxy size */

  datadir = DATADIR;
  snprintf(frectable, 128, "%s/%s", datadir, LETTERFREQUENCYFILE);
  printf("Checking for letter frequency file: %s\n", frectable);

  namegen = TRUE;
  if ((fp = fopen(frectable, "rt")) == NULL) {
    fprintf(stdout, "Can't open the file: %s\n", frectable);

    datadir = INSTALL_DATA_DIR;
    snprintf(frectable, 128, "%s/%s", datadir, LETTERFREQUENCYFILE);
    printf("checking for letter frequency file 2 :%s\n", frectable);

    if ((fp = fopen(frectable, "rt")) == NULL) {
      fprintf(stdout, "Can't open the file: %s\n", frectable);
      namegen = FALSE;
    }
  }

  if (namegen) {
    loadFrequencyTable(frectable);
    printf("letter frequency table loaded\n");
    fclose(fp);
  }

  ngalaxies = GameParametres(GET, GNGALAXIES, 0);
  nplanets = GameParametres(GET, GNPLANETS, 0);
  nplanetspergalaxy = (int)((float)nplanets / ngalaxies + 0.5);
  ru = ulx / 2;
  uly = ulx; /* universe is square */

  gpos = malloc(ngalaxies * sizeof(Point));
  if (gpos == NULL) {
    fprintf(stderr, "ERROR in malloc CreateUniverse()\n");
    exit(-1);
  }

  for (i = 0; i < ngalaxies; i++) {
    gpos[i].x = 0;
    gpos[i].y = 0;
  }

  if (ngalaxies == 1) {
    rg = ulx / 3;
  } else {
    rg = ulx / (sqrt(ngalaxies * 16));
  }

  printf("\n\tnumber of galaxies: %d\n", ngalaxies);
  printf("\tnumber of planets per galaxy: %d\n", nplanetspergalaxy);
  printf("\tgalaxy radius: %.0f\n", rg);

  /* galaxy center coordinates */

  x0 = 2 * (ru - rg) * Random(-1) - (ru - rg);
  y0 = 2 * (ru - rg) * Random(-1) - (ru - rg);
  gpos[0].x = x0;
  gpos[0].y = y0;
  d2limit = 9 * (float)rg * rg;
  for (j = 1; j < ngalaxies; j++) {
    n = 0;
    do {
      x0 = 2 * (ru - rg) * Random(-1) - (ru - rg);
      y0 = 2 * (ru - rg) * Random(-1) - (ru - rg);
      d2min = (float)ulx * ulx;
      n++;
      for (i = 0; i < j; i++) {
        d2 = (x0 - gpos[i].x) * (x0 - gpos[i].x) +
             (y0 - gpos[i].y) * (y0 - gpos[i].y);
        if (d2 < d2min)
          d2min = d2;
      }
      if (n > 100) {
        n = 0;
        d2limit *= .8; /* relax condition */
      }
    } while (d2min < d2limit);
    gpos[j].x = x0;
    gpos[j].y = y0;
  }

  for (j = 0; j < ngalaxies; j++) {
    if (np >= nplanets)
      break;
    x0 = gpos[j].x;
    y0 = gpos[j].y;
    if (ngalaxies == 1) {
      x0 = 0;
      y0 = 0;
    }
    for (i = 0; i < nplanetspergalaxy; i++) {
      if (np >= nplanets)
        break;
      n = 0;
      do {
        valid = 1;
        x = x0 + 6 * rg * (float)rand() / RAND_MAX - 3 * rg;
        y = y0 + 6 * rg * (float)rand() / RAND_MAX - 3 * rg;
        if (n > 100) {
          fprintf(stderr,
                  "ERROR: Universe size too small or too much planets. "
                  "Exiting...\n");
          exit(-1);
        }

        if (CheckPlanetDistance(lheadobjs, x, y, 600)) {
          valid = 0;
          n++;
        }
        /* planet distribution */
        d2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
        r = (float)rand() / RAND_MAX;
        if (r > 5 * exp(-d2 / (1.44 * rg * rg / 2.)))
          valid = 0;

        if (x < -ulx / 2 || x > ulx / 2 || y < -uly / 2 || y > uly / 2) {
          valid = 0;
        }
      } while (valid == 0);

      obj = NewObj(PLANET, SHIP0, x, y, 0, 0, CANNON0, ENGINE0, 0, NULL, NULL);
      if (obj != NULL) {
        if (i < NUMPLANETNAMES - 1 && j == 0) {
          strncpy(obj->name, ptnames[i + 1], OBJNAMESMAXLEN);
        } else {
          if (namegen) {
            strncpy(obj->name, getRandomName(0), OBJNAMESMAXLEN);
          } else {
            strncpy(obj->name, ptnames[0], OBJNAMESMAXLEN);
          }
        }
        Add2ObjList(lheadobjs, obj);
        np++;
      }
    }
  }

  if (np < nplanets) {
    int m = nplanets - np;
    for (i = 0; i < m; i++) {
      x = ulx * Random(-1) - ulx / 2;
      y = uly * Random(-1) - uly / 2;

      obj = NewObj(PLANET, SHIP0, x, y, 0, 0, CANNON0, ENGINE0, 0, NULL, NULL);
      if (obj != NULL) {
        if (i < NUMPLANETNAMES - 1 && j == 0) {
          strncpy(obj->name, ptnames[i + 1], OBJNAMESMAXLEN);
        } else {
          strncpy(obj->name, ptnames[0], OBJNAMESMAXLEN);
        }
        Add2ObjList(lheadobjs, obj);
        np++;
      }
    }
  }

  if (np != nplanets) {
    printf("WARNING CreateUniverse(): number of planets incorrect: %d\n", np);
    exit(-1);
  }

  free(gpos);
}

float PlanetAtraction(float* fx, float* fy, float x, float y, float m) {
  /*
    calculates the atraction forces betwwen ships and planets
    returns:
    the force in fx,fy arguments.
    the potential energy.
  */

  struct ObjList* ls;
  float rx, ry, r, r2, ir2r;
  float U; /* potential*/
  float GM;

  if (m == 0) {
    *fx = *fy = 0;
    return (0);
  }

  ls = listheadplanets.list;
  U = 0;

  while (ls != NULL) {
    rx = ls->obj->x - x;
    ry = ls->obj->y - y;
    r2 = rx * rx + ry * ry;

    if (r2 < GRAVITATION_RANGE2) {
      GM = G * ls->obj->mass;
      r = sqrt(r2);
      ir2r = 1. / (r2 * r);
      *fx += GM * rx * ir2r;
      *fy += GM * ry * ir2r;
      U -= GM / r;
    }
    ls = ls->next;
  }

  *fx *= m;
  *fy *= m;
  U *= m;

  return (U);
}

int PrintfObjInfo(FILE* fp, Object* obj) {
  /*
    printf obj info
    used only for debug.
  */

  int in, dest, parent, planet;
  fprintf(fp,
          "time: %d\n id:%d durable:%d visible:%d\n \
points:%d habitat:%d mode:%d \nmodified: %d \n	       \
x:%g y:%g x0:%g y0:%g \nvx:%g vy:%g \n		       \
fx0:%g fy0:%g fx:%g fy:%g\n a:%g ang_v:%g accel:%g \n  \
gas:%g gas_max:%g life:%g shield:%g state:%g\n",
          GetTime(), obj->id, obj->durable, obj->visible, obj->kills,
          obj->habitat, obj->mode, obj->modified, obj->x, obj->y, obj->x0,
          obj->y0, obj->vx, obj->vy, obj->fx0, obj->fy0, obj->fx, obj->fy,
          obj->a, obj->ang_v, obj->accel, obj->gas, obj->gas_max, obj->life,
          obj->shield, obj->state);

  fprintf(fp,
          "dest_r2:%g mass:%d radio:%d \ntype:%d subtype:%d\n\
 damage:%d player:%d player:%d\nai:%d trace:%d norder:%d\n",
          obj->dest_r2, obj->mass, obj->radio, obj->type, obj->subtype,
          obj->damage, obj->player, obj->player, obj->ai, obj->trace,
          obj->norder);

  in = 0;
  dest = 0;
  planet = 0;
  parent = 0;

  if (obj->in != NULL)
    in = obj->in->id;
  if (obj->dest != NULL)
    dest = obj->dest->id;
  if (obj->parent != NULL)
    parent = obj->parent->id;
  if (obj->planet != NULL)
    planet = obj->id;

  fprintf(fp, "parent:%d dest:%d in:%d planet:%d\n weapon:%d engine:%d\n ",
          parent, dest, in, planet, obj->weapon->type, obj->engine.type);

  fprintf(fp, "parent:%p dest:%p in:%p planet:%p\n", (void*)obj->parent,
          (void*)obj->dest, (void*)obj->in, (void*)obj->planet);

  return (0);
}

Object* ChooseInitPlanet(struct HeadObjList lheadobjs) {
  /*
    returns:
    a pointer to a free planet.
  */

  struct ObjList* ls;
  int n, cont = 0;
  int nplanets = 0;
  int rplanet;

  /* counting planets */
  ls = lheadobjs.list;
  while (ls != NULL) {
    if (ls->obj->type == PLANET) {
      cont++;
    }
    ls = ls->next;
  }

  nplanets = cont;
  n = 0;

  do {
    rplanet = (int)(nplanets * Random(-1));

    ls = lheadobjs.list;
    cont = 0;
    while (ls != NULL) {
      if (ls->obj->type == PLANET) {
        if (ls->obj->player == 0 && cont == rplanet) { /* a random planet */
          return (ls->obj);
        }
      }
      ls = ls->next;
      cont++;
    }
    n++;
  } while (n < 10);

  cont = 0;
  ls = lheadobjs.list;
  while (ls != NULL) {
    if (ls->obj->type == PLANET) {
      if (ls->obj->player == 0) { /* the first free planet */
        return (ls->obj);
      }
    }
    ls = ls->next;
    cont++;
  }

  fprintf(stderr,
          "ERROR ChooseInitPlanet(): Too many players or too few planets\n");
  return (NULL);
}

void CreateShips(struct HeadObjList* lheadobjs) {
  /*
    Choose initial planet.
    Create the initial player ships
  */
  Object *obj, *planet;
  Segment* s;
  int i;
  struct Player* plyr;
  plyr = GetPlayers();

  for (i = 1; i <= GameParametres(GET, GNPLAYERS, 0); i++) {
    planet = ChooseInitPlanet(*lheadobjs);
    if (planet == NULL) {
      fprintf(stderr, "ERROR CreateShips(): obj==NULL\n");
      exit(-1);
    }
    s = LandZone(planet->planet);
    if (s == NULL) {
      fprintf(stderr, "ERROR CreateShips(): Segment==NULL\n");
      exit(-1);
    }

    obj = NewObj(SHIP, QUEEN, 0, 0, 0, 0, CANNON5, ENGINE5, i, NULL, planet);

    obj->x = s->x0 + (s->x1 - s->x0) * (Random(-1));

    obj->y = obj->y0 = (s->y0) + obj->radio + 1;
    obj->a = PI / 2;
    obj->ai = 1;
    obj->habitat = H_PLANET;
    obj->mode = LANDED;

    Add2ObjList(lheadobjs, obj);
    plyr[obj->player].nbuildships++;

    planet->player = obj->player;

    if (GameParametres(GET, GNET, 0) == TRUE) {
      if (GetProc() == plyr[planet->player].proc) {
        SetModified(planet, SENDOBJPLANET);
        planet->sw = 1;
        planet->ttl = 0;
      }
    }

    plyr[obj->player].kplanets =
        Add2IntList((plyr[obj->player].kplanets), planet->id);
    plyr[obj->player].nplanets++;

    if (GameParametres(GET, GKPLANETS, 0) == FALSE) {
      int j, k;

      for (j = -2; j < 3; j++) {
        for (k = -2; k < 3; k++) {
          if (j * j + k * k < 8) {
            Add2IntIList(&(plyr[obj->player].ksectors),
                         Quadrant(planet->x + j * SECTORSIZE,
                                  planet->y + k * SECTORSIZE));
          }
        }
      }
    }
    if (i == actual_player) {
      cv = obj;
      ship_c = obj;
      habitat.type = H_PLANET;
      habitat.obj = planet;
    }
  }
  return;
}

int CheckGame(char* cad, int action) {
  /*
    Check some game consistence.
    Used every step (!!HERE check this)
    Used after load a game.
    action:
    0 only show messages
    1 try to correct errors
  */

  struct ObjList* ls;
  struct OrderList* lo;
  Object* obj;
  int i;
  int n = 0;
  int types[ALLOBJS];
  int type;
  int proc;
  int nplayers = 0;

  struct Player* plyr;
  plyr = GetPlayers();

  proc = GetProc();
  nplayers = (GameParametres(GET, GNPLAYERS, 0));
  /* Checking Orders */

  for (i = 0; i < ALLOBJS; i++) {
    types[i] = 0;
  }

  printf("%s", cad);

  ls = listheadobjs.list;
  while (ls != NULL) {
    obj = ls->obj;

    type = obj->type + 1;
    if (type > ALLOBJS)
      type = ALLOBJS;
    types[type]++;

    if (proc == plyr[obj->player].proc) {
      n = 0;
      lo = obj->lorder;
      while (lo != NULL) {
        n++;
        lo = lo->next;
      }
      if (n != obj->norder) {
        fprintf(stderr, "ERROR CheckGame():norder incorrect\n");
        printf("\tObj: %d, type: %d norder=%d , size of list: %d\n", obj->id,
               obj->type, obj->norder, n);
        printf("\tDeleting all the orders of object %d\n", obj->id);
        DelAllOrder(obj);
      }
    }

    if (obj->player < 0 || obj->player > nplayers + 2) {
      fprintf(stderr, "\tError CheckGame() obj: %d player id: %d invalid.\n",
              obj->id, obj->player);
    }

    if (obj->type == SHIP && obj->subtype == TOWER &&
        obj->habitat != H_PLANET) {
      fprintf(stderr,
              "\tError CheckGame() obj %d TOWER not in planet. habitat: %d . "
              "player: %d \n",
              obj->id, obj->habitat, obj->player);
      printf("\tRemoving the ship (%d)%d\n", obj->player, obj->pid);
      obj->state = 0;
      SetModified(obj, SENDOBJKILL);
      obj->ttl = 0;
      ls = ls->next;
      continue;
    }

    /* if planet don't belongs to a landed ship */
    if (obj->type == SHIP) {
      if (obj->radar < 2000) {
        printf("SHIP %d(%d) player: %d with radar: %d\n", obj->pid, obj->id,
               obj->player, obj->radar);
      }
      if (obj->subtype == FIGHTER) {
        if (obj->items & ITSURVIVAL && obj->level < MINLEVELPILOT) {
          printf(
              "Error ship %d level: %d items: %d  => Removed survival pod.\n",
              obj->id, obj->level, obj->items);
          obj->items = obj->items & (~ITSURVIVAL);
        }

        if (fabs(obj->cost -
                 COSTFIGHTER * COSTFACTOR * pow(COSTINC, obj->level)) > 0.01) {
          printf("Error ship %d (%d-%d) level: %d cost: %f ->  %f \n", obj->id,
                 obj->player, obj->pid, obj->level, obj->cost,
                 0.01 * COSTFACTOR * pow(COSTINC, obj->level));
          obj->cost = 0.01 * COSTFACTOR * pow(COSTINC, obj->level);
        }
      }
      if (obj->mode == LANDED && obj->in == NULL) {
        fprintf(stderr, "\tError CheckGame() obj %d landed and in NULL\n",
                obj->id);
        ls = ls->next;
        continue;
      }
      if (obj->mode == LANDED && obj->in != NULL && obj->habitat == H_PLANET) {
        if (obj->in->planet == NULL) {
          fprintf(stderr, "\tError CheckGame() obj %d planet not assigned\n",
                  obj->id);
          ls = ls->next;
          continue;
        }
      }
      if (obj->habitat != H_SPACE && obj->in == NULL) {
        fprintf(stderr,
                "\tError CheckGame() obj %d not in space and in = NULL\n",
                obj->id);
        fprintf(stderr, "\t\ttype: %d stype: %d\n", obj->type, obj->subtype);
        fprintf(stderr, "\t\thabitat: %d\n", obj->habitat);
        if (action) {
          printf("\tRemoving the ship (%d)%d\n", obj->player, obj->pid);
          obj->state = 0;
          SetModified(obj, SENDOBJKILL);
          obj->ttl = 0;
        }

        ls = ls->next;
        continue;
      }
      if (obj->habitat == H_SHIP) {
        int sw = 0;
        if (obj->in == NULL) {
          fprintf(stderr, "\tError CheckGame() obj %d in ship and in= NULL\n",
                  obj->id);
          sw++;
        }

        if (obj->in != NULL) {
          if (CargoIsObj(obj->in, obj) == 0) {
            /* if((obj->in->items & ITPILOT)==0){ */
            sw++;
            printf(
                "\tError CheckGame() obj (%d)%d (%d,%d) has no a pilot (%d)%d "
                "(%d,%d)\n",
                obj->in->player, obj->in->pid, obj->in->type, obj->in->subtype,
                obj->player, obj->pid, obj->type, obj->subtype);
            if (action) {
              printf("\tRemoving the ship (%d)%d\n", obj->player, obj->pid);
              obj->state = 0;
              SetModified(obj, SENDOBJKILL);
              obj->ttl = 0;
            }
          }
        }
        if (sw) {
          ls = ls->next;
          continue;
        }
      }
    }

    ls = ls->next;
  }
  return (0);
}

void Density(void) {
  /*

    Not used by now

  */
  struct ObjList* ls;
  int i, j;
  int den[10][10];
  float x, y;

  for (i = 0; i < 10; i++)
    for (j = 0; j < 10; j++)
      den[i][j] = 0;

  ls = listheadobjs.list;
  while (ls != NULL) {
    if (ls->obj->type == PLANET) {
      if (ls->obj->habitat == H_PLANET) {
        x = ls->obj->in->planet->x;
        y = ls->obj->in->planet->y;
      } else {
        x = ls->obj->x;
        y = ls->obj->y;
      }

      i = 10. * x / (float)GameParametres(GET, GULX, 0);
      j = 10. * y / (float)GameParametres(GET, GULY, 0);
      i += 5;
      j += 5;
      if (i < 0)
        i = 0;
      if (j < 0)
        j = 0;
      if (i > 9)
        i = 9;
      if (j > 9)
        j = 9;
      den[i][j]++;
    }
    ls = ls->next;
  }

  for (i = 0; i < 10; i++) {
    for (j = 0; j < 10; j++) {
    }
  }
}

void GetUniverse(void) {
  struct Player* plyr;

  /*ExecLoad("kk"); */
  ExecLoad(savefile);
  plyr = GetPlayers();
  plyr[actual_player].status = PLAYERMODIFIED;
  p_time = GetTime();
  CheckGame("Checking game after load...", 1);
  printf("done\n");
}

void NetComm(void) {
  /* net gamming */

  /* orders to  thread:
     OTSENDOBJS: send modified objects.
     OTSENDALLOBJS save: send all the objects.
     OTSENDLOAD load: load all the objects.
     OTSENDKILL to finish: quit the program.
  */
  int mode;
  int fd;
  struct Keys* key;

  mode = GameParametres(GET, GMODE, 0);
  order2thread = OTSENDOBJS;
  key = GetKeys();

  if (mode == SERVER) {
    if (key->save == TRUE) {
      order2thread = OTSENDSAVE;
    }

    if (key->load == TRUE) {
      /* check if is possible open the dir */
      /* checking the file */
      if ((fd = open(savefile, O_RDONLY)) == -1) {
        fprintf(stdout, "CommServer()[OTSENDLOAD]: Can'T open the file: %s\n",
                savefile);
      } else {
        order2thread = OTSENDLOAD;
      }
      close(fd);
    }
  }

  if (GameParametres(GET, GQUIT, 0) == 2) {
    order2thread = OTSENDKILL;
  }

  switch (mode) {
    case SERVER:
      LoadBuffer(order2thread, &buffer1, SERVER);
      sem_post(&sem_barrier1); /* run comm. thread*/
      break;
    case CLIENT:
      sem_post(&sem_barrier1); /* run comm. thread*/
      break;
    default:
      break;
  }

  sem_wait(&sem_barrier); /* HERE send this far away in code */
}

void int_handler(int sign) {
  fprintf(stderr, "signal n %d received (SIGINT)\n", sign);
  Quit(NULL, NULL);
}

void segfault_handler(int sign) {
  fprintf(stderr, "signal n %d received (SEGFAULT)\n", sign);
  if (ExecSave(listheadobjs, SAVETMPFILE) != 0) {
    fprintf(stderr, "Error in main(): I can't open %s\n", SAVETMPFILE);
    exit(-1);
  }
  fprintf(stderr, "%s %s\n", GetLocale(L_GAMESAVEDTO), SAVETMPFILE);
  exit(-1);
  /*  Quit(NULL,NULL); */
}

void GetGold(void) {
  /*
    Count the number of planets that belongs to each player
    Increase players gold
  */

  struct ObjList* ls;
  int nplayers;
  int i;
  int proc;
  float inctower;
  float levelfactor;
  struct Player* plyr;
  plyr = GetPlayers();

  proc = GetProc();
  nplayers = GameParametres(GET, GNPLAYERS, 0);

  for (i = 0; i < nplayers + 2; i++) {
    plyr[i].nplanets = 0;
    plyr[i].nships = 0;
    plyr[i].balance = 0;
    plyr[i].level = 0;
  }

  ls = listheadobjs.list;
  while (ls != NULL) {
    switch (ls->obj->type) {
      case PLANET:
        plyr[ls->obj->player].nplanets++;
        break;
      case SHIP:
        if (ls->obj->subtype != SATELLITE) {
          plyr[ls->obj->player].nships++;
          plyr[ls->obj->player].level += ls->obj->level;
        }
        break;
      default:
        ls = ls->next;
        continue;
        break;
    }

    if (proc != plyr[ls->obj->player].proc) {
      ls = ls->next;
      continue;
    }
    if (ls->obj->player == 0) {
      ls = ls->next;
      continue;
    }

    switch (ls->obj->type) {
      case PLANET:
        /*gold for each planet*/
        plyr[ls->obj->player].balance += .01 * RESOURCEFACTOR;
        break;
      case SHIP:
        if (ls->obj->subtype == PILOT) {
          plyr[ls->obj->player].balance -= 0.25 * ls->obj->cost;
          ls = ls->next;
          continue;
        }
        plyr[ls->obj->player].balance -= ls->obj->cost;

        switch (ls->obj->subtype) {
          case TOWER:
            levelfactor = (1.0 + 1.0 * ls->obj->level);

            if (ls->obj->in->planet->gold > 0) {
              inctower = (.4 + 0.1 * levelfactor) * RESOURCEFACTOR;
              if (ls->obj->in->planet->gold > inctower) {
                plyr[ls->obj->player].balance += inctower;
                ls->obj->in->planet->gold -= inctower;
                ls->obj->in->planet->A -= 0.015;
                if (ls->obj->in->planet->A < 0)
                  ls->obj->in->planet->A = 0;
              } else {
                plyr[ls->obj->player].balance += ls->obj->in->planet->gold;
                ls->obj->in->planet->gold = 0;
              }
            } else {
              ls->obj->in->planet->gold = 0;
              if (ls->obj->in->planet->A > 0.015) {
                plyr[ls->obj->player].balance += 0.015 * levelfactor;
                ls->obj->in->planet->A -= 0.015;
                if (ls->obj->in->planet->A < 0)
                  ls->obj->in->planet->A = 0;
              } else {
                plyr[ls->obj->player].balance +=
                    ls->obj->in->planet->A * levelfactor;
                ls->obj->in->planet->A = 0.0;
              }
            }

            break;

          default:
            break;
        }

        break;
      default:
        break;
    }
    ls = ls->next;
  }

  for (i = 0; i < nplayers + 2; i++) {
    if (proc == plyr[i].proc) {
      plyr[i].gold += plyr[i].balance;
    }
  }
}

void GetPoints(struct HeadObjList* hol, int proc, struct Player* p) {
  /*
     version 0.3    12Dic2010

     if state is 0 sum points and experience
  */

  struct ObjList* ls;
  Object* obj; /* dead object */
  int sw = 0;
  int gnet;
  struct Player* plyr;
  plyr = GetPlayers();

  ls = hol->list;
  gnet = GameParametres(GET, GNET, 0);

  while (ls != NULL) {
    obj = ls->obj;
    sw = 0;
    if (gnet == TRUE) {
      if (obj->modified == SENDOBJDEAD)
        sw = 1;
    } else {
      if (obj->state <= 0)
        sw = 1;
    }

    if (obj->type == SHIP && obj->subtype == PILOT && obj->sw != 0) {
      sw = 1;
    }

    if (sw) {
      GetPointsObj(hol, plyr, obj); /* points and experience */
    }

    /*****
           Partial experience
           By now, only due to proximity with enemies
    ******/
    if (obj->pexperience > 0) {
      obj->pexperience = obj->pexperience > 0.1 ? 0.1 : obj->pexperience;
      Experience(obj, obj->pexperience);
      obj->pexperience = 0;
    }

    ls = ls->next;
  }
  return;
}

void GetPointsObj(struct HeadObjList* lhobjs, struct Player* p, Object* obj) {
  /*
     version 0.3    12Dic2010
     obj   dead object
     if state is 0 sum points and experience
  */

  Object* obj2; /* killer */
  Object* obj3; /* who receive points */
  int il;
  float factor, points;

  if (obj == NULL)
    return;
  if (obj->sw == 0)
    return;

  obj2 = obj3 = NULL;

  switch (obj->type) {
    case PROJECTILE:
      /* points to nobody */

      break;
    case SHIP:
      /* points to the killer */
      if (obj->subtype == PILOT) { /* no points for killing a pilot */
        break;
      }
      obj2 = SelectObj(lhobjs, (obj->sw));

      if (obj2 != NULL) {
        obj3 = NULL;
        if (obj2->type == SHIP)
          obj3 = obj2;
        if (obj3 != NULL) {
          /* must be a SHIP */
          obj3->kills++;
          p[obj3->player].nkills++;

          /* Experience for kill an enemy */
          il = obj->level - obj3->level;
          if (il > 3)
            il = 3;
          factor = 50;
          if (il < 0)
            factor /= 2;
          if (il < -1)
            factor /= 2;
          if (il < -2)
            factor /= 2;
          if (factor > 0) {
            points = factor * pow(2, obj->level);
            if (obj->subtype == PILOT && obj->state <= 0)
              points /= 2;
            if (points < 10)
              points = 10;
            Experience(obj3, points);
          }
          /* --Experience for kill an enemy */
        }
      }
      obj->sw = 0;
      break;
    case ASTEROID:

      obj2 = SelectObj(lhobjs, (obj->sw));
      if (obj2 != NULL) {
        obj3 = NULL;
        /* 	    if(obj2->type==PROJECTILE){ */
        /* 	      obj3=SelectObj(&listheadobjs,obj2->parent->id); */
        /* 	    } */
        if (obj2->type == SHIP)
          obj3 = obj2;
        if (obj3 != NULL) {
          /* must be a SHIP */

          switch (obj->subtype) {
            case ASTEROID1:
              p[obj3->player].gold += 50;
              break;
            case ASTEROID2:
              p[obj3->player].gold += 100;
              break;
            case ASTEROID3:
              p[obj3->player].gold += 200;
              break;
            default:
              fprintf(stderr,
                      "ERROR in GetGold():asteroid subtype %d unknown\n",
                      obj->subtype);
              exit(-1);
              break;
          }
        }
      }
      obj->sw = 0;
      break;
    default:
      break;
  }

  return;
}

void PrintGameOptions(void) {
  printf("actual game options:\n");
  printf("\tUniverse size: %d\n", GameParametres(GET, GULX, 0));
  printf("\tNumber of Galaxies: %d\n", GameParametres(GET, GNGALAXIES, 0));
  printf("\tNumber of planets: %d\n", GameParametres(GET, GNPLANETS, 0));
  printf("\tNumber of players: %d\n", GameParametres(GET, GNPLAYERS, 0));
  if (GameParametres(GET, GKPLANETS, 0) == TRUE) {
    printf("\tPlanets known by all the players.\n");
  }
  if (GameParametres(GET, GKPLANETS, 0))
    printf("\tknown planets: yes\n");
  else
    printf("\tknown planets: no\n");

  if (GameParametres(GET, GCOOPERATIVE, 0))
    printf("\tcooperative mode: yes\n");
  else
    printf("\tcooperative mode: no\n");

  if (GameParametres(GET, GCOMPCOOPERATIVE, 0))
    printf("\tcomputer cooperative mode: yes\n");
  else
    printf("\tcomputer cooperative mode: no\n");

  if (GameParametres(GET, GQUEEN, 0))
    printf("\tQueen mode: yes\n");
  else
    printf("\tQueen mode: no\n");

  if (GameParametres(GET, GPIRATES, 0))
    printf("\tpirates: yes\n");
  else
    printf("\tpirates: no\n");

  if (GameParametres(GET, GENEMYKNOWN, 0))
    printf("\tEnemy known: yes\n");
  else
    printf("\tEnemy known: no\n");

  if (GameParametres(GET, GNET, 0))
    printf("\tnet game: yes\n");
  else
    printf("\tnet game: no\n");

  printf("\tSound: %d  volume: %d\n", GameParametres(GET, GSOUND, 0),
         GameParametres(GET, GSOUNDVOL, 0));
  printf("\tMusic: %d  volume: %d\n", GameParametres(GET, GMUSIC, 0),
         GameParametres(GET, GMUSICVOL, 0));
}

void SetGameParametres(struct Parametres param) {
  int width, height;

  GameParametres(SET, DEFAULT, 0); /* defaults game values */

  if (gfont == NULL) {
    GameParametres(SET, GPANEL, PANEL_HEIGHT);
  } else {
    GameParametres(SET, GPANEL, 2 * gdk_text_height(gfont, "pL", 2));
  }

  if (param.server == TRUE || param.client == TRUE) {
    GameParametres(SET, GNET, TRUE);
    printf("NET\n");
    if (param.server == TRUE) {
      GameParametres(SET, GMODE, SERVER);
    }
    if (param.client == TRUE) {
      GameParametres(SET, GMODE, CLIENT);
    }
  }

  GameParametres(SET, GNGALAXIES, param.ngalaxies);
  GameParametres(SET, GNPLANETS, param.nplanets);
  GameParametres(SET, GNPLAYERS, param.nplayers); /*+1 system +1 pirates */

  if (param.kplanets == TRUE) {
    GameParametres(SET, GKPLANETS, TRUE);
  }

  if (param.pirates == FALSE) {
    GameParametres(SET, GPIRATES, FALSE);
  }

  if (param.enemyknown == TRUE) {
    GameParametres(SET, GENEMYKNOWN, TRUE);
  }

  GameParametres(SET, GULX, param.ul);
  GameParametres(SET, GULY, param.ul);

  GameParametres(SET, GCOOPERATIVE, param.cooperative);
  GameParametres(SET, GCOMPCOOPERATIVE, param.compcooperative);

  GameParametres(SET, GMUSIC, param.music);
  GameParametres(SET, GSOUND, param.sound);
  GameParametres(SET, GSOUNDVOL, param.soundvol);
  GameParametres(SET, GMUSICVOL, param.musicvol);

  GameParametres(SET, GWIDTH, DEFAULTWIDTH);
  GameParametres(SET, GHEIGHT, DEFAULTHEIGHT);
  GetGeom(param.geom, &width, &height);
  GameParametres(SET, GWIDTH, width);
  GameParametres(SET, GHEIGHT, height - GameParametres(GET, GPANEL, 0));

  printf("W: %d H: %d \n", GameParametres(GET, GWIDTH, 0),
         GameParametres(GET, GHEIGHT, 0));
  printf("W: %d H: %d \n", width, height);

  /*********** process identifier ***********/

  SetProc(0);
  SetNProc(1);

  if (GameParametres(GET, GMODE, 0) == CLIENT) {
    SetProc(1);
  }
  if (GameParametres(GET, GNET, 0) == TRUE) {
    SetNProc(2);
  }

  /********** --process identifier ***********/
}

void MakeTitle(struct Parametres param, char* title) {
  strncat(title, TITLE, 64 - strlen(title));
  if (param.server == TRUE)
    strcat(title, "(server)  ");
  if (param.client == TRUE)
    strcat(title, "(client)  ");
  strncat(title, version, 64 - strlen(title));
  strcat(title, "  ");
  strncat(title, copyleft, 64 - strlen(title));
}

void InitGameVars(void) {
  int i;
  struct Sockfd sfd;
  int nx, ny;
  struct Player* plyr;

  /****** start server and client *********/

  if (GameParametres(GET, GNET, 0) == TRUE) {
    if (param.server == TRUE) {
      printf("SERVER\n");
      OpenComm(0, param, &sfd);
      printf("opencomm finished\n");
      StartComm(0, &sfd);
    }
    if (param.client == TRUE) {
      printf("CLIENT\n");
      OpenComm(1, param, &sfd);
      printf("opencomm finished\n");
      StartComm(1, &sfd);
    }
    printf("conexion established\n");

    /* synchronization with comm threads  */
    sem_wait(&sem_barrier);
  }
  /****** --start server and client *********/

  /*****************************************************************/
  /* client and server now known all the game options              */
  /*****************************************************************/

  listheadobjs.list = NULL;
  listheadobjs.n = 0;
  listheadplanets.list = NULL;
  listheadplanets.n = 0;
  listheadtext.next = NULL;
  listheadtext.info.n = 0;

  for (i = 0; i < 4; i++)
    fobj[i] = 0;
  gtime0 = 0;

  /********** players ***********/
  /* CreatePlayers(&players); */
  CreatePlayers(clientname, param);
  plyr = GetPlayers();
  /********** --players ***********/

  /********** teams ***********/
  CreateTeams(param);
  /********** --teams **********/

  if (param.queen) {
    printf("WARNING: Queen mode set to ON.\n");
    GameParametres(SET, GQUEEN, TRUE);
  }

  /******** Create Universe **************/
  if (param.client == FALSE) {
    printf("CreateUniverse()...");
    CreateUniverse(GameParametres(GET, GULX, 0), GameParametres(GET, GULY, 0),
                   &listheadobjs, planetnames);
    CreatePlanetList(listheadobjs, &listheadplanets);
    printf("...done\n");
    printf("CreateShips()...");
    CreateShips(&listheadobjs);
    printf("...done\n");
  }
  /******** -- Create Universe **************/

  listheadcontainer = malloc((GameParametres(GET, GNPLANETS, 0) + 1) *
                             sizeof(struct HeadObjList));
  MemUsed(MADD,
          (GameParametres(GET, GNPLANETS, 0) + 1) * sizeof(struct HeadObjList));
  if (listheadcontainer == NULL) {
    fprintf(stderr, "ERROR in malloc main()\n");
    exit(-1);
  }
  for (i = 0; i < GameParametres(GET, GNPLANETS, 0) + 1; i++) {
    listheadcontainer[i].list = NULL;
    listheadcontainer[i].n = 0;
  }

  listheadkplanets = malloc((GameParametres(GET, GNPLAYERS, 0) + 2) *
                            sizeof(struct HeadObjList));
  MemUsed(MADD,
          (GameParametres(GET, GNPLAYERS, 0) + 2) * sizeof(struct HeadObjList));
  if (listheadkplanets == NULL) {
    fprintf(stderr, "ERROR in malloc main()\n");
    exit(-1);
  }
  for (i = 0; i < GameParametres(GET, GNPLAYERS, 0) + 2; i++) {
    listheadkplanets[i].list = NULL;
    listheadkplanets[i].n = 0;
  }

  /********* sending and receiving Universe *****************/

  if (param.server == TRUE) {
    if (ExecSave(listheadobjs, SAVETMPFILE) != 0) {
      fprintf(stderr, "Error in main(): I can't open %s\n", SAVETMPFILE);
      exit(-1);
    }

    SetModifiedAll(&listheadobjs, ALLOBJS, SENDOBJUNMOD, TRUE);
    p_time = GetTime();
  }

  if (GameParametres(GET, GNET, 0) == TRUE) {
    sem_post(&sem_barrier1);
    sem_wait(&sem_barrier);
  }

  if (param.client == TRUE) {
    GetUniverse();
    g_objid = glocal.g_objid;
    plyr = GetPlayers();

    SetModifiedAll(&listheadobjs, ALLOBJS, SENDOBJUNMOD, TRUE);
    CreatePlanetList(listheadobjs, &listheadplanets);
  }
  /********* --sending and receiving Universe *****************/

  actual_player = 1;
  if (GameParametres(GET, GMODE, 0) ==
      SERVER) { /* only two human players, by now*/
    actual_player = 2;
  }
  actual_player0 = actual_player;
  plyr[actual_player].control = HUMAN;

  printf("Actual Player: %d\n", actual_player);

  DestroyObjList(&listheadplayer);
  listheadplayer.n = 0;
  listheadplayer.list = NULL;
  CreatePlayerList(listheadobjs, &listheadplayer, actual_player);
  listheadplayer.update = 0;

  cv = NextCv(&listheadplayer, cv, actual_player);
  if (cv != NULL) {
    habitat.type = cv->habitat;
    habitat.obj = cv->in;
    cv->selected = TRUE;
  }

  /* Adding planets to players list */
  AddPlanets2List(&listheadobjs, plyr);

  /* print teams */
  PrintTeams();

  /*****CELLON*****/

  nx = GameParametres(GET, GULX, 0) / 2000;
  ny = GameParametres(GET, GULY, 0) / 2000;

  cell = malloc(nx * ny * sizeof(int));
  if (cell == NULL) {
    fprintf(stderr, "ERROR in malloc (creating cell)\n");
    exit(-1);
  }
  for (i = 0; i < nx * ny; i++) {
    cell[i] = 0;
  }

  /*****--CELLON*****/
  PrintGameOptions();
}

void AddPlanets2List(struct HeadObjList* listheadobjs, struct Player* players) {
  /*
     Add known planets to players list
  */
  struct ObjList* ls;
  int i;

  if (listheadobjs == NULL || players == NULL) {
    fprintf(stderr, "ERROR in AddPlanets2List()\n");
    exit(-1);
  }

  ls = listheadobjs->list;
  while (ls != NULL) {
    if (ls->obj->type == PLANET) {
      for (i = 0; i < GameParametres(GET, GNPLAYERS, 0) + 1; i++) {
        if (GameParametres(GET, GKPLANETS, 0) == TRUE ||
            players[ls->obj->player].team == players[i].team ||
            (GameParametres(GET, GENEMYKNOWN, 0) == TRUE &&
             players[ls->obj->player].team > 1)) {
          players[i].kplanets = Add2IntList((players[i].kplanets), ls->obj->id);
        }
      }
    }
    ls = ls->next;
  }
}
