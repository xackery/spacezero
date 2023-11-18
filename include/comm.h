#ifndef _COMM_
#define _COMM_

#include <stdint.h>
/* #define BUFFERSIZE 128*4096 */
#define BUFFERSIZE 4096

struct MessageHeader {
  uint32_t id;     /* identification of the order */
  uint32_t nobjs;  /* number of objects to send */
  uint32_t nbytes; /* number of bytes to send */
};

struct Message {
  struct MessageHeader header; /* header of the message */
  char* buf;                   /* buffer of data */
};

struct Buffer {
  char* data;
  uint32_t n;    /* position of read write  */
  uint32_t size; /* size of the buffer */
};

int send_buf(int sfd, char* buf, int nbytes);
int recv_buf(int sfd, char* buf);
int send_message(int sfd, struct Message* mess);
int recv_message(int sfd, struct Message* mess);

int SendFile(int fd, int sfd);
int RecvFile(int fd, int sfd);
int CopyFile2Buf(int fd, struct Buffer* buf);
int CopyBuf2File(char* buf, int fd, int n);

int SendBuffer(int, struct Buffer* buffer);
int RecvBuffer(int, struct Buffer* buffer);

#endif
