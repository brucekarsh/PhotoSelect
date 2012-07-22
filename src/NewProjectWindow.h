#ifndef NEWPROJECTWINDOW_H__
#define NEWPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>

#include "WidgetRegistry.h"
#include "QueryView.h"

class BaseWindow;
namespace sql {
  class Connection;
}

class NewProjectWindow {
  public:
  void accept();

  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *accept_button;
  GtkWidget *quit_button;
  GtkWidget *project_name_box;
  GtkWidget *project_name_label;
  GtkWidget *project_name_entry;
  sql::Connection *connection;
  QueryView query_view;
  BaseWindow *baseWindow;

  NewProjectWindow(sql::Connection *connection_, BaseWindow* baseWindow_) :
      connection(connection_), baseWindow(baseWindow_), query_view(connection_) {
  }

  ~NewProjectWindow() {
    WidgetRegistry<NewProjectWindow>::forget_widget(window);
  }

  void
  quit() {
    gtk_widget_destroy(GTK_WIDGET(window));
  }

  static void
  accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    NewProjectWindow *newProjectWindow = WidgetRegistry<NewProjectWindow>::get_object(widget);
    newProjectWindow->accept();
  }

  static void
  quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    NewProjectWindow *newProjectWindow = WidgetRegistry<NewProjectWindow>::get_object(widget);
    newProjectWindow->quit();
  }

  void
  run() {
    // Make a window with a vertical box (windowBox) in it.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    std::string window_title = "Create a New Project";
    gtk_window_set_title(GTK_WINDOW(window), window_title.c_str());   
    WidgetRegistry<NewProjectWindow>::set_widget(window, this);
    windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(windowBox);
    gtk_container_add(GTK_CONTAINER(window), windowBox);

    // Add a box (project_name_box) to windowBox
    project_name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(project_name_box);
    gtk_box_pack_start(GTK_BOX(windowBox), project_name_box, FALSE, FALSE, 0);

    // Add a label (project_name_label) to project_name_box
    project_name_label = gtk_label_new("Project name");
    gtk_widget_show(project_name_label);
    gtk_box_pack_start(GTK_BOX(project_name_box), project_name_label, FALSE, FALSE, 0);

    // Add an entry (project_name_entry) to project_name_box
    project_name_entry = gtk_entry_new();
    gtk_widget_show(project_name_entry);
    gtk_box_pack_start(GTK_BOX(project_name_box), project_name_entry, FALSE, FALSE, 0);

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
}; // end class NewProjectWindow

#include "BaseWindow.h"
#include "PhotoSelectPage.h"


inline  void
NewProjectWindow::accept() {
  // Get the project name
  std::string project_name = gtk_entry_get_text(GTK_ENTRY(project_name_entry));
  if (0 == project_name.length()) {
    query_view.set_error_label("Missing project name.");
    return;
  }
  // Insert it into the database and get its id
  long project_id = Utils::insert_into_project(connection, project_name);
  if (project_id == -1) {
    query_view.set_error_label("Duplicate project name.");
    return;
  }

  if (project_id == -1) {
    return;
  }

  std::list<std::string> photoFilenameList;
  std::list<long> photoFileIdList;
  photoFilenameList = query_view.getPhotoFilenameList();
  photoFileIdList = query_view.getPhotoFileIdList();

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
  quit();


}
#endif // NEWPROJECTWINDOW_H__
