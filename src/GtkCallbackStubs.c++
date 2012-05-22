#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include "PhotoSelectWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>

class PhotoSelectWindow;
 extern "C" {

  G_MODULE_EXPORT void
  drawingarea1_expose_event_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    printf("drawingarea1_expose_event_cb\n");
    PhotoSelectWindow *photoSelectWindow = WindowRegistry::getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Keep_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Keep_clicked_cb\n");
  }

  G_MODULE_EXPORT void
  Drop_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Drop_clicked_cb\n");
  }

  G_MODULE_EXPORT void
  Next_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Next_clicked_cb\n");
    PhotoSelectWindow *photoSelectWindow = WindowRegistry::getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      photoSelectWindow -> next();
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  quit(GtkWidget *widget, gpointer data) {
    printf("Quit_activated_cb\n");
    PhotoSelectWindow *photoSelectWindow = WindowRegistry::getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      photoSelectWindow -> quit();
      gtk_main_quit();
    }
  }

  G_MODULE_EXPORT void
  preferences_Close_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Closed clicked in Preferences window\n");
    PreferencesWindow *preferencesWindow = WindowRegistry::getPreferencesWindow(widget);
    if (0 != preferencesWindow) {
      preferencesWindow->close_clicked();
    }
  }


  G_MODULE_EXPORT void
  Back_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Back_clicked_cb\n");
    PhotoSelectWindow *photoSelectWindow = WindowRegistry::getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      photoSelectWindow -> back();
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Rotate_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectWindow *photoSelectWindow = WindowRegistry::getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      printf("Rotate_clicked_cb\n");
      photoSelectWindow -> rotation += 1;
      if (photoSelectWindow -> rotation >= 4) photoSelectWindow -> rotation = 0;
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Gimp_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Gimp_clicked_cb\n");
  }

  G_MODULE_EXPORT void
  Position_changed_cb(GtkWidget *widget, gpointer data) {
    printf("Position_changed_cb\n");
  }

  G_MODULE_EXPORT void
  Position_editing_done_cb(GtkWidget *widget, gpointer data) {
    printf("editing_done_cb\n");
  }

  G_MODULE_EXPORT void
  view_query_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    PhotoSelectWindow *photoSelectWindow = WindowRegistry::getPhotoSelectWindow(GTK_WIDGET(GTK_MENU_ITEM(menuItem)));
    photoSelectWindow->query();
  }

  G_MODULE_EXPORT void
  edit_preferences_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    PhotoSelectWindow *photoSelectWindow = WindowRegistry::getPhotoSelectWindow(GTK_WIDGET(GTK_MENU_ITEM(menuItem)));
    photoSelectWindow->preferences();
  }

}
