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
  GtkWidget *left_scrolled_window;
  GtkWidget *left_scrolled_vbox;
  sql::Connection *connection;
  Preferences *preferences;
  BaseWindow *baseWindow;
  std::string project_name;

  EditTagsWindow(sql::Connection *connection_, Preferences *preferences_,
      BaseWindow* baseWindow_, std::string project_name_) :
      connection(connection_), preferences(preferences_),
      baseWindow(baseWindow_), project_name(project_name_),
      left_scrolled_vbox(NULL) {
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
    EditTagsWindow *editTagsWindow =
        WindowRegistry<EditTagsWindow>::getWindow(widget);
    editTagsWindow->accept();
  }

  static void
  quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    EditTagsWindow *editTagsWindow =
        WindowRegistry<EditTagsWindow>::getWindow(widget);
    editTagsWindow->quit();
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
    left_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
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

    // Make a vbox (right_scrolled_vbox) and put it the right_scrolled_window
    GtkWidget *right_scrolled_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(GTK_WIDGET(right_scrolled_vbox));
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(right_scrolled_window),
        right_scrolled_vbox);

    // Put the project tags in the right_scrolled_vbox
    std::cout << "Getting project tags for " << project_name << std::endl;
    std::map<std::string, Utils::project_tag_s> project_tags
        = Utils::get_project_tags(connection, project_name);
    typedef std::pair<std::string, Utils::project_tag_s> map_entry_t;
std::cout << "Begin packing buttons" << std::endl;
    BOOST_FOREACH(map_entry_t map_entry, project_tags) {
      std::string name = map_entry.first;
      Utils::project_tag_s project_tag = map_entry.second;
std::cout << "Project Tag " << name << std::endl;
      // Make a button, pack it, show it and connect it.
      GtkWidget *button = gtk_check_button_new_with_label(name.c_str());
      gtk_box_pack_start(GTK_BOX(right_scrolled_vbox), button, FALSE, FALSE, 0);
      gtk_widget_show(button);
    }
std::cout << "Done packing buttons" << std::endl;

    rebuild_left_scrolled_vbox();

    // Make and entry and a button (create_tag_entry, create_tag_button), put them in an hbox
    // (create_tag_hbox) and put that in left_vbox
    GtkWidget *create_tag_entry = gtk_entry_new();
    GtkWidget *create_tag_button = gtk_button_new_with_label("Create Tag");
    g_signal_connect(create_tag_button, "clicked", G_CALLBACK(create_tag_button_clicked_cb),
        (gpointer)create_tag_entry);
    GtkWidget *create_tag_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(create_tag_entry);
    gtk_widget_show(create_tag_button);
    gtk_widget_show(create_tag_hbox);
    gtk_box_pack_start(GTK_BOX(create_tag_hbox), create_tag_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(create_tag_hbox), create_tag_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_vbox), create_tag_hbox, FALSE, FALSE, 0);

    // Make a button (add_tag_button) and put it in left_vbox
    GtkWidget *add_tags_button = gtk_button_new_with_label("Add Selected Tags To Project Tags");
    gtk_widget_show(add_tags_button);
    gtk_box_pack_start(GTK_BOX(left_vbox), add_tags_button, FALSE, FALSE, 0);
    

    // Make a button (delete_tag_button), and put it in right_vbox
    GtkWidget *delete_tags_button = gtk_button_new_with_label("Delete Selected Project Tags");
    gtk_widget_show(delete_tags_button);
    gtk_box_pack_start(GTK_BOX(right_vbox), delete_tags_button, FALSE, FALSE, 0);

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

    adjust_size(right_scrolled_vbox, right_scrolled_window);
    adjust_size(left_scrolled_vbox, left_scrolled_window);


    gtk_widget_show(window);
  }

  void rebuild_left_scrolled_vbox() {
    // Destroy the left_scrolled_vbox if it already exists
    if (NULL != left_scrolled_vbox) {
      gtk_widget_destroy(left_scrolled_vbox);
    }

    // Make a vbox (left_scrolled_vbox) and put it the left_scrolled_window
    left_scrolled_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(GTK_WIDGET(left_scrolled_vbox));
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(left_scrolled_window),
        left_scrolled_vbox);

    // Put all known tags in the left_scrolled_vbox
    std::set<std::string> all_tags = Utils::get_all_tags(connection);
    BOOST_FOREACH(std::string name, all_tags) {
      // Make a button, pack it, show it and connect it.
      GtkWidget *button = gtk_check_button_new_with_label(name.c_str());
      gtk_box_pack_start(GTK_BOX(left_scrolled_vbox), button, FALSE, FALSE, 0);
      gtk_widget_show(button);
    }
  }

  void adjust_size(GtkWidget *scrolled_vbox, GtkWidget *scrolled_window) {
    GtkRequisition minimum_size;
    GtkRequisition natural_size;
    gtk_widget_get_preferred_size(scrolled_vbox, &minimum_size, &natural_size);
    std::cout << "minimum size " << minimum_size.width << "X" << minimum_size.height << std::endl;
    std::cout << "natural size " << natural_size.width << "X" << natural_size.height << std::endl;
    gint width = minimum_size.width + 5;
    gint height = minimum_size.height + 20; // leave extra room so that a new tag will be visible
    const gint max_width = 400;
    const gint max_height = 400;
    if (width < natural_size.width) width = natural_size.width;
    if (height < natural_size.height) height = natural_size.height;
    if (width > max_width) width = max_width;
    if (height > max_height) height = max_height;
    gtk_widget_set_size_request(scrolled_window, width, height);
  }

  static void create_tag_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    EditTagsWindow *edit_tags_window = WindowRegistry<EditTagsWindow>::getWindow(widget);
    GtkWidget *create_tag_entry = GTK_WIDGET(callback_data);
    if (NULL != edit_tags_window && NULL != create_tag_entry) {
      std::string new_tag_name = gtk_entry_get_text(GTK_ENTRY(create_tag_entry));
      edit_tags_window->create_tag_button_clicked(new_tag_name);
    }
  }

  void create_tag_button_clicked(std::string tag_name) {
    // TODO need to report invalid tag names to user
    // TODO need to scroll to duplicate tag and highlight it
    // TODO need to highlighted new tag
    if (valid_tag_name(tag_name)) {
      // Put the tag into the database
      Utils::insert_tag(connection, tag_name);
      // Rebuild the all tags display in the UI
      rebuild_left_scrolled_vbox();
    }
  }

  bool valid_tag_name(std::string tag_name) {
    // Require tags to be 1 to 24 characters long.
    if (0 == tag_name.size() || 24 < tag_name.size()) {
      return false;
    }

    // Require tags to be composed of a limited set of characters
    std::string valid_tag_chars =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    size_t pos = tag_name.find_first_not_of(valid_tag_chars);
    if (std::string::npos != pos) {
      return false;
    }
    return true;
  }

};

#include "BaseWindow.h"
#include "PhotoSelectPage.h"


inline  void
EditTagsWindow::accept() {
  quit();
}
#endif // EDITTAGSWINDOW_H__
