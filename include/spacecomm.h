#ifndef _SPACECOMM_
#define _SPACECOMM_

#include <sys/types.h>
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "comm.h"
#include "general.h"
#include "help.h"
#include "objects.h"

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 50100

#define NETSTEP 3

/* types of orders among server and clients
   Orders to Threads
*/

#define OTSENDPING 1
#define OTSENDOBJS 2 /* send modified objects */
#define OTSENDSAVE 3 /* sendallobjects */
#define OTSENDLOAD 4 /* load game */
#define OTSENDKILL 5 /* kill client */
#define OTSENDEND 6  /* no more messages */

/*
   header messages ids
*/

/*#define SENDOBJ MSENDOBJS */ /* send modified objects */
#define SENDGLOBAL 17
#define SENDPLAYER 18
#define SENDPLAYERMOD 19
#define SENDPLANET 20
#define SENDSTR 21
#define SENDPLANETLIST 22
#define SENDSECTORLIST 23
#define SENDPAUSE 24
#define SENDMESS 25
#define SENDOK 26
#define SENDEND 27

/* object modify types, used in transmission buffer */

#define SENDOBJUNMOD 33   /* obj unmodified, don't send */
#define SENDOBJMOD0 34    /* obj modified, send only position */
#define SENDOBJMOD 35     /* obj modified, send more information */
#define SENDOBJAALL 36    /* obj modified, send almost all info */
#define SENDOBJALL 37     /* obj modified, send all info */
#define SENDOBJKILL 38    /* delete the object  */
#define SENDOBJNEW 39     /* new obj */
#define SENDOBJDEAD 40    /* don't send and remove */
#define SENDOBJPLANET 41  /* send all info planet */
#define SENDOBJSTR 42     /* send a text message */
#define SENDOBJSEND 43    /* object just sended */
#define SENDOBJNOTSEND 44 /* don't send */

#define NMPLANETDISCOVERED 1
#define NMPLANETLOST 2

#define NMADD 1
#define NMREAD 2
#define NMCLEAN 3
#define NMCOUNT 4
#define NMPRINT 5

struct Thread_arg {
  int sfd;
  int sfd2;
  int id;
  int n;
};

struct Sockfd {
  int sfd, sfd2;
  int nsfd, nsfd2;
};

struct NetMess {
  int id;
  int a, b;
};

struct ListNetMess {
  struct NetMess mess;
  struct ListNetMess* next;
};

/* Declaracion de funciones */

int OpenComm(int mode, struct Parametres param, struct Sockfd* sfd);
int StartComm(int mode, struct Sockfd* sockfd);
void* CommClient(void*);
void* CommServer(void*);

/* int CopyObjs2Buffer(char *buf,struct HeadObjList lh); */
/* int CopyObj2Buffer(struct Buffer *buffer,void *object,int type); */
int ReadObjsfromBuffer(char* buf);
int CopyGlobal2Buffer_00(char* buf);

int AddPlayerOrders2Buffer(int player, char*);
int CopyPlayerOrdersfromBuffer(int player, char*);

int AddObjOrders2Buffer(struct Buffer* buffer, Object* obj);
int CopyObjOrdersfromBuffer(Object* obj0, char* buf);

void PrintTextMessage(struct Message* mess);
void SendTextMessage(char* mess);
int PendingTextMessage(void);
void GetTextMessage(char* mess);

int NetMess(struct NetMess*, int action);
int CopyNetMess2Buffer(struct Buffer* buffer, struct NetMess* mess0);

int SetModified(Object* obj, int mode);
int SetModifiedAll(struct HeadObjList* lh, int type, int mode, int force);
int CheckModifiedPre(struct HeadObjList* lh, int proc);
int CheckModifiedPost(struct HeadObjList* lh, int proc);
void Setttl0(struct HeadObjList* lh);
void Setttl(struct HeadObjList* lh, int ttl);

void LoadBuffer(int order, struct Buffer* buf, int mode);
int CopyObjs2Buffer(struct Buffer* buffer, struct HeadObjList hl);
int CopyObj2Buffer(struct Buffer* buffer, void* object, int modtype);
int CopyMessHeader2Buffer(struct Buffer* buffer, struct MessageHeader* messh);
int CopyGlobal2Buffer(struct Buffer* buf);
int CopyPlayer2Buffer(struct Buffer* buffer, struct Player* player);
int CopyOrder2Buffer(struct Buffer* buffer, struct Order* order);
int CopyInt2Buffer(struct Buffer* buffer, int* i);

int ServerProcessBuffer(struct Buffer* buffer);
int ClientProcessBuffer(struct Buffer* buffer);
int CopyPlayerMod2Buffer(struct Buffer* buffer, struct Player* player);
int AddObjCargo2Buffer(struct Buffer* buffer, Object* obj);

int CopyObjCargofromBuffer(Object* obj0, char* buf0);

#endif
