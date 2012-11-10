#ifndef PREFERENCESWINDOW_H__
#define PREFERENCESWINDOW_H__
#include <algorithm>
#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
//#include <cairo-xlib.h>
#include "WidgetRegistry.h"
#include "Preferences.h"

class PreferencesWindow {
  public:
    Preferences *thePreferences;
    GtkWidget *window;
    GtkWidget *dbhost;    // GtkEntry for the Db Host
    GtkWidget *user;      // GtkEntry for the Db User
    GtkWidget *password;  // GtkEntry for the Db Password
    GtkWidget *database;  // GtkEntry for the DB database name
    GtkWidget *accept_button;  // GtkButton
    GtkWidget *cancel_button;  // GtkButton
    GtkWidget *apply_button;  // GtkButton
    std::list<GtkWidget *> exif_check_buttons;

    PreferencesWindow(Preferences* thePreferences);
    ~PreferencesWindow();
    void highlight();
    void run();
    void apply();
    std::list<std::string> get_checked_exif_selections();
    std::list<std::string> get_text_exif_selections();
    void quit();
    void accept_button_clicked();
    void apply_button_clicked();
    void cancel_button_clicked();

    // Static member functions

    static void accept_button_clicked_cb(GtkWidget* widget, gpointer callback_data);
    static void cancel_button_clicked_cb(GtkWidget* widget, gpointer callback_data);
    static void apply_button_clicked_cb(GtkWidget* widget, gpointer callback_data);
};
#endif  // PREFERENCESWINDOW_H__
