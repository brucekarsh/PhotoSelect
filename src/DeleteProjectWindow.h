#ifndef DELETEPROJECTWINDOW_H__
#define DELETEPROJECTWINDOW_H__
#include "Db.h"
#include <gtk/gtk.h>
#include <string>

class Preferences;
class BaseWindow;

class DeleteProjectWindow {
  public:
    Db db;
    GtkWidget *window;
    GtkWidget *windowBox;
    GtkWidget *first_radio_button;
    Preferences *preferences;
    BaseWindow *baseWindow;

    DeleteProjectWindow(Preferences *preferences_, BaseWindow* baseWindow_);
    ~DeleteProjectWindow();
    void accept();
    void quit();
    void run();
    std::string get_project_name();

    // Static member functions

    static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
};
#endif // DELETEPROJECTWINDOW_H__
