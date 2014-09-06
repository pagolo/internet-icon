#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

char * mysprintf (const char *format, ...);
int is_local_address(unsigned char *y);

#endif