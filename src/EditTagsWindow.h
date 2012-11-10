#ifndef EDITTAGSWINDOW_H__
#define EDITTAGSWINDOW_H__
#include "Db.h"
#include <gtk/gtk.h>
#include <string>
#include <list>

class Preferences;
class BaseWindow;

class EditTagsWindow {
  public:

  Db db;
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *first_radio_button;
  GtkWidget *left_scrolled_window;
  GtkWidget *left_scrolled_vbox;
  GtkWidget *right_scrolled_vbox;
  GtkWidget *right_scrolled_window;
  GtkWidget *really_delete_known_tags_button;
  Preferences *preferences;
  BaseWindow *baseWindow;
  std::string project_name;

  EditTagsWindow(Preferences *preferences_,
      BaseWindow* baseWindow_, std::string project_name_);
  ~EditTagsWindow();
  void done();
  void run();
  void rebuild_left_scrolled_vbox();
  void rebuild_right_scrolled_vbox();
  void adjust_size(GtkWidget *scrolled_vbox, GtkWidget *scrolled_window);
  void create_tag_button_clicked(std::string tag_name);
  void delete_window();
  void add_tags_button_clicked();
  void delete_known_tags_button_clicked();
  void really_delete_known_tags_button_clicked();
  void delete_known_tags_op(const std::list<std::string> &activated_known_tags);
  bool delete_known_tags_transaction(const std::list<std::string> &activated_known_tags);
  void delete_project_tags_button_clicked();
  std::list<std::string> get_activated_known_tags();
  std::list<std::string> get_activated_project_tags();
  bool valid_tag_name(std::string tag_name);
  void rebuild_all_tag_views();

  // Static member functions

  static void done_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void create_tag_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void add_tags_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void delete_known_tags_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void really_delete_known_tags_button_clicked_cb(GtkWidget *widget,
      gpointer callback_data);
  static void delete_project_tags_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void delete_window_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);
};
#endif // EDITTAGSWINDOW_H__
