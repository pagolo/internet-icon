
#include "utils.h"

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

int is_local_address(unsigned char *y)
{
    // 10.x.y.z 127.x.y.z
    if (y[0] == 10 || y[0] == 127)
        return 1;

    // 172.16.0.0 - 172.31.255.255
    if ((y[0] == 172) && (y[1] >= 16) && (y[1] <= 31))
        return 1;

    // 192.168.0.0 - 192.168.255.255
    if ((y[0] == 192) && (y[1] == 168))
        return 1;

    return 0;
}
