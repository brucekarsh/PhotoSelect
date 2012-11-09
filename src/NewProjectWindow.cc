#include "NewProjectWindow.h"

#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <boost/bind.hpp>

#include "BaseWindow.h"
#include "PhotoSelectPage.h"
#include "SinglePhotoPage.h"
#include "WidgetRegistry.h"
#include "QueryView.h"

using namespace std;

NewProjectWindow::NewProjectWindow(Preferences *preferences_,
    PhotoFileCache *photoFileCache_, BaseWindow* baseWindow_) :
    preferences(preferences_),  photoFileCache(photoFileCache_),
    baseWindow(baseWindow_) {
}

NewProjectWindow::~NewProjectWindow() {
  WidgetRegistry<NewProjectWindow>::forget_widget(window);
}

void NewProjectWindow::quit() {
  gtk_widget_destroy(GTK_WIDGET(window));
}

void NewProjectWindow::run() {
  // Make a window with a vertical box (windowBox) in it.
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  string window_title = "Create a New Project";
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
  gtk_box_pack_start(GTK_BOX(windowBox), query_view.get_widget(), TRUE, TRUE, 0);
  accept_button = query_view.get_accept_button();
  g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);
  quit_button = query_view.get_quit_button();
  g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_button_clicked_cb), NULL);

  g_signal_connect(window, "delete-event", G_CALLBACK(quit_button_clicked_cb), NULL);
  g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);

  gtk_widget_show(window);
}

void NewProjectWindow::accept() {
  // Get the project name and validate it
  string project_name = gtk_entry_get_text(GTK_ENTRY(project_name_entry));
  if (0 == project_name.length()) {
    query_view.set_error_label("Missing project name.");
    return;
  }
  // Get the photoFilenameVector and the photFileIdList
  vector<string> photoFilenameVector;
  list<long> photoFileIdList;
  photoFilenameVector = query_view.getPhotoFilenameVector();
  photoFileIdList = query_view.getPhotoFileIdList();
  // Issue the transaction
  long project_id;
  bool b = accept_transaction(project_name, photoFilenameVector, photoFileIdList, project_id);
  // Handle failure
  if (project_id == -1) {
    query_view.set_error_label("Duplicate project name.");
  } else if (!b) {
    query_view.set_error_label("Failed adding project.");
  } else {
    // Success
    vector<string> adjusted_date_time_vector;
    bool b = Db::get_project_photo_files_transaction(project_name, photoFilenameVector,
        adjusted_date_time_vector);
    if (!b) {
      // TODO handle get_project_photo_files_transaction failure
    } 

    SinglePhotoPage *photoSelectPage = new SinglePhotoPage(photoFileCache);
    photoSelectPage->setup(photoFilenameVector, adjusted_date_time_vector, project_name,
        preferences);
    baseWindow->add_page(photoSelectPage->get_tab_label(),
        photoSelectPage->get_notebook_page(), project_name);
    quit();
  }
}

void NewProjectWindow::accept_op(const string &project_name,
    const vector<string> &photoFilenameVector,
    const list<long> &photoFileIdList, long &project_id) {
  Db::enter_operation();


  Db::insert_into_project_op(project_name, project_id);
  if (project_id == -1) {
    return;
  }
  list<long>::const_iterator id_iter = photoFileIdList.begin();
  for (vector<string>::const_iterator filename_iter = photoFilenameVector.begin();
      filename_iter != photoFilenameVector.end();
      ++filename_iter) {
    long photo_file_id = *id_iter;
    ++id_iter;
    Db::add_photo_to_project_op(project_id, photo_file_id);
  }
}

bool NewProjectWindow::accept_transaction(const string &project_name, 
    const vector<string> &photoFilenameVector, 
    const list<long> &photoFileIdList, long &project_id) {
  boost::function<void (void)> f = boost::bind(&NewProjectWindow::accept_op, this,
      boost::cref(project_name), boost::cref(photoFilenameVector),
      boost::cref(photoFileIdList), boost::ref(project_id));
  return Db::transaction(f);
}

/* static */ void NewProjectWindow::accept_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  NewProjectWindow *newProjectWindow = WidgetRegistry<NewProjectWindow>::get_object(widget);
  newProjectWindow->accept();
}

/* static */ void NewProjectWindow::quit_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  NewProjectWindow *newProjectWindow = WidgetRegistry<NewProjectWindow>::get_object(widget);
  newProjectWindow->quit();
}
