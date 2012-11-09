#include "ImportWindow.h"

#include <gtk/gtk.h>
#include <boost/lexical_cast.hpp>
#include <queue>
#include <stdlib.h>
#include "PhotoDbImporter.h"
#include "Preferences.h"
#include "WidgetRegistry.h"

using namespace std;

ImportWindow::ImportWindow(Preferences* thePreferences_) :
    thePreferences( thePreferences_), cancel_requested(false), processing_imports(false) {
}

ImportWindow::~ImportWindow() {
  WidgetRegistry<ImportWindow>::forget_widget(window);
}

void ImportWindow::run() {
  // Make a window with a vertical box (windowBox) in it.
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(window, 600, 600);
  string window_title = "Create a New Project";
  gtk_window_set_title(GTK_WINDOW(window), window_title.c_str());
  WidgetRegistry<ImportWindow>::set_widget(window, this);
  GtkWidget *windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(windowBox);
  gtk_container_add(GTK_CONTAINER(window), windowBox);

  // Make a GtkFileChooserWidget (file_chooser) and put it in windowBox

  file_chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_box_pack_start(GTK_BOX(windowBox), file_chooser, TRUE, TRUE, 0);
  gtk_widget_show(file_chooser);

  // Make a progress bar (progressBar) and put it in the windowBox
  progressBar = gtk_progress_bar_new();
  gtk_widget_show(progressBar);
  gtk_box_pack_start(GTK_BOX(windowBox), progressBar, FALSE, FALSE, 0);

  // Make an hbox for buttons (button_hbox) and put it in the windowbox
  GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(button_hbox);
  gtk_box_pack_start(GTK_BOX(windowBox), button_hbox, FALSE, FALSE, 0);

  // Put Cancel and Accept buttons in button_hbox
  GtkWidget *cancel_button = gtk_button_new_with_label("Quit");
  gtk_widget_show(cancel_button);
  gtk_box_pack_end(GTK_BOX(button_hbox), cancel_button, FALSE, FALSE, 0);
  GtkWidget *accept_button = gtk_button_new_with_label("Accept");
  gtk_widget_show(accept_button);
  gtk_box_pack_end(GTK_BOX(button_hbox), accept_button, FALSE, FALSE, 0);

  // Add a box for the scrollable text (scrollBox) to windowBox
  scrollBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start(GTK_BOX(windowBox), scrollBox, TRUE, TRUE, 0);
  gtk_widget_hide(scrollBox);

  // Add a box for status information (status_row) to scrollBox
  GtkWidget *status_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(status_row);
  gtk_box_pack_start(GTK_BOX(scrollBox), status_row, FALSE, FALSE, 0);

  // Add a label for status (status_label) to status_row
  status_label = gtk_label_new("This is the status label");
  gtk_widget_show(status_label);
  gtk_box_pack_start(GTK_BOX(status_row), status_label, FALSE, FALSE, 0);

  // Add a scrollable window (scrollWindow) to scrollBox
  scrollWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_size_request(scrollWindow, 0, 300);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWindow),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrollWindow);
  gtk_box_pack_start(GTK_BOX(scrollBox), scrollWindow, TRUE, TRUE, 0);

  // Add a text view (scrollTextView) to scrollWindow
  scrollTextView = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(scrollTextView), FALSE);
  gtk_widget_show(scrollTextView);
  gtk_container_add(GTK_CONTAINER(scrollWindow), scrollTextView);

  g_signal_connect(cancel_button, "clicked", G_CALLBACK(cancel_button_clicked_cb), NULL);
  g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);
  g_signal_connect(window, "delete-event", G_CALLBACK(delete_event_cb), NULL);

  WidgetRegistry<ImportWindow>::set_widget(window, this);

  gtk_widget_show(window);
}

bool ImportWindow::is_cancel_requested() {
  return cancel_requested;
}

void ImportWindow::cancel_button_clicked() {
  if (processing_imports) {
    cancel_requested = true;
  } else {
    dieDieDie();
  }
}

void ImportWindow::accept_button_clicked() {
  if(!processing_imports) {
    start_importing();
  }
}

void ImportWindow::delete_event() {
  cout << "delete_event" << endl;
  if (processing_imports) {
    cancel_requested = true;
  } else {
    dieDieDie();
  }
}

void ImportWindow::dieDieDie() {
    gtk_widget_destroy(window);
    delete this;
}

void ImportWindow::pulseProgressBar() {
  if (progressBar) {
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressBar));
  }
}

void ImportWindow::setProgressBar(gdouble fraction) {
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), fraction);
}

void ImportWindow::runUI(int maxtimes) {
  while (gtk_events_pending() && maxtimes-- > 0) {
    gtk_main_iteration ();
  } 
}

void ImportWindow::display_on_UI(string text) {
  GtkTextBuffer *scrollTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(scrollTextView));
  gtk_text_buffer_insert_at_cursor(scrollTextBuffer, (text + "\n").c_str(), 1+text.size());
  gtk_label_set_text(GTK_LABEL(status_label), (string("status: processed ")
      + boost::lexical_cast<string>(process_count) + "\n").c_str());
  process_count++;
  runUI(100);
}

void ImportWindow::start_importing() {
  PhotoDbImporter photoDbImporter;
  processing_imports = true;
  string dbhost = thePreferences -> get_dbhost();
  string user = thePreferences -> get_user();
  string password = thePreferences -> get_password();
  string database = thePreferences -> get_database();
  GSList *file_list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(file_chooser));
  GSList *p;
  queue<string> dirs_to_process;
  p = file_list;
  process_count = 0;

  // Indicate in the UI that we are starting
  GtkTextBuffer *scrollTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(scrollTextView));
  string txt = "Starting...\n";
  gtk_text_buffer_set_text(scrollTextBuffer, txt.c_str(), -1);
  gtk_label_set_text(GTK_LABEL(status_label), "status: Starting.");
  gtk_widget_show(scrollBox);
  runUI(100);

  for (p = file_list; NULL != p; p = p -> next) {
    cout << (char*)(p -> data) << endl;
    dirs_to_process.push(string((char*)(p -> data)));
    if (cancel_requested) break;
  }

  if (!cancel_requested) {
    g_slist_free_full(file_list, g_free);
    photoDbImporter.set_dirs_to_process(dirs_to_process);
    gtk_widget_show(progressBar);
    photoDbImporter.go_through_files(this);    
  }
  dieDieDie();
}

/* static */ void ImportWindow::cancel_button_clicked_cb(GtkButton *button,
    gpointer user_data) {
  ImportWindow* importWindow = WidgetRegistry<ImportWindow>::get_object(GTK_WIDGET(button));
  if (importWindow) {
    importWindow->cancel_button_clicked();
  }
}

/* static */ void ImportWindow::accept_button_clicked_cb(GtkButton *button,
    gpointer user_data) {
  ImportWindow* importWindow = WidgetRegistry<ImportWindow>::get_object(GTK_WIDGET(button));
  if (importWindow) {
    importWindow->accept_button_clicked();
  }
}

/* static */ void ImportWindow::delete_event_cb(GtkWidget *widget, gpointer user_data) {
  ImportWindow* importWindow = WidgetRegistry<ImportWindow>::get_object(widget);
  if (importWindow) {
    importWindow->delete_event();
  }
}
