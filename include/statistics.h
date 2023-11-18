#ifndef _STATISTICS_
#define _STATISTICS_

#include <gdk/gdkx.h>
#include "general.h"

#define NSTATS 64

struct Stats {
  int time;
  float level[MAXNUMPLAYERS + 2];
  int nplanets[MAXNUMPLAYERS + 2];
};

void InitStatistics(void);

void UpdateStatistics(void);

void fprintStatistics(FILE* fp);
void fscanfStatistics(FILE* fp);

void PrintStatistics(void);

void Statistics_Draw(GdkPixmap* pixmap, Rectangle* r);

#endif
