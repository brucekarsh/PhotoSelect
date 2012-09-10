#ifndef RENAMEPROJECTWINDOW_H__
#define RENAMEPROJECTWINDOW_H__
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <boost/foreach.hpp>

#include "WidgetRegistry.h"

class Preferences;
class BaseWindow;
namespace sql {
  class Connection;
}

class RenameProjectWindow {
  public:
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *error_label;
  GtkWidget *first_radio_button;
  GtkWidget *new_name_entry;
  sql::Connection *connection;
  Preferences *preferences;
  BaseWindow *baseWindow;

  RenameProjectWindow(sql::Connection *connection_, Preferences *preferences_,
      BaseWindow* baseWindow_) :
      connection(connection_), preferences(preferences_),
      baseWindow(baseWindow_) {
  }

  ~RenameProjectWindow() {
    WidgetRegistry<RenameProjectWindow>::forget_widget(window);
  }

  void accept();

  void
  quit() {
    gtk_widget_destroy(GTK_WIDGET(window));
  }

  static void
  accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    RenameProjectWindow *renameProjectWindow =
        WidgetRegistry<RenameProjectWindow>::get_object(widget);
    renameProjectWindow->accept();
  }

  static void
  quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    RenameProjectWindow *renameProjectWindow =
        WidgetRegistry<RenameProjectWindow>::get_object(widget);
    renameProjectWindow->quit();
  }

  void
  run() {
    // Make a window with a vertical box (windowBox) in it.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    WidgetRegistry<RenameProjectWindow>::set_widget(window, this);
    windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(windowBox);
    gtk_container_add(GTK_CONTAINER(window), windowBox);

    // Add a label (error_label) to windowBox
    error_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(error_label),
        "<span color=\"red\">This is the error label</span>");
    gtk_widget_hide(error_label);
    gtk_box_pack_start(GTK_BOX(windowBox), error_label, FALSE, FALSE, 0);

    // Make a label (title_label) and put it in windowBox
    GtkWidget *title_label = gtk_label_new("Rename Project");
    gtk_widget_show(GTK_WIDGET(title_label));
    gtk_box_pack_start(GTK_BOX(windowBox), title_label, FALSE, FALSE, 0);

    // Make a box, label and text entry (new_name_box, new_name_label, new_name_entry) and
    // put them in windowBox
    GtkWidget *new_name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(new_name_box);
    GtkWidget *new_name_label = gtk_label_new("New Project Name");
    gtk_widget_show(GTK_WIDGET(new_name_label));
    gtk_box_pack_start(GTK_BOX(new_name_box), new_name_label, FALSE, FALSE, 0);
    new_name_entry = gtk_entry_new();
    gtk_widget_show(GTK_WIDGET(new_name_entry));
    gtk_box_pack_start(GTK_BOX(new_name_box), new_name_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(windowBox), new_name_box, FALSE, FALSE, 0);

    // Make a label (old_name_label) and put it in windowBox
    GtkWidget *old_name_label = gtk_label_new("Current Project Name:");
    //gtk_misc_set_alignment(GTK_MISC(old_name_label), 0.0, 0.5);
    gtk_widget_set_halign(GTK_WIDGET(old_name_label), GTK_ALIGN_START);
    gtk_widget_show(old_name_label);
    gtk_box_pack_start(GTK_BOX(windowBox), old_name_label, FALSE, FALSE, 0);

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

    std::list<std::string> project_names = Db::get_project_names(connection);
    
    GtkWidget* radio_button;
    first_radio_button = NULL;
    BOOST_FOREACH(std::string project_name, project_names) {
      if (NULL == first_radio_button) {
        radio_button = gtk_radio_button_new(NULL);
        first_radio_button = radio_button;
      } else {
        radio_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(
            first_radio_button));
      }
      // Make a radio button and label packed into a vbox. We do this instead of
      // just making a radio button with a label because we want the label text to
      // be selectable.
      gtk_widget_show(GTK_WIDGET(radio_button));
      GtkWidget *radio_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_widget_show(radio_button_box);
      gtk_box_pack_start(GTK_BOX(scrolled_vbox), radio_button_box, FALSE, FALSE, 0);
      gtk_box_pack_start(GTK_BOX(radio_button_box), radio_button, FALSE, FALSE, 0);
      GtkWidget *radio_button_label = gtk_label_new(project_name.c_str());
      gtk_label_set_selectable(GTK_LABEL(radio_button_label), true);
      gtk_widget_show(radio_button_label);
      gtk_box_pack_start(GTK_BOX(radio_button_box), radio_button_label, FALSE, FALSE, 0);
    }

    // Make a hidden radio button that's initially active so that no active button appears
    // until the user makes a choice. (This is intended to lessen the chance that a user
    // inadvertently renames the first project on the list because they forgot to make a
    // choice).

    if (NULL != first_radio_button) {
      GtkWidget *hidden_radio_button =
          gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON( first_radio_button), "");
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hidden_radio_button), true);
          gtk_widget_hide(GTK_WIDGET(hidden_radio_button));
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

  std::string get_old_project_name() {
    if (NULL == first_radio_button) {
      return "";
    }
    GSList *radio_buttons = gtk_radio_button_get_group(GTK_RADIO_BUTTON(first_radio_button));
    for (GSList *p = radio_buttons; p != NULL; p = g_slist_next(p)) {
      GtkRadioButton *radio_button = GTK_RADIO_BUTTON(p->data);
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_button))) {
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(radio_button));
        if (NULL == parent) {
	  // NULL parent implies that this is the hidden button
	  return "";
        }
        GList *parent_contents = gtk_container_get_children(GTK_CONTAINER(parent));
        GtkLabel *label = GTK_LABEL(g_list_nth_data(parent_contents,1));
        std::string project_name = gtk_label_get_text(label);
	return project_name;
      }
    }
    return "";
  }

  void
  set_error_label(std::string text) {
    gtk_label_set_markup(GTK_LABEL(error_label),
        ("<span color=\"red\">" + text + "</span>").c_str());
    gtk_widget_show(GTK_WIDGET(error_label));
  }
};

inline  void
RenameProjectWindow::accept() {
  boolean error_occurred = false;
  std::string error_string;
  std::string old_project_name = get_old_project_name();
  if (0 == old_project_name.size()) {
    if (error_string.length() != 0) error_string += "\n";
    error_string += "Please select project to rename.";
    error_occurred = true;
  }

  std::string new_project_name = gtk_entry_get_text(GTK_ENTRY(new_name_entry));
  if (0 == new_project_name.size()) {
    if (error_string.length() != 0) error_string += "\n";
    error_string += "Please enter a new project name.";
    error_occurred = true;
  }

  if (error_occurred) {
    set_error_label(error_string);
    return;
  }

  bool rename_was_successful = Db::rename_project(connection,
      old_project_name, new_project_name);
  if (!rename_was_successful) {
    if (error_string.length() != 0) error_string += "\n";
    error_string += "Project name already used. Please pick a new name.";
    set_error_label(error_string);
    return;
  }

  quit();
}
#endif // RENAMEPROJECTWINDOW_H__
