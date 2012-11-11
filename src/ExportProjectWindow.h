#ifndef EXPORTPROJECTWINDOW_H__
#define EXPORTPROJECTWINDOW_H__
#include "Db.h"
#include <gtk/gtk.h>
#include <list>
#include <string>

class Preferences;
class BaseWindow;

class ExportProjectWindow {
  public:
    BaseWindow *baseWindow;
    Db db;
    GtkWidget *export_all_button;
    GtkWidget *export_labeled_button;
    GtkWidget *extra_widgets;
    GtkWidget *file_chooser;
    std::list<GtkWidget *> label_buttons;
    Preferences *preferences;
    std::string project_name;

    ExportProjectWindow( std::string project_name_, Preferences *preferences_,
        BaseWindow* baseWindow_);
    ~ExportProjectWindow();
    static void export_all_button_toggled_cb(GtkToggleButton *togglebutton, 
        gpointer user_data);
    void export_all_button_toggled(GtkToggleButton *togglebutton, 
        gpointer user_data);
    void run();
    void write_file(std::string out_filename);
};
#endif // EXPORTPROJECTWINDOW_H__
