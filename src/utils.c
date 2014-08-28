
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
