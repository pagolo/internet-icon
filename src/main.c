/*
 * main.c
 * Copyright (C) 2014 Paolo Bozzo <paolo@linux-l6dg>
 * 
 */

#include <stdio.h>
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
Config cfg = { 15, "auto", 53, _ENABLE, _AUTO, "ifconfig.me/ip", "InternetIcon/Get wan ip"};

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
    char *ip = inet_ntoa(_res.nsaddr_list[i].sin_addr);
    if (
         strstr(ip, "255.") == ip ||
         strstr(ip, "0.") == ip ||
         strstr(ip, "127.") == ip ||
         strstr(ip, "10.") == ip ||
         strstr(ip, "192.168.") == ip
       )
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
tray_about (GtkMenuItem * item, gpointer window)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_from_pixdata (&my_pixbuf_ok, TRUE, NULL);
  if (!(pixbuf)) {
    fprintf (stderr, _("Get pixbuf error...\n"));
    return;
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
  gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (dialog), pixbuf);
  g_object_unref (pixbuf), pixbuf = NULL;
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void notify_about (gpointer data)
{
  tray_about (NULL, NULL);
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
  transmit[_DIALOG] = (void *)dialog;
  transmit[_MYDATA] = (void *)MyData;

  g_timeout_add (300, set_dialog_content, transmit);
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
show_info_notify (gpointer data)
{
  show_info (NULL, NULL);
}

static GtkStatusIcon *
create_tray_icon (GtkWidget * menu, GdkPixbuf *pdata)
{
  GtkStatusIcon *tray_icon;

  tray_icon = gtk_status_icon_new_from_pixbuf (pdata);
  if (tray_icon == NULL) {
    fprintf (stderr, _("Can't create status icon.\n"));
    cfg.status_icon = _DISABLE;
    return NULL;
  }
  if (gtk_status_icon_is_embedded (tray_icon) && cfg.status_icon == _AUTO) {
    fprintf (stderr, _("Can't view status icon, disabling...\n"));
    cfg.status_icon = _DISABLE;
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
  static gboolean internet_on, internet_back = FALSE;

  if ((internet_on = check_internet ())) {
    pdata = gdk_pixbuf_from_pixdata ((GdkPixdata *) & my_pixbuf_ok, TRUE, NULL);
    message = _("Internet connection available");
  }
  else {
    pdata = gdk_pixbuf_from_pixdata ((GdkPixdata *) & my_pixbuf_ko, TRUE, NULL);
    message = _("No internet connection available");
  }

  if (cfg.status_icon != _DISABLE) {
    if (!(exchange->icon)) {
      exchange->icon = create_tray_icon (create_menu (), pdata);
      if (!(exchange->icon))
        return FALSE;
    }
    else {
      gtk_status_icon_set_from_pixbuf (exchange->icon, pdata);
    }
    if (cfg.status_icon == _DISABLE) {
      gtk_status_icon_set_visible (exchange->icon, FALSE);
    }
    else {
      gtk_status_icon_set_tooltip_text (exchange->icon, message);
    }
  }

  if (cfg.notification != _DISABLE && internet_back != internet_on) {
    if (!(exchange->notify)) {
      exchange->notify = notify_notification_new (
  			"Internet icon",
      	message,
  			NULL);
    }
    else {
      notify_notification_update (exchange->notify, "Internet icon", message, NULL);
    }
    notify_notification_clear_actions (exchange->notify);
    notify_notification_add_action (exchange->notify, "info", _("Info"), (NotifyActionCallback) notify_about, NULL, NULL);
    if (internet_on) {
      notify_notification_add_action (exchange->notify, "showip", _("Your IP"), (NotifyActionCallback) show_info_notify, NULL, NULL);
    }
    notify_notification_set_image_from_pixbuf (exchange->notify, pdata);
    notify_notification_show (exchange->notify, NULL);
  }

  internet_back = internet_on;
  
  return TRUE;
}

int
main (int argc, char *argv[])
{
  Exchange exchange = {NULL, NULL};

  setlocale(LC_ALL, getenv("LANG"));
  bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain(GETTEXT_PACKAGE);


  if (find_task (argv[0])) {
    printf (_("Program is already active for this user.\n"));
    return (EXIT_FAILURE);
  }

  parse_config();

  if (!notify_init (PACKAGE))
		return (EXIT_FAILURE);

  res_init();

  gtk_init (&argc, &argv);

  internet_update (&exchange);
  if (cfg.status_icon == _DISABLE && cfg.notification == _DISABLE) {
    printf(_("Nothing to do, exiting...\n"));
    notify_uninit();
    return (EXIT_SUCCESS);
  }

  g_timeout_add_seconds (cfg.timeout_seconds, internet_update, &exchange);

  gtk_main ();

  notify_uninit();
  
  return EXIT_SUCCESS;
}
