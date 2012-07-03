#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include "PhotoSelectPage.h"
#include "ImportWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>

class PhotoSelectPage;
 extern "C" {

  G_MODULE_EXPORT void
  drawingarea1_expose_event_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    printf("drawingarea1_expose_event_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> redraw_image();
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
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> next();
      photoSelectPage -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  quit(GtkWidget *widget, gpointer data) {
    printf("Quit_activated_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> quit();
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
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> back();
      photoSelectPage -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Rotate_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      printf("Rotate_clicked_cb\n");
      photoSelectPage -> rotation += 1;
      if (photoSelectPage -> rotation >= 4) photoSelectPage -> rotation = 0;
      photoSelectPage -> redraw_image();
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
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(
        GTK_WIDGET(GTK_MENU_ITEM(menuItem)));
    photoSelectPage->query();
  }

  G_MODULE_EXPORT void
  edit_preferences_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(
        GTK_WIDGET(GTK_MENU_ITEM(menuItem)));
    photoSelectPage->preferences();
  }

  G_MODULE_EXPORT void
  file_import_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(
        GTK_WIDGET(GTK_MENU_ITEM(menuItem)));
    photoSelectPage->import();
  }

  G_MODULE_EXPORT void
  import_response_cb(GtkWidget *widget, gint response_id, gpointer user_data) {
    printf("import_response_activate_cb\n");
    printf("response_id = %d\n", response_id);
    ImportWindow *importWindow = WindowRegistry::getImportWindow(widget);
    importWindow->import_response_cb(response_id);
  }

}
