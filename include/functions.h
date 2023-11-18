#ifndef _FUNCTIONS_
#define _FUNCTIONS_

#include "players.h"

#define GET 0
#define SET 1

#define MRESET 0
#define MSET 1
#define MADD 2
#define MGET 3

float Random(int);
float Sqrt(int n);
void delay(int time);
int GetControl(struct Player*, int player);
int GameParametres(int option, int param, int value);

int Proc(int option, int value);
int GetProc(void);
int SetProc(int value);
int NProc(int option, int value);
int GetNProc(void);
int SetNProc(int value);

int sTime(int action, int t);
int GetTime(void);
void SetTime(int t);
void IncTime(void);

void DelCharFromCad(char* cad, char* filter);

int MemUsed(int action, size_t value);

#endif
