#ifndef IMPORTWINDOW_H__
#define IMPORTWINDOW_H__
#include <queue>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
#include <stdlib.h>

namespace sql {
  class Connection;
}

class ImportWindow {
  public:
  Preferences *thePreferences;
  GtkWidget *window;
  GtkWidget *file_chooser;
  GtkWidget *progressBar;
  bool cancel_requested;
  bool processing_imports;
  sql::Connection *connection;

  ImportWindow(Preferences* thePreferences_, sql::Connection *connection_) :
      thePreferences( thePreferences_), cancel_requested(false), processing_imports(false), connection(connection_) {
  }

  ~ImportWindow() {
    WidgetRegistry<ImportWindow>::forget_widget(window);
  }

  void run() {
    // Make a window with a vertical box (windowBox) in it.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window, 600, 600);
    std::string window_title = "Create a New Project";
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

    g_signal_connect(cancel_button, "clicked", G_CALLBACK(cancel_button_clicked_cb), NULL);
    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(delete_event_cb), NULL);

    WidgetRegistry<ImportWindow>::set_widget(window, this);

    gtk_widget_show(window);
  }

  bool is_cancel_requested() {
    return cancel_requested;
  }

  static void cancel_button_clicked_cb(GtkButton *button, gpointer user_data) {
    ImportWindow* importWindow = WidgetRegistry<ImportWindow>::get_object(GTK_WIDGET(button));
    if (importWindow) {
      importWindow->cancel_button_clicked();
    }
  }

  static void accept_button_clicked_cb(GtkButton *button, gpointer user_data) {
    ImportWindow* importWindow = WidgetRegistry<ImportWindow>::get_object(GTK_WIDGET(button));
    if (importWindow) {
      importWindow->accept_button_clicked();
    }
  }

  static void delete_event_cb(GtkWidget *widget, gpointer user_data) {
    ImportWindow* importWindow = WidgetRegistry<ImportWindow>::get_object(widget);
    if (importWindow) {
      importWindow->delete_event();
    }
  }

  void
  cancel_button_clicked() {
    if (processing_imports) {
      cancel_requested = true;
    } else {
      dieDieDie();
    }
  }

  void
  accept_button_clicked() {
    if(!processing_imports) {
      start_importing();
    }
  }

  void
  delete_event() {
    std::cout << "delete_event" << std::endl;
    if (processing_imports) {
      cancel_requested = true;
    } else {
      dieDieDie();
    }
  }

  void dieDieDie() {
      gtk_widget_destroy(window);
      delete this;
  }

  void
  pulseProgressBar() {
    if (progressBar) {
      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressBar));
    }
  }

  void
  setProgressBar(gdouble fraction) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), fraction);
  }

  void
  runUI() {
    while (gtk_events_pending ()) {
      gtk_main_iteration ();
    } 
  }

  void start_importing();

  void display_on_UI(std::string text) {
  }
};

#include "PhotoDbImporter.h"

  void inline
  ImportWindow::start_importing() {
    PhotoDbImporter photoDbImporter(connection);
    processing_imports = true;
    std::string dbhost = thePreferences -> get_dbhost();
    std::string user = thePreferences -> get_user();
    std::string password = thePreferences -> get_password();
    std::string database = thePreferences -> get_database();
    GSList *file_list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(file_chooser));
    GSList *p;
    std::queue<std::string> dirs_to_process;
    p = file_list;

    for (p = file_list; NULL != p; p = p -> next) {
      std::cout << (char*)(p -> data) << std::endl;
      dirs_to_process.push(std::string((char*)(p -> data)));
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

#endif  // IMPORTWINDOW_H__
