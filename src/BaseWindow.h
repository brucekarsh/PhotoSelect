#ifndef BASEWINDOW_H__
#define BASEWINDOW_H__

#include <gtk/gtk.h>
#include "QueryWindow.h"
#include "PreferencesWindow.h"
#include "ImportWindow.h"

class BaseWindow {
  public:
  GtkWidget *top_level_window;
  GtkWidget *top_level_vbox;
  GtkWidget *top_level_menu_bar;
  GtkWidget *file_menu_item;
  GtkWidget *edit_menu_item;
  GtkWidget *view_menu_item;
  GtkWidget *help_menu_item;
  GtkWidget *file_menu;
  GtkWidget *edit_menu;
  GtkWidget *view_menu;
  GtkWidget *help_menu;
  GtkWidget *file_import_menu_item;
  GtkWidget *file_quit_menu_item;
  GtkWidget *edit_preferences_menu_item;
  GtkWidget *view_query_menu_item;
  GtkWidget *notebook;

  static sql::Connection *connection;
  static Preferences* thePreferences;

  BaseWindow(sql::Connection *connection, Preferences *thePreferences) {
    BaseWindow::connection = connection;
    BaseWindow::thePreferences = thePreferences;
  };

  void
  add_page(GtkWidget* label, GtkWidget* page) {
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
  }


  void run() {
    // Make a GtkWindow (top_level_window)
    top_level_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(top_level_window), "PhotoSelect");
    gtk_window_set_resizable(GTK_WINDOW(top_level_window), TRUE);
    gtk_widget_show(top_level_window);

    // Put a GtkBox (top_level_vbox) in top_level_window
    top_level_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(top_level_window), top_level_vbox);
    gtk_widget_show(top_level_vbox);

    // Put a menubar (top_level_menu_bar) in top_level_vbox
    top_level_menu_bar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(top_level_vbox), top_level_menu_bar, FALSE, FALSE, 0);
    gtk_widget_show(top_level_menu_bar);

    // Put a menuitem (file_menu_item) in top_level_menu_bar
    file_menu_item = gtk_menu_item_new_with_label("_File");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(file_menu_item), TRUE);
    gtk_container_add(GTK_CONTAINER(top_level_menu_bar), file_menu_item);
    gtk_widget_show(file_menu_item);

    // Put a menuitem (edit_menu_item) in top_level_menu_bar
    edit_menu_item = gtk_menu_item_new_with_label("_Edit");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(edit_menu_item), TRUE);
    gtk_container_add(GTK_CONTAINER(top_level_menu_bar), edit_menu_item);
    gtk_widget_show(edit_menu_item);

    // Put a menuitem (view_item) in top_level_menu_bar
    view_menu_item = gtk_menu_item_new_with_label("_View");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(view_menu_item), TRUE);
    gtk_container_add(GTK_CONTAINER(top_level_menu_bar), view_menu_item);
    gtk_widget_show(view_menu_item);

    // Put a menuitem (help_item) in top_level_menu_bar
    help_menu_item = gtk_menu_item_new_with_label("_Help");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(help_menu_item), TRUE);
    gtk_container_add(GTK_CONTAINER(top_level_menu_bar), help_menu_item);
    gtk_widget_show(help_menu_item);

    // Put a menu (file_menu) in file_menu_item
    file_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_widget_show(file_menu);

    // Put a menu (edit_menu) in edit_menu_item
    edit_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_menu_item), edit_menu);
    gtk_widget_show(edit_menu);

    // Put a menu (view_menu) in view_menu_item
    view_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_menu_item), view_menu);
    gtk_widget_show(edit_menu);

    // Put a menu (help_menu) in help_menu_item
    help_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_item), help_menu);
    gtk_widget_show(edit_menu);

    // Put a menuitem (file_import_menu_item) into file_menu
    file_import_menu_item = gtk_menu_item_new_with_label("Import");
    gtk_container_add(GTK_CONTAINER(file_menu), file_import_menu_item);
    gtk_widget_show(file_import_menu_item);
    g_signal_connect(file_import_menu_item, "activate", G_CALLBACK(file_import_activate_cb), NULL);

    // Put an imagemenuitem (file_quit_menu_item) into file_menu
    file_quit_menu_item = gtk_menu_item_new_with_label("Quit");
    gtk_container_add(GTK_CONTAINER(file_menu), file_quit_menu_item);
    gtk_widget_show(file_quit_menu_item);
    g_signal_connect(file_quit_menu_item, "activate", G_CALLBACK(gtk_main_quit), NULL);

    // Put a menuitem (edit_preferences_menu_item) into edit_menu
    edit_preferences_menu_item = gtk_menu_item_new_with_label("Preferences");
    gtk_container_add(GTK_CONTAINER(edit_menu), edit_preferences_menu_item);
    gtk_widget_show(edit_preferences_menu_item);
    g_signal_connect(edit_preferences_menu_item, "activate", G_CALLBACK(edit_preferences_activate_cb), NULL);

    // Put a menuitem (view_query_menu_item) into view_menu
    view_query_menu_item = gtk_menu_item_new_with_label("Query");
    gtk_container_add(GTK_CONTAINER(view_menu), view_query_menu_item);
    gtk_widget_show(view_query_menu_item);
    g_signal_connect(view_query_menu_item, "activate", G_CALLBACK(view_query_activate_cb), NULL);

    // Put a notebook (notebook) into top_level_vbox
    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(top_level_vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);
    
  }

  static void
  view_query_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    std::cout << "view_query_activate_cb" << std::endl;
    QueryWindow* queryWindow = new QueryWindow(connection);
    queryWindow->run();
    // XXX TODO make sure that queryWindow gets destroyed eventually.
  }

  static void
  edit_preferences_activate_cb() {
    std::cout << "edit_preferences_activate_cb" << std::endl;
    PreferencesWindow *preferencesWindow = new PreferencesWindow(thePreferences);
    preferencesWindow->run();
  }

  static void
  file_import_activate_cb() {
    std::cout << "file_import_activate_cb" << std::endl;
    ImportWindow *importWindow = new ImportWindow(thePreferences, connection);
    importWindow->run();
  }
};
#endif // BASEWINDOW_H__