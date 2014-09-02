#include <stdio.h>
#include <errno.h>
#include "utils.h"
#include "socket.h"
#include "main.h"
#include "mylocale.h"

extern Config cfg;

SOCKET
create_socket(long *arg, SOCKET old)
{
  SOCKET new;
  
  if (old >= 0) {
    closesocket(old);
  }
  new = socket (AF_INET, SOCK_STREAM, 0);
  if (new < 0) return new;
  if (arg) {
    // Set non-blocking 
    *arg = fcntl (new, F_GETFL, NULL);
    if (*arg < 0) {
      closesocket(new);
      return (-1);
    }
    if (fcntl (new, F_SETFL, *arg | O_NONBLOCK) < 0) {
      closesocket(new);
      return (-1);
    }
  }
  return new;
}
int
test_connection (in_addr_t ip, int port)
{
  int res, valopt;
  struct sockaddr_in addr;
  long arg;
  fd_set myset;
  struct timeval tv;
  socklen_t lon;
  int soc = -1;

  // Create socket 
  soc = create_socket (&arg, soc);
  if (soc < 0) {
    fprintf (stderr, _("Error creating socket\n"));
    return (0);
  }


  // Trying to connect with timeout 
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = ip;
  res = connect (soc, (struct sockaddr *) &addr, sizeof (addr));

  if (res < 0) {
    if (errno == EINPROGRESS) {
      tv.tv_sec = cfg.timeout_seconds - 1;
      tv.tv_usec = 0;
      FD_ZERO (&myset);
      FD_SET (soc, &myset);
      if (select (soc + 1, NULL, &myset, NULL, &tv) > 0) {
        lon = sizeof (int);
        getsockopt (soc, SOL_SOCKET, SO_ERROR, (void *) (&valopt), &lon);
        if (valopt) {
          fprintf (stderr, _("Error in connection %d - %s\n"), valopt,
                   strerror (valopt));
          closesocket (soc);
          return (0);
        }
      }
      else {
        fprintf (stderr, _("Timeout reached.\n"));
        closesocket (soc);
        return (0);
      }
    }
    else {
      fprintf (stderr, _("Error calling connect() %d - %s\n"), errno, strerror (errno));
      closesocket (soc);
      return (0);
    }
  }

  closesocket (soc);
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
get_lan_ip (void)
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
  int soc = create_socket (NULL, -1);
  if (soc < 0) {
    fprintf (stderr, "Error creating socket\n");
    free(MyData);
    return (0);
  }

  MyData->WanIp = mysprintf ("<i>%s</i>", _("Waiting for wan ip..."));

  /* Query available interfaces. */
  ifc.ifc_len = sizeof (buf);
  ifc.ifc_buf = buf;
  if (ioctl (soc, SIOCGIFCONF, &ifc) < 0) {
    fprintf (stderr, "Query interfaces error\n");
    closesocket(soc);
    free(MyData);
    return NULL;
  }

  closesocket(soc);
  
  /* Iterate through the list of interfaces. */
  ifr = ifc.ifc_req;
  nInterfaces = ifc.ifc_len / sizeof (struct ifreq);
  for (i = 0; i < nInterfaces; i++) {
    struct ifreq *item = &ifr[i];
    char ip[IPLEN];
    if (strcmp (item->ifr_name, "lo") == 0)
      continue;
    If = calloc (1, sizeof (*If));
    If->IpString = mysprintf ("%s\t<i>(%s <b>%s</b>)</i>",
                              get_ip_str ((void *) &(item->ifr_addr), ip, IPLEN), 
                              _("device"),
                              item->ifr_name);
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

int get_domain_and_page (char **domain, char **page) {
  char *work = strdup(cfg.wanip_page);
  char *ptr = work, *z;
  if (!work) return 0;
  while (*ptr == ' ') ++ptr;
  z = strchr (ptr, '/');
  if (z) *z = '\0';
  *domain = strdup(ptr);
  if (z) *z = '/';
  else z = "/";
  *page = strdup(z);
  free (work);
  return 1;
}

AllIp *
get_wan_ip (AllIp *MyData)
{
  long arg;
  fd_set myset;
  struct timeval tv;
  socklen_t lon;
  struct hostent *Host = { 0 };
  struct sockaddr_in INetSocketAddr = { 0 };
  int res, i, valopt;
  char *domain, *page, *request;
  const char request_format[] =
    "GET %s\r\nHTTP/1.0\r\nUser-Agent: %s\r\n\r\n";

  // free previous string
  if (MyData->WanIp) free(MyData->WanIp);
  MyData->WanIp = NULL;

  if (!(get_domain_and_page (&domain, &page))) {
    fprintf (stderr, _("Error hadling strings (%d %s)\n"), errno,
             strerror (errno));
    MyData->WanIp = strdup("<i>Memory problems.</i>");
    return MyData;
  }
  
  Host = (struct hostent *) gethostbyname (domain);
  if (Host == NULL) {
    fprintf (stderr, _("Error getting host address (%d %s)\n"), errno,
             strerror (errno));
    MyData->WanIp = mysprintf("<i>%s</i>", _("Error getting host address."));
    return MyData;
  }
  free(domain);

  char *err_soc = _("Error creating socket");
  int soc = create_socket (&arg, -1);
  if (soc < 0) {
    fprintf (stderr, "%s (%d %s)\n", err_soc, errno,
             strerror (errno));
    MyData->WanIp = mysprintf("<i>%s.</i>", err_soc);
    return MyData;
  }

  // Trying to connect
  res = -1;
  MyData->WanIp = strdup(_("Can't retrive wan ip."));
  
  for (i = 0; Host->h_addr_list[i]; i++) {
    //printf("loop counter = %d\n", i);
    memcpy ((char *) &INetSocketAddr.sin_addr, Host->h_addr_list[i],
            Host->h_length);
    INetSocketAddr.sin_family = Host->h_addrtype;
    INetSocketAddr.sin_port = htons (80);
    res =
      connect (soc, (struct sockaddr *) &INetSocketAddr,
               sizeof (INetSocketAddr));
    if (res < 0) {
      if (errno == EINPROGRESS) {
        tv.tv_sec = 6;
        tv.tv_usec = 0;
        FD_ZERO (&myset);
        FD_SET (soc, &myset);
        if (select (soc + 1, NULL, &myset, NULL, &tv) > 0) {
          lon = sizeof (int);
          getsockopt (soc, SOL_SOCKET, SO_ERROR, (void *) (&valopt), &lon);
          if (valopt) {
            fprintf (stderr, "%s %d - %s\n", _("Error in connection()"), valopt,
                     strerror (valopt));
            soc = create_socket (&arg, soc);
            if (soc < 0) {
              fprintf (stderr, "%s (%d %s)\n", err_soc, errno,
                       strerror (errno));
              MyData->WanIp = mysprintf("<i>%s.</i>", err_soc);
              return MyData;
            }
            continue;
          }
          // no error... exit loop...
          break;
        }
        else {
          // SOLARIS: ioctl(connection, I_SETSIG, S_HANGUP);
          shutdown(soc, SHUT_RDWR);
          soc = create_socket (&arg, soc);
          if (soc < 0) {
            fprintf (stderr, "%s (%d %s)\n", err_soc, errno,
                       strerror (errno));
            MyData->WanIp = mysprintf("<i>%s.</i>", err_soc);
            return MyData;
          }
          continue;
        }
      }
      else {
        fprintf (stderr, "%s %d - %s\n", _("Error in connection"), errno, strerror (errno));
        closesocket (soc);
        return (MyData);
      }
    }
  }

  if (fcntl (soc, F_SETFL, arg) < 0) {
    fprintf (stderr, "%s (%d %s)\n",
             _("Error setting socket flags"),
             errno,
             strerror (errno));
    MyData->WanIp = mysprintf("<i>%s.</i>", _("Error setting socket flags"));
    return MyData;
  }

  request = mysprintf (request_format, page, cfg.user_agent);
  free (page);
  if (request == NULL) {
    fprintf (stderr, "%s (%d %s)\n",
             _("Error handling strings"),
             errno,
             strerror (errno));
    closesocket (soc);
    MyData->WanIp = mysprintf ("<i>%s.</i>", _("Insufficient memory"));
    return MyData;
  }
    
  if (write (soc, request, strlen (request)) < 1) {
    fprintf (stderr, _("request ip failed\n"));
    closesocket (soc);
    MyData->WanIp = mysprintf("<i>%s.</i>", _("Can't find wan ip"));
    return MyData;
  }

  {
    char buffer[IPLEN] = {0};
    int nchars = read (soc, buffer, IPLEN);
    if (nchars < 1) {
      fprintf (stderr, "%s\n", _("Can't retrive wan ip"));
      MyData->WanIp = mysprintf("%s.", _("Can't retrive wan ip"));
      return MyData;
    }
    {
      char *z = strchr (buffer, '\r');
      if (!z)
        z = strchr (buffer, '\n');
      if (z)
        *z = '\0';
    }
    MyData->WanIp = mysprintf ("%s\t<i>(<b>%s</b>)</i>", buffer, _("WAN/Public"));
  }
  
  closesocket (soc);
  return MyData;
}

void free_all_ip (AllIp *MyData)
{
  IpList *list_item, *next_item;
  if (!MyData) return;
  if (MyData->WanIp) free (MyData->WanIp);
  for (list_item = MyData->LanIpList; list_item; list_item = next_item) {
    next_item = list_item->Next;
    if (list_item->IpString) free (list_item->IpString);
    free (list_item);
  }
  free (MyData);
}
