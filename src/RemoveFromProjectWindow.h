#ifndef REMOVEFROMPROJECTWINDOW_H__
#define REMOVEFROMPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include "QueryView.h"

class RemoveFromProjectWindow {
  public:
    Db db;
    GtkWidget *window;
    GtkWidget *windowBox;
    GtkWidget *accept_button;
    GtkWidget *quit_button;
    QueryView query_view;
    std::string project_name;

    RemoveFromProjectWindow(std::string project_name_); 
    ~RemoveFromProjectWindow(); 
    void quit(); 
    void run();
    void accept();
    void accept_op(const std::string &project_name,
        const std::vector<std::string> &photoFilenameVector,
        const std::list<long> &photoFileIdList, long &project_id);
    bool accept_transaction(const std::string &project_name,
        const std::vector<std::string> &photoFilenameVector,
        const std::list<long> &photoFileIdList, long &project_id);

    // Static member functions

    static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
}; // end class RemoveFromProjectWindow
#endif // REMOVEFROMPROJECTWINDOW_H__
