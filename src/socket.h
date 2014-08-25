#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#ifndef SOCKET
#define SOCKET int
#endif  /*  */
#define closesocket    close
  
#define NO_SOCKET -1
#define NO_FILE NO_SOCKET
#define DATBUF_SIZE 512
#define IPLEN INET6_ADDRSTRLEN + 1

typedef struct _IpList
{
  char *IpString;
  struct _IpList *Next;
} IpList;

typedef struct _AllIp
{
  char *WanIp;
  IpList *LanIpList;
  SOCKET *socket;
} AllIp;

int TestConnection (const char *ip, int port);
AllIp *GetLanIp (void);
AllIp *GetWanIp (AllIp *MyData);
void FreeAllIp (AllIp *MyData);


#endif  /*  */
