
enum
{
  STATUS_NO_MESSAGE,
  STATUS_MESSAGE_FIRST,
  STATUS_MESSAGE
};

enum {
  _DIALOG, _MYDATA
};

typedef struct {
  int   timeout_seconds;
  char *test_ip;
  int   test_port;
  char *wanip_page;
  char *user_agent;
} Config;
