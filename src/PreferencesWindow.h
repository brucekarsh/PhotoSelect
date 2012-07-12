#ifndef PREFERENCESWINDOW_H__
#define PREFERENCESWINDOW_H__
#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include "WindowRegistry.h"
#include "Preferences.h"

class PreferencesWindow {
  public:
  Preferences *thePreferences;
  GtkWidget *window;
  GtkEntry *dbhost;    // GtkEntry for the Db Host
  GtkEntry *user;      // GtkEntry for the Db User
  GtkEntry *password;  // GtkEntry for the Db Password
  GtkEntry *database;  // GtkEntry for the DB database name
  GtkWidget *accept_button;  // GtkButton
  GtkWidget *cancel_button;  // GtkButton
  GtkWidget *apply_button;  // GtkButton


  PreferencesWindow(Preferences* thePreferences) {
    this->thePreferences = thePreferences;
  }

  ~PreferencesWindow() {
    WindowRegistry::forgetPreferencesWindow(window);
  }

  void highlight() {
    std::cout << "highlight preferences window" << std::endl;
    GdkWindow *gdk_window = gtk_widget_get_window(window);
    gtk_window_set_urgency_hint(GTK_WINDOW(window), true);
    gtk_widget_show(window);
    gdk_window_raise(gdk_window);
    gdk_flush();
    gdk_window_show(gdk_window);
    gdk_flush();
    gint x, y;
    gdk_window_get_root_origin(gdk_window, &x, &y);
    std::cout << "gdk_window_get_root_origin " << x << " " << y << std::endl;
    gdk_window_move(gdk_window, x+10, y);
    gdk_flush();
    usleep(50000);
    gdk_window_move(gdk_window, x-10, y);
    gdk_flush();
    usleep(50000);
    gdk_window_move(gdk_window, x, y+10);
    gdk_flush();
    usleep(50000);
    gdk_window_move(gdk_window, x, y-10);
    gdk_flush();
    usleep(50000);
    gdk_window_move(gdk_window, x-10, y-10);
    gdk_flush();
    usleep(50000);
    gdk_window_move(gdk_window, x-10, y+10);
    gdk_flush();
    usleep(50000);
    gdk_window_move(gdk_window, x+10, y-10);
    gdk_flush();
    usleep(50000);
    gdk_window_move(gdk_window, x, y);
    gdk_flush();
  }

  void run() {
    
    /* Load UI from file. If error occurs, report it and quit application. */
    GError *error = NULL;
    GtkBuilder* builder = gtk_builder_new();
    if( ! gtk_builder_add_from_file( builder,
        "/home/bruce/PROJECTS/NEWPHOTOSELECT/src/GladeTestProject1.glade", &error ) ) {
      g_warning( "%s", error->message );
      g_free( error );
    }
    window = GTK_WIDGET( gtk_builder_get_object( builder, "PreferencesWindow" ));
    GdkGeometry geometry;
    geometry.min_width = -1;
    geometry.min_height = -1;
    geometry.max_width = G_MAXSHORT;
    geometry.max_height = -1;
    gtk_window_set_geometry_hints(GTK_WINDOW(window), window, &geometry,
        (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
    dbhost = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesDbHost" ));
    user = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesUser" ));
    password = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesPassword" ));
    database = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesDatabase" ));
    accept_button = GTK_WIDGET( gtk_builder_get_object( builder, "preferences_accept_button" ));
    cancel_button = GTK_WIDGET( gtk_builder_get_object( builder, "preferences_cancel_button" ));
    apply_button = GTK_WIDGET( gtk_builder_get_object( builder, "preferences_apply_button" ));

    gtk_entry_set_text(dbhost, thePreferences -> get_dbhost().c_str());
    gtk_entry_set_text(user, thePreferences -> get_user().c_str());
    gtk_entry_set_text(password, thePreferences -> get_password().c_str());
    gtk_entry_set_text(database, thePreferences -> get_database().c_str());
    WindowRegistry::setPreferencesWindow(window, this);

    gtk_builder_connect_signals(builder, NULL);
    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);
    g_signal_connect(cancel_button, "clicked", G_CALLBACK(cancel_button_clicked_cb), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(cancel_button_clicked_cb), NULL);
    g_signal_connect(apply_button, "clicked", G_CALLBACK(apply_button_clicked_cb), NULL);

    g_object_unref( G_OBJECT( builder ) );
    gtk_widget_show(window);
  }

  void apply() {
    thePreferences -> set_dbhost(gtk_entry_get_text(dbhost));
    thePreferences -> set_user(gtk_entry_get_text(user));
    thePreferences -> set_password(gtk_entry_get_text(password));
    thePreferences -> set_database(gtk_entry_get_text(database));
    thePreferences -> writeback();
  }

  void quit() {
    gtk_widget_destroy(window);
  }

  void accept_button_clicked() {
    apply();
    quit();
  }

  void apply_button_clicked() {
    apply();
  }

  void cancel_button_clicked() {
    quit();
  }

  static void
  accept_button_clicked_cb(GtkWidget* widget, gpointer callback_data) {
    PreferencesWindow *preferencesWindow = WindowRegistry::getPreferencesWindow(widget);
    preferencesWindow->accept_button_clicked();
  }

  static void
  cancel_button_clicked_cb(GtkWidget* widget, gpointer callback_data) {
    PreferencesWindow *preferencesWindow = WindowRegistry::getPreferencesWindow(widget);
    preferencesWindow->cancel_button_clicked();
  }

  static void
  apply_button_clicked_cb(GtkWidget* widget, gpointer callback_data) {
    PreferencesWindow *preferencesWindow = WindowRegistry::getPreferencesWindow(widget);
    preferencesWindow->apply_button_clicked();
  }
};
#endif  // PREFERENCESWINDOW_H__
