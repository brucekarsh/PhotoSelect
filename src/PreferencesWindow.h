#ifndef PREFERENCESWINDOW_H__
#define PREFERENCESWINDOW_H__
#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include "PhotoSelectWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>

class PhotoSelectWindow;

class PreferencesWindow {
  public:
  Preferences *thePreferences;
  GtkWidget *window;
  GtkEntry *dbhost;    // GtkEntry for the Db Host
  GtkEntry *user;      // GtkEntry for the Db User
  GtkEntry *password;  // GtkEntry for the Db Password
  GtkEntry *database;  // GtkEntry for the DB database name


  PreferencesWindow(Preferences* thePreferences) {
    this->thePreferences = thePreferences;
  }

  void run() {
    printf("PreferencesWindow::run()\n");
    
    /* Load UI from file. If error occurs, report it and quit application. */
    GError *error = NULL;
    GtkBuilder* builder = gtk_builder_new();
    if( ! gtk_builder_add_from_file( builder,
        "/home/bruce/PROJECTS/NEWPHOTOSELECT/src/GladeTestProject1.glade", &error ) ) {
      g_warning( "%s", error->message );
      g_free( error );
    }
    window = GTK_WIDGET( gtk_builder_get_object( builder, "PreferencesWindow" ));
    dbhost = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesDbHost" ));
    user = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesUser" ));
    password = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesPassword" ));
    database = GTK_ENTRY( gtk_builder_get_object( builder, "preferencesDatabase" ));

    gtk_entry_set_text(dbhost, thePreferences -> get_dbhost().c_str());
    gtk_entry_set_text(user, thePreferences -> get_user().c_str());
    gtk_entry_set_text(password, thePreferences -> get_password().c_str());
    gtk_entry_set_text(database, thePreferences -> get_database().c_str());
    WindowRegistry::setPreferencesWindow(window, this);

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref( G_OBJECT( builder ) );
    gtk_widget_show(window);
  }

  void close_clicked() {
    printf("PreferencesWindow::close_clicked called\n");
    printf("dbhost: %s\n", gtk_entry_get_text(dbhost));
    printf("user: %s\n", gtk_entry_get_text(user));
    printf("password: %s\n", gtk_entry_get_text(password));
    printf("database: %s\n", gtk_entry_get_text(database));
    thePreferences -> set_dbhost(gtk_entry_get_text(dbhost));
    thePreferences -> set_user(gtk_entry_get_text(user));
    thePreferences -> set_password(gtk_entry_get_text(password));
    thePreferences -> set_database(gtk_entry_get_text(database));
    thePreferences -> writeback();
    gtk_widget_destroy(window);
  }
};
#endif  // PREFERENCESWINDOW_H__
