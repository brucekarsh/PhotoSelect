#ifndef OPENPROJECTWINDOW_H__
#define OPENPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <list>
#include <string>

class BaseWindow;
class PhotoFileCache;
class Preferences;

class OpenProjectWindow {
  public:
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *first_radio_button;
  Preferences *preferences;
  BaseWindow *baseWindow;
  PhotoFileCache *photoFileCache;
  std::list<long> photoFileIdList;

  OpenProjectWindow(Preferences *preferences_,
      PhotoFileCache *photoFileCache_, BaseWindow* baseWindow_);
  ~OpenProjectWindow();
  void apply();
  void quit();
  static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  static void apply_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
  void run();
  std::string get_project_name();
  void accept_button_clicked();
  void apply_button_clicked();
};
#endif // OPENPROJECTWINDOW_H__
