#ifndef EDITTAGSWINDOW_H__
#define EDITTAGSWINDOW_H__
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

class EditTagsWindow {
  public:
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *first_radio_button;
  sql::Connection *connection;
  Preferences *preferences;
  BaseWindow *baseWindow;

  EditTagsWindow(sql::Connection *connection_, Preferences *preferences_,
      BaseWindow* baseWindow_) :
      connection(connection_), preferences(preferences_),
      baseWindow(baseWindow_) {
  }

  ~EditTagsWindow() {
    WindowRegistry<EditTagsWindow>::forgetWindow(window);
  }

  void accept();

  void
  quit() {
    gtk_widget_destroy(GTK_WIDGET(window));
  }

  static void
  accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    EditTagsWindow *renameProjectWindow =
        WindowRegistry<EditTagsWindow>::getWindow(widget);
    renameProjectWindow->accept();
  }

  static void
  quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    EditTagsWindow *renameProjectWindow =
        WindowRegistry<EditTagsWindow>::getWindow(widget);
    renameProjectWindow->quit();
  }

  void
  run() {
    // Make a window with a vertical box (windowBox) in it.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    WindowRegistry<EditTagsWindow>::setWindow(window, this);
    windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(windowBox);
    gtk_container_add(GTK_CONTAINER(window), windowBox);

    // Make a label (title_label) and put it in windowBox
    GtkWidget *title_label = gtk_label_new("Edit Project Tags");
    gtk_widget_show(GTK_WIDGET(title_label));
    gtk_box_pack_start(GTK_BOX(windowBox), title_label, FALSE, FALSE, 0);

    // Make an hbox to hold the lists and put it in windowBox
    GtkWidget *lists_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_show(lists_hbox);
    gtk_box_pack_start(GTK_BOX(windowBox), lists_hbox, TRUE, TRUE, 0);

    // Make a vbox to hold the left list and put it in the lists_hbox
    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(left_vbox);
    gtk_box_pack_start(GTK_BOX(lists_hbox), left_vbox, TRUE, TRUE, 0);

    // Make a vbox to hold the right list and put it in the lists_hbox
    GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(right_vbox);
    gtk_box_pack_start(GTK_BOX(lists_hbox), right_vbox, TRUE, TRUE, 0);

    // Add a title label to the left vbox
    GtkWidget *left_vbox_label = gtk_label_new("Known Tags");
    gtk_widget_show(left_vbox_label);
    gtk_box_pack_start(GTK_BOX(left_vbox), left_vbox_label, FALSE, FALSE, 0);

    // Add a title label to the right vbox
    GtkWidget *right_vbox_label = gtk_label_new("Project Tags");
    gtk_widget_show(right_vbox_label);
    gtk_box_pack_start(GTK_BOX(right_vbox), right_vbox_label, FALSE, FALSE, 0);

    // Make a ScrolledWindow (left_scrolled_window) and put it in the left_vbox
    GtkWidget *left_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(left_scrolled_window),
        GTK_SHADOW_ETCHED_IN);
    gtk_widget_show(GTK_WIDGET(left_scrolled_window));
    gtk_box_pack_start(GTK_BOX(left_vbox), left_scrolled_window, TRUE, TRUE, 0);

    // Make a ScrolledWindow (right_scrolled_window) and put it in the right_vbox
    GtkWidget *right_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(right_scrolled_window),
        GTK_SHADOW_ETCHED_OUT);
    gtk_widget_show(GTK_WIDGET(right_scrolled_window));
    gtk_box_pack_start(GTK_BOX(right_vbox), right_scrolled_window, TRUE, TRUE, 0);

#ifdef LATER
    // Make a vbox (scrolled_vbox) and put it in the scrolled_window
    GtkWidget *scrolled_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(GTK_WIDGET(scrolled_vbox));
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), scrolled_vbox);
#endif //LATER

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

    g_signal_connect(window, "destroy", G_CALLBACK(quit_button_clicked_cb), NULL);
    g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_button_clicked_cb), NULL);
    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);

#ifdef LATER
    GtkRequisition minimum_size;
    GtkRequisition natural_size;
    gtk_widget_get_preferred_size(scrolled_vbox, &minimum_size, &natural_size);
    gint width = minimum_size.width;
    gint height = minimum_size.width;
    const gint max_width = 400;
    const gint max_height = 400;
    if (width < natural_size.width) width = natural_size.width;
    if (height < natural_size.height) height = natural_size.height;
    if (width > max_width) width = max_width;
    if (height > max_height) height = max_height;
    gtk_widget_set_size_request(scrolled_window, width, height);
#endif //LATER

    gtk_widget_show(window);
  }

};

#include "BaseWindow.h"
#include "PhotoSelectPage.h"


inline  void
EditTagsWindow::accept() {
  quit();
}
#endif // EDITTAGSWINDOW_H__
