
#include <gtk/gtk.h>
#include <libnotify/notify.h>

enum {
  _DIALOG, _MYDATA
};

enum {
  _NOTIFY, _ICON, _AUTO, _ENDFLAGS
};

typedef struct {
  int   timeout_seconds;
  char *test_ip;
  int   test_port;
  int   op_mode;
  char *wanip_page;
  char *user_agent;
} Config;

typedef struct {
  GtkStatusIcon *icon;
  NotifyNotification *notify;
} Exchange;
