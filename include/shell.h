#ifndef _SHELL_
#define _SHELL_

#include <gtk/gtk.h>
#include "menu.h"
#include "objects.h"

/* keystrokes */
#define RESET 0
#define ADD 1
#define DELETELAST 2
#define RETURNLAST 3
#define LOAD 4

void initshell(void);
void ShellTitle(int order,
                char* mess,
                GdkPixmap* pixmap,
                GdkGC* color,
                GdkFont* font,
                int x,
                int y);
int Shell(int command,
          GdkPixmap* pixmap,
          GdkGC* color,
          GdkFont*,
          struct HeadObjList* hl,
          struct Player* player,
          struct Keys*,
          Object** cv,
          char* cad);
Object* ExecOrder(struct HeadObjList* hl,
                  Object* obj,
                  struct Player* ps,
                  int order,
                  char* par);
void SelectionBox(GdkPixmap* pixmap, GdkGC* color, Object**, int);
int Keystrokes(int mode, guint* val, char* c);
char Keyval2Char(guint keyval);

int Get2Args(char* cad, char* arg1, char* arg2);

#endif
