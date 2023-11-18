#ifndef _SAVE_
#define _SAVE_

#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ai.h"
#include "functions.h"
#include "general.h"
#include "help.h"
#include "objects.h"
#include "spacecomm.h"
#include "statistics.h"

struct ObjTable {
  int id, parent, dest, in, planet;
};

struct Global {
  int actual_player;
  int g_objid, g_projid;
  int ship_c, cv;
  int habitat_type;
  int habitat_id;
  int fobj[4];
};

char* CreateOptionsFile(void);
char* CreateSaveFile(int server, int client);
char* CreateRecordFile(void);
char* CreateKeyboardFile(void);

int CreateDir(char* dir);
void SaveRecord(char* file, struct Player* players, int record);

int FprintfObj(FILE* fp, Object* obj);
int FscanfObj(FILE* fp, Object* obj, struct ObjTable*);

int FprintfPlanet(FILE* fp, Object* obj);
int FscanfPlanet(FILE* fp, struct Planet* planet);

int FprintfOrders(FILE* fp, Object* obj);
int FscanfOrders(FILE* fp, Object* obj);

void FprintfCCData(FILE* fp, struct CCDATA* ccdata);
void FscanfCCData(FILE* fp, struct CCDATA* ccdata);

void FprintfPlanetInfo(FILE* fp, struct PlanetInfo* pinfo);
void FprintfPlanetInfoList(FILE* fp, struct CCDATA* ccdata);
void FscanfPlanetInfoList(FILE* fp, struct CCDATA* ccdata);
void FscanfPlanetInfo(FILE* fp, struct PlanetInfo* pinfo);

void SaveParamOptions(char* file, struct Parametres* par);

int LoadParamOptions(char* file, struct Parametres* par);
void PrintParamOptions(struct Parametres* par);

int ExecSave(struct HeadObjList, char*);
int ExecLoad(char*);

#endif
