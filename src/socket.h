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
#define closesocket(d)    if(d>=0) close(d)
  
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
} AllIp;

int test_connection (const char *ip, int port);
AllIp *get_lan_ip (void);
AllIp *get_wan_ip (AllIp *MyData);
void free_all_ip (AllIp *MyData);


#endif  /*  */
