#ifndef ADDTOPROJECTWINDOW_H__
#define ADDTOPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <vector>
#include "Db.h"
#include "QueryView.h"

class AddToProjectWindow {
  public:
    Db db;
    GtkWidget *window;
    GtkWidget *windowBox;
    GtkWidget *accept_button;
    GtkWidget *quit_button;
    QueryView query_view;
    std::string project_name;

    AddToProjectWindow(std::string project_name_);
    ~AddToProjectWindow();
    void quit();
    static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    void run();
    void accept();
    void accept_op(const std::string &project_name,
        const std::vector<std::string> &photoFilenameVector,
        const std::list<long> &photoFileIdList, long &project_id);
    bool accept_transaction(const std::string &project_name,
        const std::vector<std::string> &photoFilenameVector,
        const std::list<long> &photoFileIdList, long &project_id);
}; // end class AddToProjectWindow
#endif // ADDTOPROJECTWINDOW_H__
