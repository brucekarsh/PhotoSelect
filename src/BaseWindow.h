#ifndef BASEWINDOW_H__
#define BASEWINDOW_H__

#include <gtk/gtk.h>
#include "PreferencesWindow.h"
#include "ImportWindow.h"
#include <boost/foreach.hpp>

class OpenProjectWindow;
class NewProjectWindow;
class RenameProjectWindow;
class DeleteProjectWindow;
class PhotoFileCache;

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
  GtkWidget *file_project_menu_item;
  GtkWidget *file_project_menu;
  GtkWidget *file_project_open_menu_item;
  GtkWidget *file_project_new_menu_item;
  GtkWidget *file_project_add_to_menu_item;
  GtkWidget *file_project_rename_menu_item;
  GtkWidget *file_project_remove_menu_item;
  GtkWidget *file_project_delete_menu_item;
  GtkWidget *file_quit_menu_item;
  GtkWidget *edit_preferences_menu_item;
  GtkWidget *view_tags_menu_item;
  std::list<GtkWidget *> view_tags_menu_items;
  GtkWidget *view_tags_menu;
  GtkWidget *notebook;
  PreferencesWindow *preferencesWindow;
  guint preferencesWindow_handler_id;
  gpointer preferencesWindow_instance;
  PhotoFileCache *photoFileCache;

  static sql::Connection *connection;
  static Preferences* thePreferences;

  std::list<guint> signal_handler_ids;
  std::list<gpointer> signal_instances;

  BaseWindow(sql::Connection *connection, Preferences *thePreferences,
      PhotoFileCache *photoFileCache_) {
    BaseWindow::connection = connection;
    BaseWindow::thePreferences = thePreferences;
    preferencesWindow = NULL;
    photoFileCache = photoFileCache_;
  };

  ~BaseWindow() {
    WindowRegistry<BaseWindow>::forgetWindow(top_level_window);
  }

  void
  add_page(GtkWidget* label, GtkWidget* page) {
    gint page_num = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), page, true);
    gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(notebook), page, true);
  }

  void
  remove_page(GtkWidget *page) {
    gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), page);
    if (-1 != page_num) {
      gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);
    }
  }


  void run() {
    GtkWidget *notebook_ = gtk_notebook_new();
    run(notebook_, GTK_WIN_POS_NONE, 500, 500);
  }

  void run(GtkWidget *notebook_, GtkWindowPosition window_position, gint width, gint height) {
    // Remember the notebook and make it a part of a group
    notebook = notebook_;
    gtk_notebook_set_group_name(GTK_NOTEBOOK(notebook), "BaseWindowNotebook");

    // Make a GtkWindow (top_level_window)
    top_level_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(top_level_window), window_position);
    gtk_window_set_title(GTK_WINDOW(top_level_window), "PhotoSelect");
    gtk_window_set_resizable(GTK_WINDOW(top_level_window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(top_level_window), width, height);
    gtk_widget_show(top_level_window);
    WindowRegistry<BaseWindow>::setWindow(top_level_window, this);


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

    // Put a menuitem (file_project_menu_item) into file_menu
    file_project_menu_item = gtk_menu_item_new_with_label("Project");
    gtk_container_add(GTK_CONTAINER(file_menu), file_project_menu_item);
    gtk_widget_show(file_project_menu_item);
    
    // Put a menuitem (file_import_menu_item) into file_menu
    file_import_menu_item = gtk_menu_item_new_with_label("Import...");
    gtk_container_add(GTK_CONTAINER(file_menu), file_import_menu_item);
    gtk_widget_show(file_import_menu_item);

    // Put a menu (file_project_menu) into file_project_menu_item
    file_project_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_project_menu_item), file_project_menu);
    gtk_widget_show(file_project_menu);

    // Put a menuitem (file_project_open_menuitem) into file_project_menu
    file_project_open_menu_item = gtk_menu_item_new_with_label("Open Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_open_menu_item);
    gtk_widget_show(file_project_open_menu_item);

    // Put a menuitem (file_project_new_menuitem) into file_project_menu
    file_project_new_menu_item = gtk_menu_item_new_with_label("New Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_new_menu_item);
    gtk_widget_show(file_project_new_menu_item);

    // Put a menuitem (file_project_add_to_menuitem) into file_project_menu
    file_project_add_to_menu_item = gtk_menu_item_new_with_label("Add To Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_add_to_menu_item);
    gtk_widget_show(file_project_add_to_menu_item);

    // Put a menuitem (file_project_remove_menuitem) into file_project_menu
    file_project_remove_menu_item = gtk_menu_item_new_with_label("Remove From Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_remove_menu_item);
    gtk_widget_show(file_project_remove_menu_item);

    // Put a menuitem (file_project_rename_menuitem) into file_project_menu
    file_project_rename_menu_item = gtk_menu_item_new_with_label("Rename Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_rename_menu_item);
    gtk_widget_show(file_project_rename_menu_item);

    // Put a menuitem (file_project_delete_menuitem) into file_project_menu
    file_project_delete_menu_item = gtk_menu_item_new_with_label("Delete Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_delete_menu_item);
    gtk_widget_show(file_project_delete_menu_item);

    // Put an imagemenuitem (file_quit_menu_item) into file_menu
    file_quit_menu_item = gtk_menu_item_new_with_label("Quit");
    gtk_container_add(GTK_CONTAINER(file_menu), file_quit_menu_item);
    gtk_widget_show(file_quit_menu_item);

    // Put a menuitem (edit_preferences_menu_item) into edit_menu
    edit_preferences_menu_item = gtk_menu_item_new_with_label("Preferences");
    gtk_container_add(GTK_CONTAINER(edit_menu), edit_preferences_menu_item);
    gtk_widget_show(edit_preferences_menu_item);

    // Put a menuitem (view_tags_menu_item) into view_menu
    view_tags_menu_item = gtk_menu_item_new_with_label("Tags");
    gtk_container_add(GTK_CONTAINER(view_menu), view_tags_menu_item);
    gtk_widget_show(view_tags_menu_item);

    // Put a menu (view_tags_menu) into view_tags_menu_item
    view_tags_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_tags_menu_item), view_tags_menu);
    gtk_widget_show(view_tags_menu);

    // Put some choices into the view_tags_menu
    GSList *group = NULL;
    std::string view_tags_menu_item_labels[] = { "none", "left", "right", "top", "bottom"};
    BOOST_FOREACH(std::string label, view_tags_menu_item_labels) {
      GtkWidget *item = gtk_radio_menu_item_new_with_label(group, label.c_str());
      view_tags_menu_items.push_back(item);
      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
      gtk_container_add(GTK_CONTAINER(view_tags_menu), item);
      gtk_widget_show(item);
    }

    // Put a notebook (notebook) into top_level_vbox
    gtk_box_pack_start(GTK_BOX(top_level_vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);

    connect_signals();
  }

  void file_project_new_activate();
  void file_project_open_activate();
  void file_project_rename_activate();
  void file_project_delete_activate();

  static void
  file_project_open_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(menuItem));
    baseWindow->file_project_open_activate();
  }

  static void
  file_project_new_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(menuItem));
    baseWindow->file_project_new_activate();
  }

  static void
  file_project_rename_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(menuItem));
    baseWindow->file_project_rename_activate();
  }

  static void
  file_project_delete_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(menuItem));
    baseWindow->file_project_delete_activate();
  }

  static void
  edit_preferences_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(menuItem));
    baseWindow->preferences_activate();
  }

  void
  preferences_activate() {
    if (NULL == preferencesWindow) {
      preferencesWindow = new PreferencesWindow(thePreferences);
      preferencesWindow->run();
      g_signal_connect(preferencesWindow->window, "destroy", G_CALLBACK(preferences_window_destroy_cb), (gpointer) this);
    } else {
      preferencesWindow->highlight();
    }
  }

  static void
  file_import_activate_cb() {
    ImportWindow *importWindow = new ImportWindow(thePreferences, connection);
    importWindow->run();
  }

  void
  preferences_window_destroy() {
    g_signal_handler_disconnect(preferencesWindow_instance, preferencesWindow_handler_id);
    preferencesWindow = NULL;
    preferencesWindow_instance = NULL;
    preferencesWindow_handler_id = 0;
  }

  static void
  preferences_window_destroy_cb(GtkWidget *widget, gpointer user_data) {
    BaseWindow *baseWindow = (BaseWindow *)user_data;
    baseWindow->preferences_window_destroy();
  }

  static GtkNotebook *
  create_window_cb(GtkNotebook *notebook, GtkWidget *page, gint x, gint y, gpointer user_data) {
    BaseWindow *base_window = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(notebook));
    GtkNotebook *new_notebook = base_window->create_window(notebook, page, x, y, user_data);
    return new_notebook;
  }

  void
  page_removed(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
    std::cout << "page_removed entered" << std::endl;
    gint n_pages = gtk_notebook_get_n_pages(notebook);
    if (0 == n_pages) {
      std::cout << "page_removed calling quit" << std::endl;
      quit();
      std::cout << "page_removed returning from quit" << std::endl;
    }
    std::cout << "page_removed returning" << std::endl;
  }

  static void
  page_removed_cb(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
    std::cout << "page_removed_cb entered" << std::endl;
    BaseWindow *base_window = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(notebook));
    if (NULL != base_window) {
      std::cout << "page_removed_cb calling page_removed" << std::endl;
      base_window->page_removed(notebook, child, page_num, user_data);
      std::cout << "page_removed_cb returning from page_removed" << std::endl;
    }
    std::cout << "page_removed_cb returning" << std::endl;
  }

  GtkNotebook *
  create_window(GtkNotebook *notebook, GtkWidget *page, gint x, gint y, gpointer user_data) {
    GtkWidget* parent_window = gtk_widget_get_toplevel(GTK_WIDGET(notebook));
    gint parent_window_width, parent_window_height;
    gtk_window_get_size(GTK_WINDOW(parent_window), &parent_window_width, &parent_window_height);
    GtkWidget *new_notebook = gtk_notebook_new();
    BaseWindow *base_window = new BaseWindow(connection, thePreferences, photoFileCache);
    base_window->run(new_notebook, GTK_WIN_POS_MOUSE, parent_window_width, parent_window_height);
    return GTK_NOTEBOOK(new_notebook);
  }

  void
  quit() {
    std::cout << "quit entered" << std::endl;
    std::cout << "quit calling disconnect signals" << std::endl;
    disconnect_signals();
    // If we are the last BaseWindow, then stop the event loop
    long n_base_windows =  WindowRegistry<BaseWindow>::count();
    std::cout << "quit n_base_windows = " << n_base_windows << std::endl;
    if (1 == n_base_windows) {
      std::cout << "quit calling gtk_main_quit" << std::endl;
      gtk_main_quit();
      std::cout << "quit returning from gtk_main_quit" << std::endl;
    }
    std::cout << "quit calling gtk_widget_destroy" << std::endl;
    gtk_widget_destroy(top_level_window);
    std::cout << "quit returning from gtk_widget_destroy" << std::endl;
    std::cout << "quit calling delete this" << std::endl;
    delete this;
    std::cout << "quit return from delete this" << std::endl;
    std::cout << "quit returning" << std::endl;
  }

  static void
  quit_cb(GtkWidget *widget, gpointer user_data) {
    std::cout << "quit_cb entered" << std::endl;
    BaseWindow *base_window = WindowRegistry<BaseWindow>::getWindow(widget);
    std::cout << "quit_cb base_window = " << (long) base_window << std::endl;
    if (NULL != base_window) {
      std::cout << "quit_cb calling quit" << std::endl;
      base_window->quit();
      std::cout << "quit_cb returning from quit" << std::endl;
    }
    std::cout << "quit_cb returning" << std::endl;
  }

  void
  view_tags_toggled(GtkCheckMenuItem *checkmenuitem) {
    std::cout << gtk_menu_item_get_label(GTK_MENU_ITEM(checkmenuitem))
        << " " << gtk_check_menu_item_get_active(checkmenuitem) << std::endl;
  }

  static void
  view_tags_toggled_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    BaseWindow *base_window = WindowRegistry<BaseWindow>::getWindow(GTK_WIDGET(checkmenuitem));
    base_window->view_tags_toggled(checkmenuitem);
  }
    
  void connect_signals() {
    connect_signal(top_level_window, "destroy", G_CALLBACK(quit_cb), NULL);
    connect_signal(file_import_menu_item, "activate", G_CALLBACK(file_import_activate_cb), NULL);
    connect_signal(file_project_open_menu_item, "activate",
        G_CALLBACK(file_project_open_activate_cb), NULL);
    connect_signal(file_project_new_menu_item, "activate",
        G_CALLBACK(file_project_new_activate_cb), NULL);
    connect_signal(file_project_rename_menu_item, "activate",
        G_CALLBACK(file_project_rename_activate_cb), NULL);
    connect_signal(file_project_delete_menu_item, "activate",
        G_CALLBACK(file_project_delete_activate_cb), NULL);
    connect_signal(file_quit_menu_item, "activate", G_CALLBACK(quit_cb), NULL);
    connect_signal(edit_preferences_menu_item, "activate",
        G_CALLBACK(edit_preferences_activate_cb), NULL);
    connect_signal(notebook, "create-window", G_CALLBACK(create_window_cb), NULL);
    connect_signal(notebook, "page-removed", G_CALLBACK(page_removed_cb), NULL);
    BOOST_FOREACH(GtkWidget *item, view_tags_menu_items) {
      connect_signal(item, "toggled", G_CALLBACK(view_tags_toggled_cb), NULL);
    }
  }

  void connect_signal(
      gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data) {
    glong signal_handler_id = g_signal_connect(instance, detailed_signal, c_handler, data);
    signal_handler_ids.push_back(signal_handler_id);
    signal_instances.push_back(instance);
  }

  void disconnect_signals() {
    std::list<gpointer>::iterator signal_instances_it = signal_instances.begin();
    BOOST_FOREACH(gulong signal_handler_id, signal_handler_ids) {
      gpointer instance = *signal_instances_it;
      ++signal_instances_it;
      g_signal_handler_disconnect(instance, signal_handler_id);
    }
    if (NULL != preferencesWindow) {
      g_signal_handler_disconnect(preferencesWindow_instance, preferencesWindow_handler_id);
    }
  }
};

#include "BaseWindow.h"
#include "OpenProjectWindow.h"
#include "NewProjectWindow.h"
#include "DeleteProjectWindow.h"
#include "RenameProjectWindow.h"

inline  void
BaseWindow::file_project_open_activate() {
  OpenProjectWindow* openProjectWindow =
      new OpenProjectWindow(connection, thePreferences, photoFileCache, this);
  openProjectWindow->run();
  // TODO make sure that openProjectWindow gets destroyed eventually.
}

inline  void
BaseWindow::file_project_new_activate() {
  NewProjectWindow* newProjectWindow =
      new NewProjectWindow(connection, thePreferences, photoFileCache, this);
  newProjectWindow->run();
  // TODO make sure that newProjectWindow gets destroyed eventually.
}

inline  void
BaseWindow::file_project_rename_activate() {
  RenameProjectWindow* renameProjectWindow =
      new RenameProjectWindow(connection, thePreferences, this);
  renameProjectWindow->run();
  // TODO make sure that RenameProjectWindow gets destroyed eventually.
}

inline  void
BaseWindow::file_project_delete_activate() {
  DeleteProjectWindow* deleteProjectWindow =
      new DeleteProjectWindow(connection, thePreferences, this);
  deleteProjectWindow->run();
  // TODO make sure that deleteProjectWindow gets destroyed eventually.
}
#endif // BASEWINDOW_H__
