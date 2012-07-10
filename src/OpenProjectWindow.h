#ifndef OPENPROJECTWINDOW_H__
#define OPENPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <json_spirit.h>

#include "WindowRegistry.h"

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

class Preferences;
class BaseWindow;

class OpenProjectWindow {
  public:
  GtkWidget *window;
  GtkWidget *windowBox;
  sql::Connection *connection;
  Preferences *preferences;
  BaseWindow *baseWindow;
  PhotoFileCache *photoFileCache;
  std::list<std::string> photoFilenameList;
  std::list<long> photoFileIdList;

  OpenProjectWindow(sql::Connection *connection_, Preferences *preferences_,
      PhotoFileCache *photoFileCache_, BaseWindow* baseWindow_) :
      connection(connection_), preferences(preferences_), photoFileCache(photoFileCache_),
      baseWindow(baseWindow_) {
  }

  void accept();
  void submit();

  void
  quit() {
    gtk_widget_destroy(GTK_WIDGET(window));
  }

  static void
  accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    OpenProjectWindow *openProjectWindow = WindowRegistry::getOpenProjectWindow(widget);
    openProjectWindow->accept();
  }

  static void
  quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    OpenProjectWindow *openProjectWindow = WindowRegistry::getOpenProjectWindow(widget);
    openProjectWindow->quit();
  }

  void
  run() {
    GtkWidget *quitButton;

    // Make a window with a vertical box (windowBox) in it.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    WindowRegistry::setOpenProjectWindow(window, this);
    windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(windowBox);
    gtk_container_add(GTK_CONTAINER(window), windowBox);

    g_signal_connect(window, "destroy", G_CALLBACK(quit_button_clicked_cb), NULL);
    g_signal_connect(quitButton, "clicked", G_CALLBACK(quit_button_clicked_cb), NULL);

    gtk_widget_show(window);
  }
};

#include "BaseWindow.h"
#include "PhotoSelectPage.h"


inline  void
OpenProjectWindow::accept() {
  quit();
}
#endif // OPENPROJECTWINDOW_H__
