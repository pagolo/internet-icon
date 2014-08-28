#ifndef __MYLOCALE_H__
#define __MYLOCALE_H__

#define getstr(a,b) _(b)
#define _(string) gettext(string)
#define LOCALEDIR "/usr/lib/locale"   // NO final slash

#define	LIMAGIC(category) \
	  (category == LC_COLLATE						\
	   ? ((unsigned int) (0x20051014 ^ (category)))				\
	   : category == LC_CTYPE						\
	   ? ((unsigned int) (0x20090720 ^ (category)))				\
	   : ((unsigned int) (0x20031115 ^ (category))))
	
typedef struct {
  char *locale_code;
  char *language_name;
  struct localelist *next;
} localelist;

extern localelist *get_locales_all(void);

#endif
