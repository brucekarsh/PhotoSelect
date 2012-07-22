#ifndef ADDTOPROJECTWINDOW_H__
#define ADDTOPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>

#include "WidgetRegistry.h"
#include "QueryView.h"

/* MySQL Connector/C++ specific headers */
//#include <driver.h>
//#include <connection.h>
//#include <statement.h>
//#include <prepared_statement.h>
//#include <resultset.h>
//#include <metadata.h>
//#include <resultset_metadata.h>
//#include <exception.h>
//#include <warning.h>

class Preferences;
class BaseWindow;
namespace sql {
  class Connection;
}

class AddToProjectWindow {
  public:
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *accept_button;
  GtkWidget *quit_button;
  sql::Connection *connection;
  std::list<std::string> photoFilenameList;
  std::list<long> photoFileIdList;
  QueryView query_view;
  std::string project_name; // Nothing sets this yet.

  AddToProjectWindow(sql::Connection *connection_, std::string project_name_)
      : connection(connection_), project_name(project_name_), query_view(connection_) {
  }

  ~AddToProjectWindow() {
    WidgetRegistry<AddToProjectWindow>::forget_widget(window);
  }

  void accept();
  void submit();

  std::string translate_field_name(const std::string &fieldName) {
    if (fieldName == "Path") {
      return "p.filePath";
    }
    if (fieldName == "Date") {
      return "t.adjustedDateTime";
    }
    return fieldName;
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
    gtk_container_add(GTK_CONTAINER(windowBox), query_view.get_widget());
    accept_button = query_view.get_accept_button();
    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);
    quit_button = query_view.get_quit_button();
    g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_button_clicked_cb), NULL);

    g_signal_connect(window, "delete-event", G_CALLBACK(quit_button_clicked_cb), NULL);
    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);

    gtk_widget_show(window);
  }
}; // end class AddToProjectWindow

#include "BaseWindow.h"
#include "PhotoSelectPage.h"

inline  void
AddToProjectWindow::accept() {
#ifdef LATER
  // Get the project name
  if (0 == project_name.length()) {
    set_error_label("Missing project name.");
    return;
  }
  // Insert it into the database and get its id
  long project_id = Utils::insert_into_project(connection, project_name);
  if (project_id == -1) {
    set_error_label("Duplicate project name.");
  }

  if (project_id == -1) {
    connection->rollback();
    return;
  }

  // Add the filenames into the ProjectPhotoFile table
  std::list<long>::iterator id_iter = photoFileIdList.begin();
  for (std::list<std::string>::iterator filename_iter = photoFilenameList.begin();
      filename_iter != photoFilenameList.end();
      ++filename_iter) {
    long photo_file_id = *id_iter;
    ++id_iter;
    Utils::add_photo_to_project(connection, project_id, photo_file_id);
  }
  connection->commit();
#endif // LATER
  std::cout << "" << "accept() entered" << std::endl;
  quit();
}
#endif // ADDTOPROJECTWINDOW_H__
