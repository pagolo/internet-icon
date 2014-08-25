#include <stdio.h>
#include <errno.h>
#include "socket.h"

char *
mysprintf (const char *format, ...)
{
  char *buf = calloc (1, 6);
  int buflen;
  va_list argptr;

  va_start (argptr, format);
  buflen = vsnprintf (buf, 4, format, argptr);
  free (buf);
  va_end (argptr);
  
  va_start (argptr, format);
  buf = calloc (1, ++buflen);
  vsnprintf (buf, buflen, format, argptr);
  va_end (argptr);
    return buf;
}

int
TestConnection (const char *ip, int port)
{
  int res, valopt;
  struct sockaddr_in addr;
  long arg;
  fd_set myset;
  struct timeval tv;
  socklen_t lon;

  // Create socket 
  int soc = socket (AF_INET, SOCK_STREAM, 0);
  if (soc < 0) {
    fprintf (stderr, "Error creating socket (%d %s)\n", errno,
             strerror (errno));
    return (0);
  }

  // Set non-blocking 
  arg = fcntl (soc, F_GETFL, NULL);
  if (arg < 0) {
    fprintf (stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror (errno));
    //close(soc);
    return (0);
  }
  arg |= O_NONBLOCK;
  if (fcntl (soc, F_SETFL, arg) < 0) {
    fprintf (stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror (errno));
    //close(soc);
    return (0);
  }

  // Trying to connect with timeout 
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = inet_addr (ip);
  res = connect (soc, (struct sockaddr *) &addr, sizeof (addr));

  if (res < 0) {
    if (errno == EINPROGRESS) {
      tv.tv_sec = 4;
      tv.tv_usec = 0;
      FD_ZERO (&myset);
      FD_SET (soc, &myset);
      if (select (soc + 1, NULL, &myset, NULL, &tv) > 0) {
        lon = sizeof (int);
        getsockopt (soc, SOL_SOCKET, SO_ERROR, (void *) (&valopt), &lon);
        if (valopt) {
          fprintf (stderr, "Error in connection() %d - %s\n", valopt,
                   strerror (valopt));
          close (soc);
          return (0);
        }
      }
      else {
        fprintf (stderr, "Timeout or error() %d - %s\n", valopt,
                 strerror (valopt));
        close (soc);
        return (0);
      }
    }
    else {
      fprintf (stderr, "Error connecting %d - %s\n", errno, strerror (errno));
      close (soc);
      return (0);
    }
  }

  close (soc);
  return 1;
}

/*
 * this is straight from beej's network tutorial. It is a nice wrapper
 * for inet_ntop and helpes to make the program IPv6 ready
 */
char *
get_ip_str (const struct sockaddr *sa, char *s, size_t maxlen)
{
  switch (sa->sa_family) {
  case AF_INET:
    inet_ntop (AF_INET, &(((struct sockaddr_in *) sa)->sin_addr), s, maxlen);
    break;

  case AF_INET6:
    inet_ntop (AF_INET6, &(((struct sockaddr_in6 *) sa)->sin6_addr),
               s, maxlen);
    break;

  default:
    strncpy (s, "Unknown AF", maxlen);
    return NULL;
  }

  return s;
}

AllIp *
GetLanIp (void)
{
  AllIp *MyData = NULL;
  IpList *If = NULL, *IfStart = NULL, *IfPtr;
  char buf[1024] = { 0 };
  struct ifconf ifc = { 0 };
  struct ifreq *ifr = NULL;
  int nInterfaces = 0;
  int i;

  // controllare che sia ok
  MyData = calloc (1, sizeof (AllIp));
  if (!MyData)
    return NULL;

  // Create socket 
  int soc = MyData->socket = socket (AF_INET, SOCK_STREAM, 0);
  if (soc < 0) {
    fprintf (stderr, "Error creating socket (%d %s)\n", errno,
             strerror (errno));
    free (MyData);
    return (NULL);
  }

  MyData->WanIp = strdup("Wait please...\t<i>(<b>WAN/Public</b>)</i>");

  /* Query available interfaces. */
  ifc.ifc_len = sizeof (buf);
  ifc.ifc_buf = buf;
  if (ioctl (soc, SIOCGIFCONF, &ifc) < 0) {
    perror ("ioctl(SIOCGIFCONF)");
    return NULL;
  }

  /* Iterate through the list of interfaces. */
  ifr = ifc.ifc_req;
  nInterfaces = ifc.ifc_len / sizeof (struct ifreq);
  for (i = 0; i < nInterfaces; i++) {
    struct ifreq *item = &ifr[i];
    char ip[IPLEN];
    if (strcmp (item->ifr_name, "lo") == 0)
      continue;
    If = calloc (1, sizeof (*If));
    If->IpString = mysprintf ("%s\t<i>(device <b>%s</b>)</i>",
                              get_ip_str ((void *) &(item->ifr_addr), ip,
                                          IPLEN), item->ifr_name);
    if (IfStart == NULL)
      IfStart = If;
    else {
      for (IfPtr = IfStart; IfPtr->Next != NULL; IfPtr = IfPtr->Next);
      IfPtr->Next = If;
    }
  }
  MyData->LanIpList = IfStart;

  return MyData;
}

AllIp *
GetWanIp (AllIp *MyData)
{
  struct hostent *Host = { 0 };
  struct sockaddr_in INetSocketAddr = { 0 };
  int res, i;
  const char request[] =
    "GET /ip\r\nHTTP/1.0\r\nUser-Agent: InternetIcon/0.1\r\n\r\n";

  // free previous string
  if (MyData->WanIp) free(MyData->WanIp);
  MyData->WanIp = NULL;
  
  // get socket 
  int soc = MyData->socket;

  Host = (struct hostent *) gethostbyname ("ifconfig.me");
    if (Host == NULL) {
    fprintf (stderr, "Error getting host address (%d %s)\n", errno,
             strerror (errno));
    close (soc);
    MyData->WanIp = strdup("<i>Can't find wan ip.</i>");
    return MyData;
  }

  // Trying to connect
  res = -1;
  for (i = 0; Host->h_addr_list[i]; i++) {
    memcpy ((char *) &INetSocketAddr.sin_addr, Host->h_addr_list[i],
            Host->h_length);
    INetSocketAddr.sin_family = Host->h_addrtype;
    INetSocketAddr.sin_port = htons (80);
    res =
      connect (soc, (struct sockaddr *) &INetSocketAddr,
               sizeof (INetSocketAddr));
    if (res == 0) {
      break;
    }
  }
  if (res < 0) {
    fprintf (stderr, "Can't connect to site\n");
    close (soc);
    MyData->WanIp = strdup("Can't retrive wan ip.");
    return MyData;
  }

  if (write (soc, request, strlen (request)) < 1) {
    fprintf (stderr, "request ip failed\n");
    close (soc);
    MyData->WanIp = strdup("<i>Can't find wan ip.</i>");
    return MyData;
  }

  {
    char buffer[IPLEN] = {0};
    int nchars = read (soc, buffer, IPLEN);
    if (nchars < 1) {
      fprintf (stderr, "Can't retrive wan ip\n");
      MyData->WanIp = strdup("Can't retrive wan ip.");
      return MyData;
    }
    {
      char *z = strchr (buffer, '\r');
      if (!z)
        z = strchr (buffer, '\n');
      if (z)
        *z = '\0';
    }
    MyData->WanIp = mysprintf ("%s\t<i>(<b>WAN/Public</b>)</i>", buffer);
  }
  
  close (soc);
  return MyData;
}

void FreeAllIp (AllIp *MyData)
{
}
