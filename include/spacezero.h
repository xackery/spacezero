#ifndef _SPACE_
#define _SPACE_

#include <gtk/gtk.h>
#include <stdio.h>
#include "objects.h"
#include "players.h"

gint MainLoop(gpointer data);
gint MenuLoop(gpointer data);
gint Quit(GtkWidget* widget, gpointer gdata);
void PrintGameOptions(void);
int UpdateObjs(void);
void UpdateShip(Object*);
void key_eval(void);
void Collision(struct HeadObjList*);
float PlanetAtraction(float* fx, float* fy, float x, float y, float m);
void CreateUniverse(int, int, struct HeadObjList*, char**);
void InitGameVars(void);
int PrintfObjInfo(FILE* fp, Object* obj);
Object* ChooseInitPlanet(struct HeadObjList lheadobjs);
void CreateShips(struct HeadObjList* lheadobjs);
void AddPlanets2List(struct HeadObjList* listheadobjs, struct Player* players);
int CheckGame(char*, int);
void GetGold(void);
void GetPoints(struct HeadObjList* hl, int proc, struct Player* p);
void GetPointsObj(struct HeadObjList* lhobjs, struct Player* p, Object* obj);
void Density(void);
void GetUniverse(void);
void NetComm(void);
void SetGameParametres(struct Parametres param);
void MakeTitle(struct Parametres param, char* title);
void CreateTeams(struct Parametres param);
void SaveRecord(char* file, struct Player* players, int record);

#endif
