 /*****************************************************************************
 **  This is part of the SpaceZero program
 **  Copyright (C) 2006-2013  MRevenga
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License (version 3), or
 **  (at your option) any later version, as published by the Free Software 
 **  Foundation.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program; if not, write to the Free Software
 **  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ******************************************************************************/

/*************  SpaceZero  M.R.H. 2006-2013 ******************
		Author: MRevenga
		E-mail: mrevenga at users.sourceforge.net
		version 0.86 December 2013
****/

#ifndef _SECTORS_
#define _SECTORS_

#include "objects.h" 
#include "players.h"

int SelectSector(struct HeadIntIList *hlist,Object *obj,int *a,int *b);
int Quadrant(float x, float y);
void InvQuadrant(int n,int *x, int *y);
int NearestSector(struct HeadIntIList *head,float a,float b,int n);
int NearestSector_03(struct HeadIntIList *head,float a,float b,int n);
int NearRandomSector(struct HeadIntIList *head,float a,float b,int n);
int IsInUniverse(float a,float b);

#endif
