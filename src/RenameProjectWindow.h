#ifndef RENAMEPROJECTWINDOW_H__
#define RENAMEPROJECTWINDOW_H__
#include "Db.h"
#include <gtk/gtk.h>
#include <string>

class Preferences;
class BaseWindow;

class RenameProjectWindow {
  public:
  Db db;
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *error_label;
  GtkWidget *first_radio_button;
  GtkWidget *new_name_entry;
  Preferences *preferences;
  BaseWindow *baseWindow;

  RenameProjectWindow(Preferences *preferences_, BaseWindow* baseWindow_);
  ~RenameProjectWindow() ;
  void quit();
  void run();
  std::string get_old_project_name();
  void set_error_label(std::string text);
  void accept();

  // Static member functions

  static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
};
#endif // RENAMEPROJECTWINDOW_H__
