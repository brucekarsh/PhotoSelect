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

  PreferencesWindow(Preferences* thePreferences) {
    this->thePreferences = thePreferences;
  }

  ~PreferencesWindow() {
    WidgetRegistry<PreferencesWindow>::forget_widget(window);
  }

  void highlight() {
    GdkWindow *gdk_window = gtk_widget_get_window(window);
    gtk_window_set_urgency_hint(GTK_WINDOW(window), true);
    gtk_widget_show(window);
    gdk_window_raise(gdk_window);
    gdk_flush();
    gdk_window_show(gdk_window);
    gdk_flush();
    gint x, y;
    gdk_window_get_root_origin(gdk_window, &x, &y);
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
    // Make a window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GdkGeometry geometry;
    geometry.min_width = -1;
    geometry.min_height = -1;
    geometry.max_width = G_MAXSHORT;
    geometry.max_height = -1;
    gtk_window_set_geometry_hints(GTK_WINDOW(window), window, &geometry,
        (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));

    // Make a vbox (window_vbox) to hold stuff than goes into window and put it in window
    GtkWidget *window_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(window_vbox);
    gtk_container_add(GTK_CONTAINER(window), window_vbox);

    // Make a notebook (notebook) to hold tabs for the various kinds of preferences
    // and put it in window_vbox
    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    gtk_box_pack_start(GTK_BOX(window_vbox), notebook, TRUE, TRUE, 0);
    // Make a table (database_preferences_tab_table) to hold the database preferences
    GtkWidget *database_preferences_tab_table = gtk_table_new(4, 2, false);
    gtk_widget_show(database_preferences_tab_table);
    // Make a label (database_preferences_tab_label) for the database preferences tab
    GtkWidget *database_preferences_tab_label = gtk_label_new("Database");
    gtk_widget_show(database_preferences_tab_label);
    // Put the table and label into the notebook as a page
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), database_preferences_tab_table,
        database_preferences_tab_label);

    // Make a vbox (exif_preferences_tab_vbox) to hold the exif preferences
    GtkWidget *exif_preferences_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(exif_preferences_tab_vbox);
    // Make a label (exif_preferences_tab_label) for the exif preferences tab
    GtkWidget *exif_preferences_tab_label = gtk_label_new("Exif");
    gtk_widget_show(exif_preferences_tab_label);
    // Put the vbox and label into the notebook as a page
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), exif_preferences_tab_vbox,
        exif_preferences_tab_label);
    // Make a box (button_hbox) for buttons and put it into window_vbox
    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(button_hbox);
    gtk_box_pack_end(GTK_BOX(window_vbox), button_hbox, FALSE, FALSE, 0);
    // Add Help, Accept, Cancel and Apply buttons to button_box
    GtkWidget *help_button = gtk_button_new_with_label("Help");
    gtk_widget_show(button_hbox);
    gtk_box_pack_end(GTK_BOX(button_hbox), help_button, FALSE, FALSE, 0);
    accept_button = gtk_button_new_with_label("Accept");
    gtk_widget_show(accept_button);
    gtk_box_pack_end(GTK_BOX(button_hbox), accept_button, FALSE, FALSE, 0);
    cancel_button = gtk_button_new_with_label("Cancel");
    gtk_widget_show(cancel_button);
    gtk_box_pack_end(GTK_BOX(button_hbox), cancel_button, FALSE, FALSE, 0);
    apply_button = gtk_button_new_with_label("Apply");
    gtk_widget_show(apply_button);
    gtk_box_pack_end(GTK_BOX(button_hbox), apply_button, FALSE, FALSE, 0);

    // Make GtkEntrys (dbhost, user, password, database)
    dbhost = gtk_entry_new();
    gtk_widget_show(dbhost);
    user = gtk_entry_new();
    gtk_widget_show(user);
    password = gtk_entry_new();
    gtk_widget_show(password);
    database = gtk_entry_new();
    gtk_widget_show(database);

   // Make labels for the above GtkEntrys
    GtkWidget *dbhost_label = gtk_label_new("DbHost[localhost]");
    gtk_misc_set_alignment(GTK_MISC(dbhost_label), 1.0, 0.5);
    gtk_widget_show(dbhost_label);
    GtkWidget *user_label = gtk_label_new("User");
    gtk_misc_set_alignment(GTK_MISC(user_label), 1.0, 0.5);
    gtk_widget_show(user_label);
    GtkWidget *password_label = gtk_label_new("Password");
    gtk_misc_set_alignment(GTK_MISC(password_label), 1.0, 0.5);
    gtk_widget_show(password_label);
    GtkWidget *database_label = gtk_label_new("Database [PhotoSelect]");
    gtk_misc_set_alignment(GTK_MISC(database_label), 1.0, 0.5);
    gtk_widget_show(database_label);

    // Put the GtkEntrys and their labels into the database_preferences_tab_table
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        dbhost_label,   0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        dbhost,         1, 2, 0, 1, (GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        user_label,     0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        user,           1, 2, 1, 2, (GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        password_label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        password,       1, 2, 2, 3, (GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        database_label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(database_preferences_tab_table),
        database,       1, 2, 3, 4, (GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

    // Make GtkCheckBoxes for the exif fields
    std::string exif_choices[] = {
        "Exif.Image.DateTime",
        "Exif.Image.Make",
        "Exif.Image.Model",
        "Exif.Photo.ExposureTime",
        "Exif.Photo.FNumber",
        "Exif.Photo.FocalLengthIn35mmFilm",
        "Exif.Photo.ISOSpeedRatings",
        "Exif.Photo.PixelXDimension",
        "Exif.Photo.PixelYDimension"
    };
    exif_check_buttons.clear();
    std::list<std::string> checked_exif_selections = thePreferences->get_checked_exif_selections();
    std::list<std::string> text_exif_selections = thePreferences->get_text_exif_selections();
    BOOST_FOREACH(std::string choice, exif_choices) {
      GtkWidget *check_button = gtk_check_button_new_with_label(choice.c_str());
      if ( std::find(checked_exif_selections.begin(), checked_exif_selections.end(), choice) !=
          checked_exif_selections.end() ||
          std::find(text_exif_selections.begin(), text_exif_selections.end(), choice) !=
          text_exif_selections.end()) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), true);
      } 
      gtk_widget_show(check_button);
      exif_check_buttons.push_back(check_button);
      gtk_box_pack_start(GTK_BOX(exif_preferences_tab_vbox), check_button, false, false, 0);
    }

    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);
    g_signal_connect(cancel_button, "clicked", G_CALLBACK(cancel_button_clicked_cb), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(cancel_button_clicked_cb), NULL);
    g_signal_connect(apply_button, "clicked", G_CALLBACK(apply_button_clicked_cb), NULL);

    gtk_entry_set_text(GTK_ENTRY(dbhost), thePreferences -> get_dbhost().c_str());
    gtk_entry_set_text(GTK_ENTRY(user), thePreferences -> get_user().c_str());
    gtk_entry_set_text(GTK_ENTRY(password), thePreferences -> get_password().c_str());
    gtk_entry_set_text(GTK_ENTRY(database), thePreferences -> get_database().c_str());
    WidgetRegistry<PreferencesWindow>::set_widget(window, this);

    gtk_widget_show(window);
  }

  void apply() {
    thePreferences -> set_dbhost(gtk_entry_get_text(GTK_ENTRY(dbhost)));
    thePreferences -> set_user(gtk_entry_get_text(GTK_ENTRY(user)));
    thePreferences -> set_password(gtk_entry_get_text(GTK_ENTRY(password)));
    thePreferences -> set_database(gtk_entry_get_text(GTK_ENTRY(database)));

    std::list<std::string> checked_exif_selections = get_checked_exif_selections();
    std::list<std::string> text_exif_selections = get_text_exif_selections();
    thePreferences -> set_exif_selections(checked_exif_selections, text_exif_selections);
    thePreferences -> writeback();
  }

  std::list<std::string> get_checked_exif_selections() {
    std::list<std::string> selections;
    BOOST_FOREACH(GtkWidget *check_button, exif_check_buttons) {
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button))) {
        std::string exif_name = gtk_button_get_label(GTK_BUTTON(check_button));
	selections.push_back(exif_name);
      }
    }
    return selections;
  } 

  std::list<std::string> get_text_exif_selections() {
    std::list<std::string> selections;
    return selections;
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
    PreferencesWindow *preferencesWindow = WidgetRegistry<PreferencesWindow>::get_object(widget);
    preferencesWindow->accept_button_clicked();
  }

  static void
  cancel_button_clicked_cb(GtkWidget* widget, gpointer callback_data) {
    PreferencesWindow *preferencesWindow = WidgetRegistry<PreferencesWindow>::get_object(widget);
    preferencesWindow->cancel_button_clicked();
  }

  static void
  apply_button_clicked_cb(GtkWidget* widget, gpointer callback_data) {
    PreferencesWindow *preferencesWindow = WidgetRegistry<PreferencesWindow>::get_object(widget);
    preferencesWindow->apply_button_clicked();
  }
};
#endif  // PREFERENCESWINDOW_H__
