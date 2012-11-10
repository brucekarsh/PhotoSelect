#ifndef BASEWINDOW_H__
#define BASEWINDOW_H__

#include <gtk/gtk.h>
#include <string>
#include <list>
#include <map>
#include <vector>

class PhotoFileCache;
class PreferencesWindow;
class Preferences;

class BaseWindow {
  public:
    //! Holds menu items to be placed in the BaseWindow menus when PhotoSelectWindows are
    //! switched in
    class ExtraMenuItem {
      public:
        std::string item_location; // the name of the place where the menus item is to be placed
        GtkWidget *menu_item;      // should hold only a GtkMenuItem*
    };
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

    // map of names of menu locations (see class ExtraMenuItem) to their GtkMenu
    std::map<std::string, GtkWidget *> extra_menu_item_location_map;

    // a vector of the ExtraMenuItems that we've added for the current PhotoSelectPage
    std::vector<ExtraMenuItem> current_extra_menu_items;

    Preferences* thePreferences;
    std::list<guint> signal_handler_ids;
    std::list<gpointer> signal_instances;

  public:
    BaseWindow(Preferences *thePreferences, PhotoFileCache *photoFileCache_);
    ~BaseWindow();

    //! Called from PhotoSelectWindows when they are switched it to cause them to add menu items
    //! to the BaseWindow menus. The old ExtraMenuItems should already have been removed
    //! before this is called.
    //! \param extra_menu_items a list of the menu names and their items to be inserted
    void add_extra_menu_items(std::vector<ExtraMenuItem> extra_menu_items);
    void remove_previous_extra_menu_items();
    void add_page(GtkWidget* label, GtkWidget* page, std::string project_name);
    void remove_page(GtkWidget *page);
    void run();
    void run(GtkWidget *notebook_, const GtkWindowPosition &window_position, gint width,
        gint height);
    void build_window(const GtkWindowPosition &window_position, gint width, gint height);
    void build_menus();
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

    // Static member functions

    static void file_project_open_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void file_project_new_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void file_project_add_to_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void file_project_remove_from_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void file_project_rename_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void file_project_delete_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void file_project_export_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void edit_unselect_all_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void edit_preferences_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void edit_tags_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    void preferences_activate();
    static void file_adjust_time_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    void file_import_activate();
    static void file_import_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    static void clone_activate_cb(GtkMenuItem *menuItem, gpointer user_data);
    void preferences_window_destroy();
    static void preferences_window_destroy_cb(GtkWidget *widget, gpointer user_data);
    static GtkNotebook * create_window_cb(
        GtkNotebook *notebook, GtkWidget *page, gint x, gint y, gpointer user_data);
    void page_removed(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data);
    static void page_removed_cb(
        GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data);
    static void switch_page_cb(
        GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);
    GtkNotebook * create_window(
        GtkNotebook *notebook, GtkWidget *page, gint x, gint y, gpointer user_data);
    void quit();
    static void quit_cb(GtkWidget *widget, gpointer user_data);
    static void view_tags_toggled_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data);
    static void view_exif_toggled_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data);
    void connect_signals();
    void connect_signal(
        gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data);
    void disconnect_signals();
};
#endif // BASEWINDOW_H__
