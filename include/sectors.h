#ifndef _SECTORS_
#define _SECTORS_

#include "objects.h"
#include "players.h"

int SelectSector(struct HeadIntIList* hlist, Object* obj, int* a, int* b);
int Quadrant(float x, float y);
void InvQuadrant(int n, int* x, int* y);
int NearestSector(struct HeadIntIList* head, float a, float b, int n);
int NearestSector_03(struct HeadIntIList* head, float a, float b, int n);
int NearRandomSector(struct HeadIntIList* head, float a, float b, int n);
int IsInUniverse(float a, float b);

#endif
