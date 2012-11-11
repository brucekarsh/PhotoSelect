#ifndef OPENPROJECTWINDOW_H__
#define OPENPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <list>
#include <string>
#include "Db.h"

class BaseWindow;
class PhotoFileCache;
class Preferences;

class OpenProjectWindow {
  public:
    BaseWindow *baseWindow;
    Db db;
    GtkWidget *first_radio_button;
    PhotoFileCache *photoFileCache;
    std::list<long> photoFileIdList;
    Preferences *preferences;
    GtkWidget *window;
    GtkWidget *windowBox;

    OpenProjectWindow(Preferences *preferences_,
        PhotoFileCache *photoFileCache_, BaseWindow* baseWindow_);
    ~OpenProjectWindow();
    void apply();
    void quit();
    void run();
    std::string get_project_name();
    void accept_button_clicked();
    void apply_button_clicked();

    // Static member functions

    static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void apply_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
};
#endif // OPENPROJECTWINDOW_H__
