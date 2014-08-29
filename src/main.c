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
#include "socket.h"
#include "findtask.h"
#include "xml.h"
#include "utils.h"
#include "main.h"

// status for ip dialog
static guint status = STATUS_NO_MESSAGE;

// allocate config and set defaults
Config cfg = { 15, "8.8.8.8", 53, "ifconfig.me/ip", "InternetIcon/0.1"};

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
  int rc = test_connection (cfg.test_ip, cfg.test_port);
  printf ("success = %d\n", rc);
  return (gboolean) rc;
}

static gboolean
_internet_update (gpointer data)
{
  GtkStatusIcon *tray_icon = (GtkStatusIcon *) data;
  GdkPixdata *pdata;
  GError *error = NULL;
  gchar *tooltip;

  if (check_internet ()) {
    pdata = (GdkPixdata *) & my_pixbuf_ok;
    tooltip = "Internet connection available";
  }
  else {
    pdata = (GdkPixdata *) & my_pixbuf_ko;
    tooltip = "No internet connection available";
  }
  gtk_status_icon_set_from_pixbuf (tray_icon,
                                   gdk_pixbuf_from_pixdata (pdata, TRUE,
                                                            &error));
  gtk_status_icon_set_tooltip_text (tray_icon, tooltip);
  gtk_status_icon_set_visible (tray_icon, TRUE);

  return TRUE;
}

static void
tray_exit (GtkMenuItem * item, gpointer user_data)
{
  //printf("exit");
  gtk_main_quit ();
}

static void
tray_about (GtkMenuItem * item, gpointer window)
{
  GError *error = NULL;
  GdkPixbuf *pixbuf = gdk_pixbuf_from_pixdata (&my_pixbuf_ok, TRUE, &error);

  GtkWidget *dialog = gtk_about_dialog_new ();
//  gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "InternetIcon");
  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog), "0.1");
  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dialog),
                                  "(c) Paolo Bozzo");
  gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (dialog),
                                 "InternetIcon is a simple tool that shows your internet status.");
  /*
     gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), 
     "http://www.casabozzo.net");
   */
  gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (dialog), pixbuf);
  g_object_unref (pixbuf), pixbuf = NULL;
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
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

  if (status == STATUS_MESSAGE_FIRST) {
    status = STATUS_MESSAGE;
    //g_source_remove (tag);
  }
  
  if (MyData) {
    get_wan_ip (MyData);
    content = build_dialog_string (MyData);
    if (GTK_IS_MESSAGE_DIALOG (dialog) && content) {
      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(dialog), content);
      GtkWidget *button = gtk_dialog_add_button (GTK_DIALOG(dialog), "Close", 1234);
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

  if (status > STATUS_NO_MESSAGE)
    return;

  MyData = get_lan_ip ();
  if (MyData == NULL)
    return;
  
  content = build_dialog_string (MyData);
  
  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_NONE,
                                               content);
  if (content)
    free(content);
  if (dialog == NULL) {
    free_all_ip (MyData);
    return;
  }
  gtk_window_set_title (GTK_WINDOW (dialog), "Your ip addresses");
  transmit[_DIALOG] = (void *)dialog;
  transmit[_MYDATA] = (void *)MyData;

  g_timeout_add (200, set_dialog_content, transmit);
  
  status = STATUS_MESSAGE_FIRST;

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  status = STATUS_NO_MESSAGE;
}

static GtkStatusIcon *
create_tray_icon (GtkWidget * menu)
{
  GtkStatusIcon *tray_icon;

  tray_icon = gtk_status_icon_new ();
  if (tray_icon == NULL) {
    fprintf (stderr, "Can't create status icon");
    exit (5);
  }
  if (menu)
    g_signal_connect (G_OBJECT (tray_icon),
                      "popup-menu", G_CALLBACK (tray_icon_on_menu), menu);
  gtk_status_icon_set_visible (tray_icon, FALSE);

  return tray_icon;
}

static GtkWidget *
create_menu ()
{
  GtkWidget *menu, *menuItemIP, *menuItemAbout, *menuItemExit;
  menu = gtk_menu_new ();
  if (menu == NULL)
    return NULL;
  menuItemIP = gtk_menu_item_new_with_label ("Your IP");
  menuItemAbout = gtk_menu_item_new_with_label ("About");
  menuItemExit = gtk_menu_item_new_with_label ("Exit");
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

int
main (int argc, char *argv[])
{
  GtkStatusIcon *tray_icon;

  if (find_task (argv[0])) {
    printf ("Program is already active for this user.\n");
    return (5);
  }

  parse_config();

  gtk_init (&argc, &argv);

  tray_icon = create_tray_icon (create_menu ());

  g_timeout_add_seconds (cfg.timeout_seconds, _internet_update, tray_icon);

  gtk_main ();

  return 0;
}
