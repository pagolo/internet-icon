#ifndef __MYLOCALE_H__
#define __MYLOCALE_H__

#include <locale.h>
#include <libintl.h>

#define _(string) gettext(string)

#endif
