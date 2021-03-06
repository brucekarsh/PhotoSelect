#include "DeleteProjectWindow.h"

#include <gtk/gtk.h>
#include <fstream>
#include <iostream>
#include <boost/foreach.hpp>

#include "Db.h"
#include "WidgetRegistry.h"

using namespace std;

class Preferences;
class BaseWindow;

DeleteProjectWindow::DeleteProjectWindow(Preferences *preferences_,
    BaseWindow* baseWindow_) :
    preferences(preferences_),
    baseWindow(baseWindow_) {
}

DeleteProjectWindow::~DeleteProjectWindow() {
  WidgetRegistry<DeleteProjectWindow>::forget_widget(window);
}

void DeleteProjectWindow::quit() {
  gtk_widget_destroy(GTK_WIDGET(window));
}

void DeleteProjectWindow::run() {
  // Make a window with a vertical box (windowBox) in it.
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  WidgetRegistry<DeleteProjectWindow>::set_widget(window, this);
  windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(windowBox);
  gtk_container_add(GTK_CONTAINER(window), windowBox);

  // Make a label (title_label) and put it in windowBox
  GtkWidget *title_label = gtk_label_new("Delete Project");
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

  list<string>project_names;
  bool b = db.get_project_names_transaction(project_names);
  if (!b) {
    // TODO handle get_project_names_transaction failure
  }
  GtkWidget* radio_button;
  first_radio_button = NULL;
  BOOST_FOREACH(string project_name, project_names) {
    if (NULL == first_radio_button) {
      radio_button = gtk_radio_button_new_with_label(NULL, project_name.c_str());
      first_radio_button = radio_button;
    } else {
      radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(
          first_radio_button), project_name.c_str());
    }
    gtk_widget_show(GTK_WIDGET(radio_button));
    gtk_box_pack_start(GTK_BOX(scrolled_vbox), radio_button, FALSE, FALSE, 0);
  }

  g_signal_connect(window, "destroy", G_CALLBACK(quit_button_clicked_cb), NULL);
  g_signal_connect(quit_button, "clicked", G_CALLBACK(quit_button_clicked_cb), NULL);
  g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);

  GtkRequisition minimum_size;
  GtkRequisition natural_size;
  gtk_widget_get_preferred_size(scrolled_vbox, &minimum_size, &natural_size);
  gint width = minimum_size.width + 5;
  gint height = minimum_size.height + 5;
  const gint max_width = 400;
  const gint max_height = 400;
  if (width < natural_size.width) width = natural_size.width;
  if (height < natural_size.height) height = natural_size.height;
  if (width > max_width) width = max_width;
  if (height > max_height) height = max_height;
  gtk_widget_set_size_request(scrolled_window, width, height);

  gtk_widget_show(window);
}

string DeleteProjectWindow::get_project_name() {
  if (NULL == first_radio_button) {
    return "";
  }
  GSList *radio_buttons = gtk_radio_button_get_group(GTK_RADIO_BUTTON(first_radio_button));
  for (GSList *p = radio_buttons; p != NULL; p = g_slist_next(p)) {
    GtkRadioButton *radio_button = GTK_RADIO_BUTTON(p->data);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_button))) {
      string project_name = gtk_button_get_label(GTK_BUTTON(radio_button));
	return project_name;
    }
  }
  return "";
}
  
void DeleteProjectWindow::accept() {
  string project_name = get_project_name();
  if (0 == project_name.size()) {
    return;
  }
  bool b = db.delete_project_transaction(project_name);
  if (!b) {
    // TODO Handle db.delete_project_transaction failure
  }
  quit();
}

/* static */ void DeleteProjectWindow::accept_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  DeleteProjectWindow *deleteProjectWindow =
      WidgetRegistry<DeleteProjectWindow>::get_object(widget);
  deleteProjectWindow->accept();
}

/* static */ void DeleteProjectWindow::quit_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  DeleteProjectWindow *deleteProjectWindow =
      WidgetRegistry<DeleteProjectWindow>::get_object(widget);
  deleteProjectWindow->quit();
}
