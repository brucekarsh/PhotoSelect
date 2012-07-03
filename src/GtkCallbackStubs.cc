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
// TODO get rid of excessive redraws.
}
