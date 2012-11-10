#include "AdjustTimeWindow.h"

#include "WidgetRegistry.h"
#include <iostream>


class Preferences;
class BaseWindow;

using namespace std;

AdjustTimeWindow::AdjustTimeWindow( Preferences *preferences_, BaseWindow* baseWindow_) :
    preferences(preferences_), baseWindow(baseWindow_) {
}

AdjustTimeWindow::~AdjustTimeWindow() {
  if (window) {
    WidgetRegistry<AdjustTimeWindow>::forget_widget(window);
  }
}

void AdjustTimeWindow::run() {
  cout << "AdjustTimeWindow::run" << endl;
  // Make a window with a vertical box (windowBox) in it.
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  string window_title = "Adjust Time";
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
AdjustTimeWindow::quit() {
  gtk_widget_destroy(GTK_WIDGET(window));
}

void
AdjustTimeWindow::accept() {
  // Get the photoFilenameVector and the photFileIdList
  vector<string> photoFilenameVector;
  list<long> photoFileIdList;
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

void AdjustTimeWindow::accept_op(const vector<string> &photoFilenameVector,
    const list<long> &photoFileIdList, long &project_id) {
  Db::enter_operation();

  // Do something.
  list<long>::const_iterator id_iter = photoFileIdList.begin();
  for (vector<string>::const_iterator filename_iter = photoFilenameVector.begin();
      filename_iter != photoFilenameVector.end();
      ++filename_iter) {
    long photo_file_id = *id_iter;
    string photo_file_name = *filename_iter;
    ++id_iter;
    // Db::add_photo_to_project_op(project_id, photo_file_id);
    // TODO WRITEME
  }
} 

bool AdjustTimeWindow::accept_transaction(const vector<string> &photoFilenameVector,
    const list<long> &photoFileIdList, long &project_id) {
  boost::function<void (void)> f = boost::bind(&AdjustTimeWindow::accept_op, this,
      boost::cref(photoFilenameVector), boost::cref(photoFileIdList), boost::ref(project_id));
  return Db::transaction(f);
}

// Static member functions

/* static */ void AdjustTimeWindow::accept_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  AdjustTimeWindow *adjustTimeWindow = WidgetRegistry<AdjustTimeWindow>::get_object(widget);
  adjustTimeWindow->accept();
}

/* static */ void AdjustTimeWindow::quit_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  AdjustTimeWindow *adjustTimeWindow = WidgetRegistry<AdjustTimeWindow>::get_object(widget);
  adjustTimeWindow->quit();
}
