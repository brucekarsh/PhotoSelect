#ifndef BASEWINDOW_H__
#define BASEWINDOW_H__

#include <gtk/gtk.h>
#include "PreferencesWindow.h"
#include "ImportWindow.h"
#include <boost/foreach.hpp>
#include "WidgetRegistry.h"
#include "WidgetRegistry.h"

class OpenProjectWindow;
class NewProjectWindow;
class AddToProjectWindow;
class RemoveFromProjectWindow;
class RenameProjectWindow;
class DeleteProjectWindow;
class ExportProjectWindow;
class AdjustTimeWindow;
class EditTagsWindow;
class PhotoFileCache;

class BaseWindow {
  private:
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
  GtkWidget *file_project_remove_from_menu_item;
  GtkWidget *file_project_delete_menu_item;
  GtkWidget *file_project_export_menu_item;
  GtkWidget *file_adjust_time_menu_item;
  GtkWidget *file_quit_menu_item;
  GtkWidget *edit_preferences_menu_item;
  GtkWidget *edit_unselect_all_menu_item;
  GtkWidget *edit_tags_menu_item;
  GtkWidget *view_tags_menu_item;
  GtkWidget *view_clone_menu_item;
  GtkWidget *view_exif_menu_item;
  std::list<GtkWidget *> view_tags_menu_items;
  std::list<GtkWidget *> view_exif_menu_items;
  GtkWidget *view_tags_menu;
  GtkWidget *view_exif_menu;
  GtkWidget *notebook;
  PreferencesWindow *preferencesWindow;
  guint preferencesWindow_handler_id;
  gpointer preferencesWindow_instance;
  PhotoFileCache *photoFileCache;

  sql::Connection *connection;
  Preferences* thePreferences;

  std::list<guint> signal_handler_ids;
  std::list<gpointer> signal_instances;

  public:
  BaseWindow(sql::Connection *connection, Preferences *thePreferences,
      PhotoFileCache *photoFileCache_) {
    BaseWindow::connection = connection;
    BaseWindow::thePreferences = thePreferences;
    preferencesWindow = NULL;
    photoFileCache = photoFileCache_;
  };

  ~BaseWindow() {
    WidgetRegistry<BaseWindow>::forget_widget(top_level_window);
  }

  void
  add_page(GtkWidget* label, GtkWidget* page, std::string project_name) {
    GtkWidget *page_menu_label = gtk_label_new(project_name.c_str());
    gint page_num = gtk_notebook_append_page_menu(GTK_NOTEBOOK(notebook),
        page, label, page_menu_label);
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
    run(notebook_, GTK_WIN_POS_NONE, 800, 500);
  }

  void run(GtkWidget *notebook_, const GtkWindowPosition &window_position, gint width, gint height) {
    // Remember the notebook and make it a part of a group
    notebook = notebook_;
    gtk_notebook_set_group_name(GTK_NOTEBOOK(notebook), "BaseWindowNotebook");
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    build_window(window_position, width, height);
    build_menus();
    connect_signals();
    gtk_widget_show(top_level_window);
  }

  void build_window(const GtkWindowPosition &window_position, gint width, gint height) {
    // Make a GtkWindow (top_level_window)
    top_level_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(top_level_window), window_position);
    gtk_window_set_title(GTK_WINDOW(top_level_window), "PhotoSelect");
    gtk_window_set_resizable(GTK_WINDOW(top_level_window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(top_level_window), width, height);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), true);
    WidgetRegistry<BaseWindow>::set_widget(top_level_window, this);

    // Put a GtkBox (top_level_vbox) in top_level_window
    top_level_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(top_level_window), top_level_vbox);
    gtk_widget_show(top_level_vbox);

    // Put a menubar (top_level_menu_bar) in top_level_vbox
    top_level_menu_bar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(top_level_vbox), top_level_menu_bar, FALSE, FALSE, 0);
    gtk_widget_show(top_level_menu_bar);

    // Put a notebook (notebook) into top_level_vbox
    gtk_box_pack_start(GTK_BOX(top_level_vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);
  }

  void build_menus() {
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

    // Put a menuitem (file_adjust_time_menuitem) into file_menu
    file_adjust_time_menu_item = gtk_menu_item_new_with_label("Adjust Time...");
    gtk_container_add(GTK_CONTAINER(file_menu), file_adjust_time_menu_item);
    gtk_widget_show(file_adjust_time_menu_item);

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

    // Put a menuitem (file_project_remove_from_menu_item) into file_project_menu
    file_project_remove_from_menu_item = gtk_menu_item_new_with_label("Remove From Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_remove_from_menu_item);
    gtk_widget_show(file_project_remove_from_menu_item);

    // Put a menuitem (file_project_rename_menuitem) into file_project_menu
    file_project_rename_menu_item = gtk_menu_item_new_with_label("Rename Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_rename_menu_item);
    gtk_widget_show(file_project_rename_menu_item);

    // Put a menuitem (file_project_delete_menuitem) into file_project_menu
    file_project_delete_menu_item = gtk_menu_item_new_with_label("Delete Project...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_delete_menu_item);
    gtk_widget_show(file_project_delete_menu_item);

    // Put a menuitem (file_project_export_menuitem) into file_project_menu
    file_project_export_menu_item = gtk_menu_item_new_with_label("Export...");
    gtk_container_add(GTK_CONTAINER(file_project_menu), file_project_export_menu_item);
    gtk_widget_show(file_project_export_menu_item);

    // Put an imagemenuitem (file_quit_menu_item) into file_menu
    file_quit_menu_item = gtk_menu_item_new_with_label("Quit");
    gtk_container_add(GTK_CONTAINER(file_menu), file_quit_menu_item);
    gtk_widget_show(file_quit_menu_item);

    // Put a menuitem (edit_unselect_all_menu_item) into edit_menu
    edit_unselect_all_menu_item = gtk_menu_item_new_with_label("Unselect all");
    gtk_container_add(GTK_CONTAINER(edit_menu), edit_unselect_all_menu_item);
    gtk_widget_show(edit_unselect_all_menu_item);

    // Put a menuitem (edit_preferences_menu_item) into edit_menu
    edit_preferences_menu_item = gtk_menu_item_new_with_label("Preferences...");
    gtk_container_add(GTK_CONTAINER(edit_menu), edit_preferences_menu_item);
    gtk_widget_show(edit_preferences_menu_item);

    // Put a menuitem (edit_tags_menu_item) into edit_menu
    edit_tags_menu_item = gtk_menu_item_new_with_label("Tags...");
    gtk_container_add(GTK_CONTAINER(edit_menu), edit_tags_menu_item);
    gtk_widget_show(edit_tags_menu_item);

    // Put a menuitem (view_clone_menu_item) into view_menu
    view_clone_menu_item = gtk_menu_item_new_with_label("Clone");
    gtk_container_add(GTK_CONTAINER(view_menu), view_clone_menu_item);
    gtk_widget_show(view_clone_menu_item);

    // Put a menuitem (view_exif_menu_item) into view_menu
    view_exif_menu_item = gtk_menu_item_new_with_label("Exif");
    gtk_container_add(GTK_CONTAINER(view_menu), view_exif_menu_item);
    gtk_widget_show(view_exif_menu_item);

    // Put a menuitem (view_tags_menu_item) into view_menu
    view_tags_menu_item = gtk_menu_item_new_with_label("Tags");
    gtk_container_add(GTK_CONTAINER(view_menu), view_tags_menu_item);
    gtk_widget_show(view_tags_menu_item);

    // Put a menu (view_tags_menu) into view_tags_menu_item
    view_tags_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_tags_menu_item), view_tags_menu);
    gtk_widget_show(view_tags_menu);

    // Put a menu (view_exif_menu) into view_exif_menu_item
    view_exif_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_exif_menu_item), view_exif_menu);
    gtk_widget_show(view_exif_menu);

    // Put some choices into the view_tags_menu
    GSList *group = NULL;
    std::string view_tags_menu_item_labels[] = { "none", "left", "right" };
    BOOST_FOREACH(std::string label, view_tags_menu_item_labels) {
      GtkWidget *item = gtk_radio_menu_item_new_with_label(group, label.c_str());
      if (label == "right") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
      }
      view_tags_menu_items.push_back(item);
      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
      gtk_container_add(GTK_CONTAINER(view_tags_menu), item);
      gtk_widget_show(item);
    }

    // Put some choices into the view_exif_menu
    group = NULL;
    std::string view_exif_menu_item_labels[] = { "none", "left", "right" };
    BOOST_FOREACH(std::string label, view_exif_menu_item_labels) {
      GtkWidget *item = gtk_radio_menu_item_new_with_label(group, label.c_str());
      if (label == "right") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
      }
      view_exif_menu_items.push_back(item);
      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
      gtk_container_add(GTK_CONTAINER(view_exif_menu), item);
      gtk_widget_show(item);
    }
  }

  // Forward references
  void quit_all_notebook_pages();
  std::string get_project_name();
  void switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);
  void file_project_new_activate();
  void file_project_add_to_activate();
  void file_project_remove_from_activate();
  void file_project_open_activate();
  void file_project_rename_activate();
  void file_project_delete_activate();
  void file_project_export_activate();
  void file_adjust_time_activate();
  void view_tags_toggled(GtkCheckMenuItem *checkmenuitem);
  void view_exif_toggled(GtkCheckMenuItem *checkmenuitem);
  void clone_activate();
  void edit_tags_activate();
  void rebuild_all_tag_views();
  void edit_unselect_all_activate();

  static void
  file_project_open_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_project_open_activate();
    }
  }

  static void
  file_project_new_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_project_new_activate();
    }
  }

  static void
  file_project_add_to_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_project_add_to_activate();
    }
  }

  static void
  file_project_remove_from_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_project_remove_from_activate();
    }
  }

  static void
  file_project_rename_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_project_rename_activate();
    }
  }

  static void
  file_project_delete_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_project_delete_activate();
    }
  }

  static void
  file_project_export_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_project_export_activate();
    }
  }

  static void
  edit_unselect_all_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->edit_unselect_all_activate();
    }
  }

  static void
  edit_preferences_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->preferences_activate();
    }
  }

  static void
  edit_tags_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->edit_tags_activate();
    }
  }

  void
  preferences_activate() {
    if (NULL == preferencesWindow) {
      preferencesWindow = new PreferencesWindow(thePreferences);
      preferencesWindow->run();
      preferencesWindow_handler_id = g_signal_connect(preferencesWindow->window,
          "destroy", G_CALLBACK(preferences_window_destroy_cb), (gpointer) this);
      preferencesWindow_instance = preferencesWindow->window;
    } else {
      preferencesWindow->highlight();
    }
  }

  static void
  file_adjust_time_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_adjust_time_activate();
    }
  }

  void
  file_import_activate() {
    if (!connection) return;
    ImportWindow *importWindow = new ImportWindow(thePreferences, connection);
    importWindow->run();
  }

  static void
  file_import_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->file_import_activate();
    }
  }

  static void
  clone_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    BaseWindow* baseWindow = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(menuItem));
    if (baseWindow) {
      baseWindow->clone_activate();
    }
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
    BaseWindow *base_window = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(notebook));
    if (base_window) {
      GtkNotebook *new_notebook = base_window->create_window(notebook, page, x, y, user_data);
      return new_notebook;
    } else {
      return NULL;
    }
  }

  void
  page_removed(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
    gint n_pages = gtk_notebook_get_n_pages(notebook);
    if (0 == n_pages) {
      quit();
    }
  }

  static void
  page_removed_cb(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
    BaseWindow *base_window = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(notebook));
    if (NULL != base_window) {
      base_window->page_removed(notebook, child, page_num, user_data);
    }
  }

  static void
  switch_page_cb(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
    BaseWindow *base_window = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(notebook));
    if (NULL != base_window) {
      base_window->switch_page(notebook, page, page_num, user_data);
    }
  }

  GtkNotebook *
  create_window(GtkNotebook *notebook, GtkWidget *page, gint x, gint y, gpointer user_data) {
    if (!connection) return NULL;
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
    disconnect_signals();
    quit_all_notebook_pages();
    // If we are the last BaseWindow, then stop the event loop
    long n_base_windows =  WidgetRegistry<BaseWindow>::count();
    if (1 == n_base_windows) {
      gtk_main_quit();
    }
    gtk_widget_destroy(top_level_window);
    delete this;
  }

  static void
  quit_cb(GtkWidget *widget, gpointer user_data) {
    BaseWindow *base_window = WidgetRegistry<BaseWindow>::get_object(widget);
    if (NULL != base_window) {
      base_window->quit();
    }
  }

  static void
  view_tags_toggled_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    BaseWindow *base_window = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(checkmenuitem));
    if (base_window) {
      base_window->view_tags_toggled(checkmenuitem);
    }
  }

  static void
  view_exif_toggled_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    BaseWindow *base_window = WidgetRegistry<BaseWindow>::get_object(GTK_WIDGET(checkmenuitem));
    if (base_window) {
      base_window->view_exif_toggled(checkmenuitem);
    }
  }
    
  void connect_signals() {
    connect_signal(top_level_window, "destroy", G_CALLBACK(quit_cb), NULL);
    connect_signal(file_import_menu_item, "activate", G_CALLBACK(file_import_activate_cb), NULL);
    connect_signal(file_project_open_menu_item, "activate",
        G_CALLBACK(file_project_open_activate_cb), NULL);
    connect_signal(file_project_new_menu_item, "activate",
        G_CALLBACK(file_project_new_activate_cb), NULL);
    connect_signal(file_project_add_to_menu_item, "activate",
        G_CALLBACK(file_project_add_to_activate_cb), NULL);
    connect_signal(file_project_remove_from_menu_item, "activate",
        G_CALLBACK(file_project_remove_from_activate_cb), NULL);
    connect_signal(file_project_rename_menu_item, "activate",
        G_CALLBACK(file_project_rename_activate_cb), NULL);
    connect_signal(file_project_delete_menu_item, "activate",
        G_CALLBACK(file_project_delete_activate_cb), NULL);
    connect_signal(file_project_export_menu_item, "activate",
        G_CALLBACK(file_project_export_activate_cb), NULL);
    connect_signal(file_adjust_time_menu_item, "activate",
        G_CALLBACK(file_adjust_time_activate_cb), NULL);
    connect_signal(file_quit_menu_item, "activate", G_CALLBACK(quit_cb), NULL);
    connect_signal(edit_unselect_all_menu_item, "activate",
        G_CALLBACK(edit_unselect_all_activate_cb), NULL);
    connect_signal(edit_preferences_menu_item, "activate",
        G_CALLBACK(edit_preferences_activate_cb), NULL);
    connect_signal(edit_tags_menu_item, "activate",
        G_CALLBACK(edit_tags_activate_cb), NULL);
    connect_signal(notebook, "create-window", G_CALLBACK(create_window_cb), NULL);
    connect_signal(notebook, "page-removed", G_CALLBACK(page_removed_cb), NULL);
    connect_signal(notebook, "switch_page", G_CALLBACK(switch_page_cb), NULL);
    connect_signal(view_clone_menu_item, "activate", G_CALLBACK(clone_activate_cb), NULL);
    BOOST_FOREACH(GtkWidget *item, view_tags_menu_items) {
      connect_signal(item, "toggled", G_CALLBACK(view_tags_toggled_cb), NULL);
    }
    BOOST_FOREACH(GtkWidget *item, view_exif_menu_items) {
      connect_signal(item, "toggled", G_CALLBACK(view_exif_toggled_cb), NULL);
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
#include "AddToProjectWindow.h"
#include "RemoveFromProjectWindow.h"
#include "DeleteProjectWindow.h"
#include "ExportProjectWindow.h"
#include "AdjustTimeWindow.h"
#include "EditTagsWindow.h"
#include "RenameProjectWindow.h"
#include "PhotoSelectPage.h"

//! Find the project_name from the current notebook page.
//! Returns "" if it can't get a project name. (presumably because the notebook is empty).
inline std::string
BaseWindow::get_project_name() {
  std::string project_name("");
  gint pagenum = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  if (-1 != pagenum) {
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), pagenum);
    PhotoSelectPage *photo_select_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
    if (photo_select_page) {
      project_name = photo_select_page->get_project_name();
    }
  }
  return project_name;
}

inline void
BaseWindow::switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num,
    gpointer user_data) {
  PhotoSelectPage *new_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
  if (new_page) {
    std::string project_name = new_page->get_project_name();
    std::cout << "new page " << page_num << " " << project_name << std::endl;
    new_page->load_extra_menu_items();
  } else {
    std::cout << "page not found " << page_num << std::endl;
  }
}

inline void
BaseWindow::quit_all_notebook_pages() {
  while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 0) {
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 0);
    PhotoSelectPage *photo_select_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
    photo_select_page->quit();
  }
}

inline void
BaseWindow::file_project_open_activate() {
  if (!connection) return;
  OpenProjectWindow* openProjectWindow =
      new OpenProjectWindow(connection, thePreferences, photoFileCache, this);
  openProjectWindow->run();
  // TODO make sure that openProjectWindow gets destroyed eventually.
}

inline void
BaseWindow::file_project_new_activate() {
  if (!connection) return;
  NewProjectWindow* newProjectWindow =
      new NewProjectWindow(connection, thePreferences, photoFileCache, this);
  newProjectWindow->run();
  // TODO make sure that newProjectWindow gets destroyed eventually.
}

inline void
BaseWindow::file_project_add_to_activate() {
  if (!connection) return;
  std::string project_name = get_project_name();
  if (0 == project_name.size()) {
    return;
  }
  AddToProjectWindow* addToProjectWindow =
      new AddToProjectWindow(connection, project_name);
  addToProjectWindow->run();
  // TODO make sure that addToProjectWindow gets destroyed eventually.
}

inline void
BaseWindow::file_project_remove_from_activate() {
  if (!connection) return;
  std::string project_name = get_project_name();
  if (0 == project_name.size()) {
    return;
  }
  RemoveFromProjectWindow* removeFromProjectWindow =
      new RemoveFromProjectWindow(connection, project_name);
  removeFromProjectWindow->run();
  // TODO make sure that removeFromProjectWindow gets destroyed eventually.
}

inline void
BaseWindow::file_project_rename_activate() {
  if (!connection) return;
  RenameProjectWindow* renameProjectWindow =
      new RenameProjectWindow(connection, thePreferences, this);
  renameProjectWindow->run();
  // TODO make sure that RenameProjectWindow gets destroyed eventually.
}

inline void
BaseWindow::file_project_delete_activate() {
  if (!connection) return;
  DeleteProjectWindow* deleteProjectWindow =
      new DeleteProjectWindow(connection, thePreferences, this);
  deleteProjectWindow->run();
  // TODO make sure that deleteProjectWindow gets destroyed eventually.
}

inline void
BaseWindow::file_project_export_activate() {
  if (!connection) return;
  std::string project_name = get_project_name();
  if (0 == project_name.size()) {
    return;
  }
  ExportProjectWindow* exportProjectWindow =
      new ExportProjectWindow(connection, project_name, thePreferences, this);
  exportProjectWindow->run();
  // TODO make sure that exportProjectWindow gets destroyed eventually.
}

inline void
BaseWindow::edit_tags_activate() {
  if (!connection) return;
  std::cout << "tags_activate entered" << std::endl;
  std::string project_name = get_project_name();
  if (0 == project_name.size()) {
    return;
  }
  EditTagsWindow *edit_tags_window =
      new EditTagsWindow(connection, thePreferences, this, project_name);
  edit_tags_window->run();
// TODO make sure that edit_tags_window gets destroyed eventually.
}

inline void
BaseWindow::view_tags_toggled(GtkCheckMenuItem *checkmenuitem) {
  if (gtk_check_menu_item_get_active(checkmenuitem)) {
    gint pagenum = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (-1 == pagenum) return;
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), pagenum);
    PhotoSelectPage *photo_select_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
    if (photo_select_page) {
      std::string position = gtk_menu_item_get_label(GTK_MENU_ITEM(checkmenuitem));
      photo_select_page->set_tags_position(position);
    }
  }
}

inline void
BaseWindow::view_exif_toggled(GtkCheckMenuItem *checkmenuitem) {
  if (gtk_check_menu_item_get_active(checkmenuitem)) {
    gint pagenum = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (-1 == pagenum) return;
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), pagenum);
    PhotoSelectPage *photo_select_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
    if (photo_select_page) {
      std::string position = gtk_menu_item_get_label(GTK_MENU_ITEM(checkmenuitem));
      photo_select_page->set_exifs_position(position);
    }
  }
}

inline void
BaseWindow::clone_activate() {
  gint pagenum = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  if (-1 == pagenum) return;
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), pagenum);
  PhotoSelectPage *photo_select_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
  if (photo_select_page) {
    PhotoSelectPage *cloned_photo_select_page = photo_select_page->clone();
    add_page(cloned_photo_select_page->get_tab_label(),
        cloned_photo_select_page->get_notebook_page(),
        cloned_photo_select_page->get_project_name());
  }
}

inline void
BaseWindow::rebuild_all_tag_views() {
  gint n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
  for (gint pagenum = 0; pagenum < n_pages; pagenum++) {
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), pagenum);
    PhotoSelectPage *photo_select_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
    if (photo_select_page) {
      photo_select_page->rebuild_tag_view();
    }
  }
}

inline void
BaseWindow::edit_unselect_all_activate() {
  gint pagenum = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  if (-1 == pagenum) return;
  GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), pagenum);
  PhotoSelectPage *photo_select_page = WidgetRegistry<PhotoSelectPage>::get_object(page);
  if (photo_select_page) {
    photo_select_page->edit_unselect_all_activate();
  }
}

inline void
BaseWindow::file_adjust_time_activate() {
  if (!connection) return;

  AdjustTimeWindow* adjustTimeWindow =
      new AdjustTimeWindow(connection, thePreferences, this);
  adjustTimeWindow->run();
  // TODO make sure that adjustTimeWindow gets destroyed eventually.
}
#endif // BASEWINDOW_H__
