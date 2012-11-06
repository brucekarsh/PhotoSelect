#ifndef ADDTOPROJECTWINDOW_H__
#define ADDTOPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>

#include "WidgetRegistry.h"
#include "QueryView.h"

namespace sql {
  class Connection;
}

class AddToProjectWindow {
  public:
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *accept_button;
  GtkWidget *quit_button;
  QueryView query_view;
  std::string project_name;

  AddToProjectWindow(std::string project_name_)
      : project_name(project_name_) {
  }

  ~AddToProjectWindow() {
    WidgetRegistry<AddToProjectWindow>::forget_widget(window);
  }

  void
  quit() {
    gtk_widget_destroy(GTK_WIDGET(window));
  }

  static void
  accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    AddToProjectWindow *addToProjectWindow = WidgetRegistry<AddToProjectWindow>::get_object(widget);
    addToProjectWindow->accept();
  }

  static void
  quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    AddToProjectWindow *addToProjectWindow = WidgetRegistry<AddToProjectWindow>::get_object(widget);
    addToProjectWindow->quit();
  }

  void
  run() {
    // Make a window with a vertical box (windowBox) in it.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    std::string window_title = "Add To Project ";
    window_title += project_name;
    gtk_window_set_title(GTK_WINDOW(window), window_title.c_str());
    WidgetRegistry<AddToProjectWindow>::set_widget(window, this);
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
  accept() {
    // Validate the project name
    if (0 == project_name.length()) {
      query_view.set_error_label("Missing project name.");
      return;
    }

    std::vector<std::string> photoFilenameVector;
    std::list<long> photoFileIdList;
    photoFilenameVector = query_view.getPhotoFilenameVector();
    photoFileIdList = query_view.getPhotoFileIdList();
    // Issue the transaction
    long project_id;
    bool b = accept_transaction(project_name, photoFilenameVector, photoFileIdList, project_id);

    if (project_id == -1) {
      query_view.set_error_label("Missing project id.");
    } else if (!b) {
      query_view.set_error_label("Failed adding to project.");
    } else {
      quit();
    }
  }

  void accept_op(const std::string &project_name,
      const std::vector<std::string> &photoFilenameVector,
      const std::list<long> &photoFileIdList, long &project_id) {
    Db::enter_operation();
    Db::get_project_id_op(project_name, project_id);
    if (project_id == -1) {
      return;
    }

    // Add the filenames into the ProjectPhotoFile table
    std::list<long>::const_iterator id_iter = photoFileIdList.begin();
    for (std::vector<std::string>::const_iterator filename_iter = photoFilenameVector.begin();
        filename_iter != photoFilenameVector.end();
        ++filename_iter) {
      long photo_file_id = *id_iter;
      std::string photo_file_name = *filename_iter;
      ++id_iter;
      Db::add_photo_to_project_op(project_id, photo_file_id);
    }
  }

  bool accept_transaction(const std::string &project_name,
      const std::vector<std::string> &photoFilenameVector,
      const std::list<long> &photoFileIdList, long &project_id) {
    boost::function<void (void)> f = boost::bind(&AddToProjectWindow::accept_op, this,
        boost::cref(project_name), boost::cref(photoFilenameVector),
        boost::cref(photoFileIdList), boost::ref(project_id));
    return Db::transaction(f);
  }
  
}; // end class AddToProjectWindow
#endif // ADDTOPROJECTWINDOW_H__
