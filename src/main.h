
#include <gtk/gtk.h>
#include <libnotify/notify.h>

enum {
  _DIALOG, _MYDATA
};

enum {
  _DISABLE, _ENABLE, _AUTO, _ENDFLAGS
};

typedef struct {
  int   timeout_seconds;
  char *test_ip;
  int   test_port;
  int   status_icon;
  int   notification;
  char *wanip_page;
  char *user_agent;
} Config;

typedef struct {
  GtkStatusIcon *icon;
  NotifyNotification *notify;
} Exchange;
