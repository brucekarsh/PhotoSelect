#ifndef IMPORTWINDOW_H__
#define IMPORTWINDOW_H__
#include <boost/lexical_cast.hpp>
#include <queue>
#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>

class Preferences;

class ImportWindow {
  public:
  Preferences *thePreferences;
  GtkWidget *window;
  GtkWidget *file_chooser;
  GtkWidget *scrollTextView;
  GtkWidget *scrollWindow;
  GtkWidget *scrollBox;
  GtkWidget *status_label;
  GtkWidget *progressBar;
  bool cancel_requested;
  bool processing_imports;
  int process_count;

  ImportWindow(Preferences* thePreferences_);
  ~ImportWindow();
  void run();
  bool is_cancel_requested();
  void cancel_button_clicked();
  void accept_button_clicked();
  void delete_event();
  void dieDieDie();
  void pulseProgressBar();
  void setProgressBar(gdouble fraction);
  void runUI(int maxtimes);
  void display_on_UI(std::string text);
  void start_importing();

  // Static member functions

  static void cancel_button_clicked_cb(GtkButton *button, gpointer user_data);
  static void accept_button_clicked_cb(GtkButton *button, gpointer user_data);
  static void delete_event_cb(GtkWidget *widget, gpointer user_data);
};
#endif // IMPORTWINDOW_H__
