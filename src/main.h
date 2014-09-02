
enum
{
  STATUS_NO_MESSAGE,
  STATUS_MESSAGE_FIRST,
  STATUS_MESSAGE
};

enum {
  _DIALOG, _MYDATA
};

enum {
  _DISABLE, _ENABLE, _AUTO, _END
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
