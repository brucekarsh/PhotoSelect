#ifndef NEWPROJECTWINDOW_H__
#define NEWPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include "Db.h"
#include "QueryView.h"

class BaseWindow;
class Preferences;
class PhotoFileCache;

class NewProjectWindow {
  public:
    void accept();
    bool accept_transaction(const std::string &project_name,
      const std::vector<std::string> &photoFilenameVector,
      const std::list<long> &photoFileIdList, long &project_id);
    void accept_op(const std::string &project_name,
      const std::vector<std::string> &photoFilenameVector,
      const std::list<long> &photoFileIdList, long &project_id);

    Db db;
    GtkWidget *window;
    GtkWidget *windowBox;
    GtkWidget *accept_button;
    GtkWidget *quit_button;
    GtkWidget *project_name_box;
    GtkWidget *project_name_label;
    GtkWidget *project_name_entry;
    QueryView query_view;
    Preferences *preferences;
    BaseWindow *baseWindow;
    PhotoFileCache *photoFileCache;

    NewProjectWindow(Preferences *preferences_,
        PhotoFileCache *photoFileCache_, BaseWindow* baseWindow_);
    ~NewProjectWindow();
    void quit();
    void run();

    // Static member functions

    static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
}; // end class NewProjectWindow
#endif // NEWPROJECTWINDOW_H__
