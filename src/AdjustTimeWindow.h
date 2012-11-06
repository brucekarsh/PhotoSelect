#ifndef ADJUSTTIMEWINDOW_H__
#define ADJUSTTIMEWINDOW_H__
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <boost/foreach.hpp>

#include "QueryView.h"
#include "WidgetRegistry.h"

class Preferences;
class BaseWindow;
namespace sql {
  class Connection;
}

class AdjustTimeWindow {
  public:
    AdjustTimeWindow( Preferences *preferences_, BaseWindow* baseWindow_) :
        preferences(preferences_), baseWindow(baseWindow_) {
    }

    ~AdjustTimeWindow() {
      if (window) {
        WidgetRegistry<ExportProjectWindow>::forget_widget(window);
      }
    }

    void run() {
      std::cout << "AdjustTimeWindow::run" << std::endl;
      // Make a window with a vertical box (windowBox) in it.
      window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      std::string window_title = "Adjust Time";
      gtk_window_set_title(GTK_WINDOW(window), window_title.c_str());
      WidgetRegistry<AdjustTimeWindow>::set_widget(window, this);
      windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_widget_show(windowBox);
      gtk_container_add(GTK_CONTAINER(window), windowBox);

      query_view.run();
      gtk_box_pack_start(GTK_BOX(windowBox), query_view.get_widget(), TRUE, TRUE, 0);
      accept_button = query_view.get_accept_button();
      g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);
      quit_button = query_view.get_quit_button();
      g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_button_clicked_cb), NULL);

      g_signal_connect(window, "delete-event", G_CALLBACK(quit_button_clicked_cb), NULL);
      g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);

      gtk_widget_show(window);
    }

    void
    quit() {
      gtk_widget_destroy(GTK_WIDGET(window));
    }

    static void
    accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
      AdjustTimeWindow *adjustTimeWindow = WidgetRegistry<AdjustTimeWindow>::get_object(widget);
      adjustTimeWindow->accept();
    }

    static void
    quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
      AdjustTimeWindow *adjustTimeWindow = WidgetRegistry<AdjustTimeWindow>::get_object(widget);
      adjustTimeWindow->quit();
    }

    void
    accept() {
      // Get the photoFilenameVector and the photFileIdList
      std::vector<std::string> photoFilenameVector;
      std::list<long> photoFileIdList;
      photoFilenameVector = query_view.getPhotoFilenameVector();
      photoFileIdList = query_view.getPhotoFileIdList();

      // Issue the transaction
      long project_id;
      bool b = accept_transaction(photoFilenameVector, photoFileIdList, project_id);
      if (!b) {
        query_view.set_error_label("Failed adjusting time.");
      } else {
        // Success
        quit();
      }
    }

  void accept_op(const std::vector<std::string> &photoFilenameVector,
      const std::list<long> &photoFileIdList, long &project_id) {
    Db::enter_operation();

    // Do something.
    std::list<long>::const_iterator id_iter = photoFileIdList.begin();
    for (std::vector<std::string>::const_iterator filename_iter = photoFilenameVector.begin();
        filename_iter != photoFilenameVector.end();
        ++filename_iter) {
      long photo_file_id = *id_iter;
      std::string photo_file_name = *filename_iter;
      ++id_iter;
      // Db::add_photo_to_project_op(project_id, photo_file_id);
      // TODO WRITEME
    }
  } 

  bool accept_transaction(const std::vector<std::string> &photoFilenameVector,
      const std::list<long> &photoFileIdList, long &project_id) {
    boost::function<void (void)> f = boost::bind(&AdjustTimeWindow::accept_op, this,
        boost::cref(photoFilenameVector), boost::cref(photoFileIdList), boost::ref(project_id));
    return Db::transaction(f);
  }

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
