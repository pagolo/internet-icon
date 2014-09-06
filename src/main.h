
#include <gtk/gtk.h>
#include <libnotify/notify.h>

typedef enum {
  _DIALOG, _MYDATA
} Transmit;

typedef enum {
  _NOTIFY, _ICON, _AUTO, _ENDFLAGS
} OpMode;

typedef struct {
  const char *desktop;
  OpMode      op_mode;
} MyDesktop;

typedef struct {
  int   timeout_seconds;
  char *test_ip;
  int   test_port;
  OpMode op_mode;
  char *wanip_page;
  char *user_agent;
} Config;

typedef union {
  GtkStatusIcon *icon;
  NotifyNotification *notify;
} Exchange;
