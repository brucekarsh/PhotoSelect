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

    // Make a label (title_label) and put it in windowBox
    GtkWidget *title_label = gtk_label_new("Open Project");
    gtk_widget_show(GTK_WIDGET(title_label));
    gtk_box_pack_start(GTK_BOX(windowBox), title_label, FALSE, FALSE, 0);

    // Make a ScrolledWindow (scrolled_window) and put it in the windowBox
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(GTK_WIDGET(scrolled_window));
    gtk_box_pack_start(GTK_BOX(windowBox), scrolled_window, TRUE, TRUE, 0);

    // Make a vbox (scrolled_vbox) and put it in the scrolled_window
    GtkWidget *scrolled_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(GTK_WIDGET(scrolled_vbox));
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), scrolled_vbox);

    // Make some buttons (quit_button, accept_button) and put them in an hbox (button_hbox) and
    // put the hbox in windowBox

    GtkWidget *quit_button = gtk_button_new_with_label("Quit");
    GtkWidget *accept_button = gtk_button_new_with_label("Accept");
    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(GTK_WIDGET(quit_button));
    gtk_widget_show(GTK_WIDGET(accept_button));
    gtk_widget_show(GTK_WIDGET(button_hbox));
    gtk_box_pack_start(GTK_BOX(windowBox), button_hbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(button_hbox), accept_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(button_hbox), quit_button, FALSE, FALSE, 0);

    std::string sql = "SELECT DISTINCT name FROM Project ";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    
    GtkWidget* group = NULL;
    GtkWidget* l;
    while (rs->next()) {
      std::string project_name = rs->getString(1);
      if (NULL == group) {
        l = gtk_radio_button_new_with_label(NULL, project_name.c_str());
        group = l;
      } else {
        l = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(group),
            project_name.c_str());
      }
      gtk_widget_show(GTK_WIDGET(l));
      gtk_box_pack_start(GTK_BOX(scrolled_vbox), l, FALSE, FALSE, 0);
    }

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
