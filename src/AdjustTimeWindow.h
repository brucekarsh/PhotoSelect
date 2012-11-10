#ifndef ADJUSTTIMEWINDOW_H__
#define ADJUSTTIMEWINDOW_H__
#include <gtk/gtk.h>
#include "QueryView.h"

class Preferences;
class BaseWindow;

class AdjustTimeWindow {
  public:
    AdjustTimeWindow( Preferences *preferences_, BaseWindow* baseWindow_);
    ~AdjustTimeWindow();
    Db db;
    void run();
    void quit();
    void accept();
    void accept_op(const std::vector<std::string> &photoFilenameVector,
        const std::list<long> &photoFileIdList, long &project_id) ;
    bool accept_transaction(const std::vector<std::string> &photoFilenameVector,
        const std::list<long> &photoFileIdList, long &project_id);

    // Static member functions

    static void accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);

  private:
    GtkWidget *window;
    GtkWidget *windowBox;
    GtkWidget *accept_button;
    GtkWidget *quit_button;
    QueryView query_view;
    Preferences *preferences;
    BaseWindow *baseWindow;
};
#endif // ADJUSTTIMEWINDOW_H__
