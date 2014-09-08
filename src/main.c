/*
 * main.c
 * Copyright (C) 2014 Paolo Bozzo <paolo@linux-l6dg>
 * 
 */

#include <stdio.h>
#include <string.h>
#include <config.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <unistd.h>
#include <libnotify/notify.h>
#include "socket.h"
#include "findtask.h"
#include "xml.h"
#include "utils.h"
#include "main.h"
#include "mylocale.h"

// allocate config and set defaults
Config cfg = { 15, "auto", 53, _AUTO, "ifconfig.me/ip", "InternetIcon/Getting wan ip"};

// internet icons
extern const GdkPixdata my_pixbuf_ok;
extern const GdkPixdata my_pixbuf_ko;

static void
tray_icon_on_menu (GtkStatusIcon * status_icon, guint button,
                   guint activate_time, gpointer user_data)
{
  gtk_menu_popup (GTK_MENU (user_data), NULL, NULL,
                  gtk_status_icon_position_menu, status_icon, button,
                  activate_time);
}

static gboolean
check_internet (void)
{
  int i;
  gboolean tested = FALSE;
  
  if (strcasecmp(cfg.test_ip, "auto") != 0) {
    return (gboolean) test_connection (inet_addr (cfg.test_ip), cfg.test_port);
  }
	for (i = 0; i < MAXNS; i++) {
    unsigned char *ip = (unsigned char *)&(_res.nsaddr_list[i].sin_addr.s_addr);
    if (is_local_address (ip) || *ip == 0)
      continue;
    if (test_connection (_res.nsaddr_list[i].sin_addr.s_addr, cfg.test_port))
      return TRUE;
    tested = TRUE;
  }

  if (!(tested))
    return (gboolean) test_connection (inet_addr ("8.8.8.8"), cfg.test_port);
  
  return FALSE;
}

static void
tray_exit (GtkMenuItem * item, gpointer user_data)
{
  gtk_main_quit ();
}

static void
tray_about (GtkMenuItem * item, gpointer user_data)
{
  gboolean internet_on = TRUE;
  if (user_data) {
      internet_on =  *((gboolean *)user_data);
  }
  GdkPixbuf *pixbuf = gdk_pixbuf_from_pixdata (internet_on? &my_pixbuf_ok : &my_pixbuf_ko, TRUE, NULL);
  if (!(pixbuf)) {
    fprintf (stderr, _("Get pixbuf error...\n"));
    return;
  }
  GdkPixbuf *pixbuf2 = pixbuf;
  if (internet_on == FALSE) {
    pixbuf2 = gdk_pixbuf_from_pixdata (&my_pixbuf_ok, TRUE, NULL);
  }
  GtkWidget *dialog = gtk_about_dialog_new ();
  if (!(dialog)) {
    fprintf (stderr, _("Can't create dialog...\n"));
    return;
  }
  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Internet Icon");
  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog), VERSION);
  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dialog),
                                  "(c) Paolo Bozzo");
  gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (dialog),
                                 _("InternetIcon is a simple tool that shows your internet status."));
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), 
                               "https://github.com/pagolo/internet-icon");
  gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (dialog), pixbuf2);
  gtk_window_set_icon (GTK_WINDOW(dialog), pixbuf);
  if (pixbuf2 != pixbuf) {
    g_object_unref (pixbuf2);
  }
  g_object_unref (pixbuf), pixbuf = pixbuf2 = NULL;
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void notify_about (NotifyNotification *notification, char *action, gpointer user_data)
{
  tray_about (NULL, user_data);
  notify_notification_show(notification, NULL);
}

static gchar *build_dialog_string (AllIp *MyData)
{
  IpList *il;
  gchar **array, *result;
  gint count = 2, i;

  for (il = MyData->LanIpList; il; il = il->Next)
    ++count;
  array = calloc (count, sizeof (gchar *));
  count = 0;
  array[0] = strdup (MyData->WanIp);
  for (il = MyData->LanIpList; il; il = il->Next)
    array[++count] = strdup (il->IpString);
  result = g_strjoinv ("\n", array);
  for (i = 0; array[i]; i++)
    free (array[i]);
  free (array);
  return result;
}
                                  
static gboolean
set_dialog_content (gpointer data)
{
  void **transmit = data;
  GtkWidget *dialog = (GtkWidget *) transmit[_DIALOG];
  AllIp *MyData = (AllIp *) transmit[_MYDATA];
  gchar *content;

  if (MyData) {
    get_wan_ip (MyData);
    content = build_dialog_string (MyData);
    if (GTK_IS_MESSAGE_DIALOG (dialog) && content) {
      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(dialog), content);
      GtkWidget *button = gtk_dialog_add_button (GTK_DIALOG(dialog), _("Close"), 0);
      if (button) {
        g_signal_connect_swapped (button, "activate", G_CALLBACK(gtk_widget_destroy), dialog);
        gtk_widget_grab_focus (GTK_WIDGET (button));
      }
    }
    if (content) free(content);
    free_all_ip (MyData);
  }
  // returns stopping timeout
  return FALSE;
}

static void
show_info (GtkWidget * widget, gpointer window)
{
  GtkWidget *dialog;
  AllIp *MyData;
  char *content;
  void *transmit[2];
  GdkPixbuf *pixbuf = gdk_pixbuf_from_pixdata (&my_pixbuf_ok, TRUE, NULL);

  if (!(pixbuf)) {
    fprintf (stderr, _("Get pixbuf error...\n"));
    return;
  }

  MyData = get_lan_ip ();
  if (MyData == NULL)
    return;
  
  content = build_dialog_string (MyData);
  
  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
                                               0,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_NONE,
                                               content);
  if (content)
    free(content);
  if (dialog == NULL) {
    free_all_ip (MyData);
    return;
  }
  gtk_window_set_title (GTK_WINDOW (dialog), _("Your ip addresses"));
  gtk_window_set_icon (GTK_WINDOW(dialog), pixbuf);
  transmit[_DIALOG] = (void *)dialog;
  transmit[_MYDATA] = (void *)MyData;
  g_object_unref (pixbuf), pixbuf = NULL;

  g_timeout_add (500, set_dialog_content, transmit);
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
show_info_notify (NotifyNotification *notification, char *action, gpointer user_data)
{
  show_info (NULL, NULL);
  notify_notification_show(notification, NULL);
}

static GtkStatusIcon *
create_tray_icon (GtkWidget * menu, GdkPixbuf *pdata)
{
  GtkStatusIcon *tray_icon;

  tray_icon = gtk_status_icon_new_from_pixbuf (pdata);
  if (tray_icon == NULL) {
    fprintf (stderr, _("Can't create status icon.\n"));
    cfg.op_mode = -1;
    return NULL;
  }
  if (gtk_status_icon_is_embedded (tray_icon) && cfg.op_mode == _AUTO) {
    fprintf (stderr, _("Can't view status icon, disabling...\n"));
    cfg.op_mode = -1;
    return tray_icon;
  }
  if (menu)
    g_signal_connect (G_OBJECT (tray_icon),
                      "popup-menu", G_CALLBACK (tray_icon_on_menu), menu);
  gtk_status_icon_set_visible (tray_icon, TRUE);

  return tray_icon;
}

static GtkWidget *
create_menu ()
{
  GtkWidget *menu, *menuItemIP, *menuItemAbout, *menuItemExit;
  menu = gtk_menu_new ();
  if (menu == NULL)
    return NULL;
  menuItemIP = gtk_menu_item_new_with_label (_("Your IP"));
  menuItemAbout = gtk_menu_item_new_with_label (_("About"));
  menuItemExit = gtk_menu_item_new_with_label (_("Exit"));
  if (!menuItemIP || !menuItemAbout || !menuItemExit)
    return NULL;
  g_signal_connect (G_OBJECT (menuItemIP), "activate", G_CALLBACK (show_info),
                    NULL);
  g_signal_connect (G_OBJECT (menuItemAbout), "activate",
                    G_CALLBACK (tray_about), NULL);
  g_signal_connect (G_OBJECT (menuItemExit), "activate",
                    G_CALLBACK (tray_exit), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemIP);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemAbout);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemExit);
  gtk_widget_show_all (menu);
  return menu;
}

static gboolean
internet_update (gpointer data)
{
  Exchange *exchange = (Exchange *)data;
  GdkPixbuf *pdata;
  gchar *message;
  static gboolean internet_on = FALSE;
  static int internet_back = -1;

  if ((internet_on = check_internet ())) {
    pdata = gdk_pixbuf_from_pixdata ((GdkPixdata *) & my_pixbuf_ok, TRUE, NULL);
    message = _("Internet connection available");
  }
  else {
    pdata = gdk_pixbuf_from_pixdata ((GdkPixdata *) & my_pixbuf_ko, TRUE, NULL);
    message = _("No internet connection available");
  }

  if ((cfg.op_mode == _NOTIFY || cfg.op_mode == _AUTO) && internet_back != internet_on) {
    if (!(exchange->notify)) {
      exchange->notify = notify_notification_new (
  			"Internet status",
      	message,
  			NULL);
      if (cfg.op_mode == _AUTO && exchange->notify == NULL) {
        cfg.op_mode = _ICON;
        goto handle_icon;
      }
    }
    else {
      notify_notification_update (exchange->notify, "Internet status", message, NULL);
    }
    notify_notification_clear_actions (exchange->notify);
    notify_notification_add_action (exchange->notify, "info", _("Info"), NOTIFY_ACTION_CALLBACK(notify_about), &internet_on, NULL);
    if (internet_on) {
      notify_notification_add_action (exchange->notify, "showip", _("Your IP"), NOTIFY_ACTION_CALLBACK(show_info_notify), NULL, NULL);
    }
    notify_notification_set_image_from_pixbuf (exchange->notify, pdata);
    notify_notification_show (exchange->notify, NULL);
  }

handle_icon:
  if (cfg.op_mode == _ICON) {
    if (!(exchange->icon)) {
      exchange->icon = create_tray_icon (create_menu (), pdata);
      if (!(exchange->icon))
        return FALSE;
    }
    else {
      gtk_status_icon_set_from_pixbuf (exchange->icon, pdata);
    }
    if (cfg.op_mode == -1) {
      gtk_status_icon_set_visible (exchange->icon, FALSE);
    }
    else {
      gtk_status_icon_set_tooltip_text (exchange->icon, message);
      gtk_status_icon_set_visible (exchange->icon, TRUE);
    }
  }


  g_object_unref (pdata), pdata = NULL;

  internet_back = internet_on;
  
  return TRUE;
}

OpMode
guess_op_mode (void)
{
  int i = 0;
  MyDesktop desktop[] = {
    {"gnome", _NOTIFY},
    {"kde", _ICON},
    {"ubuntu", _NOTIFY},
    {"mint", _ICON},
    {"icewm", -1},
    {"twm", -1},
    {NULL, _AUTO}
  };
  char *desktop_session = getenv("DESKTOP_SESSION");

  if (!(desktop_session)) return _AUTO;

  if (strncmp(desktop_session, "default", 7) == 0) {
    char *manager = getenv("SESSION_MANAGER");
    if (manager) desktop_session = manager;
  }

  while (TRUE) {
    if (desktop[i].desktop == NULL ||
        strstr(desktop_session, desktop[i].desktop))
      return desktop[i].op_mode;
    i++;
  }
  // unreachable code;
  return _AUTO;
}

int
main (int argc, char *argv[])
{
  Exchange exchange = {NULL};

  setlocale(LC_ALL, getenv("LANG"));
  bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain(GETTEXT_PACKAGE);


  if (find_task (argv[0])) {
    printf (_("Program is already active for this user.\n"));
    return (EXIT_FAILURE);
  }

  parse_config();

  if (cfg.op_mode == _AUTO) {
    cfg.op_mode = guess_op_mode();
  }
  
  if (!notify_init (PACKAGE))
		return (EXIT_FAILURE);

  res_init();

  gtk_init (&argc, &argv);

  if (cfg.op_mode >= 0) {
    internet_update (&exchange);
  }
  
  if (cfg.op_mode < 0) {
    printf(_("Nothing to do, exiting...\n"));
    notify_uninit();
    return (EXIT_SUCCESS);
  }

  g_timeout_add_seconds (cfg.timeout_seconds, internet_update, &exchange);

  gtk_main ();

  notify_uninit();
  
  return EXIT_SUCCESS;
}
