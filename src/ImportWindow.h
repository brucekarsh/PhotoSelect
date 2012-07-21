#ifndef IMPORTWINDOW_H__
#define IMPORTWINDOW_H__
#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include <stdlib.h>

/* MySQL Connector/C++ specific headers */
#include <driver.h>
#include <connection.h>
#include <statement.h>
#include <prepared_statement.h>
#include <resultset.h>
#include <metadata.h>
#include <resultset_metadata.h>
#include <exception.h>
#include <warning.h>

class ImportWindow {
  public:
  Preferences *thePreferences;
  GtkWidget *window;
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
    
    /* Load UI from file. If error occurs, report it and quit application. */
    GError *error = NULL;
    GtkBuilder* builder = gtk_builder_new();
    if( ! gtk_builder_add_from_file( builder,
        "/home/bruce/PROJECTS/NEWPHOTOSELECT/src/GladeTestProject1.glade", &error ) ) {
      g_warning( "%s", error->message );
      g_free( error );
    }
    window = GTK_WIDGET( gtk_builder_get_object( builder, "ImportDialog" ));
    progressBar = GTK_WIDGET( gtk_builder_get_object( builder, "ImportProgressBar"));
    WidgetRegistry<ImportWindow>::set_widget(window, this);

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref( G_OBJECT( builder ) );
    gtk_widget_hide(progressBar);
    gtk_widget_show(window);
  }

  bool is_cancel_requested() {
    return cancel_requested;
  }

  void import_response_cb(gint response_id) {
    if (1 == response_id) {
      // Cancel button pressed
      if (processing_imports) {
        cancel_requested = true;
      } else {
        dieDieDie();
      }
      return;
    } else if (2 == response_id) {
      // Open button pressed
      if(!processing_imports) {
        start_importing();
      }
    } else if (-4 == response_id) {
      // Window destroyed
      cancel_requested = true;
    } else {
      std::cout << "ImportWindow::import_response_cb called with invalid response_id ("
          << response_id << "), aborting\n" << std::endl;
      abort();
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
    GSList *file_list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(window));
    GSList *p;
    std::queue<std::string> dirs_to_process;
    p = file_list;

    for (p = file_list; NULL != p; p = p -> next) {
      std::cout << (char*)(p -> data) << std::endl;
      dirs_to_process.push(std::string((char*)(p -> data)));
    }
    g_slist_free_full(file_list, g_free);
    photoDbImporter.set_dirs_to_process(dirs_to_process);
    gtk_widget_show(progressBar);
    photoDbImporter.go_through_files(this);    
    processing_imports = false;
    gtk_widget_destroy(window);
    delete this;
  }

#endif  // IMPORTWINDOW_H__
