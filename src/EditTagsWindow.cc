#include "EditTagsWindow.h"

#include <boost/foreach.hpp>
#include <fstream>
#include <gtk/gtk.h>
#include <iostream>
#include <set>

#include "BaseWindow.h"
#include "Db.h"
#include "WidgetRegistry.h"

class Preferences;
class BaseWindow;

using namespace std;

EditTagsWindow::EditTagsWindow(Preferences *preferences_,
    BaseWindow* baseWindow_, std::string project_name_) :
    preferences(preferences_),
    baseWindow(baseWindow_), project_name(project_name_),
    left_scrolled_vbox(NULL), right_scrolled_vbox(NULL) {
}

EditTagsWindow::~EditTagsWindow() {
  WidgetRegistry<EditTagsWindow>::forget_widget(window);
}

void EditTagsWindow::done() {
  if (NULL != window) {
    gtk_widget_destroy(GTK_WIDGET(window));
    window = 0;
    delete this;
  }
}

void EditTagsWindow::run() {
  // Make a window with a vertical box (windowBox) in it.
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  WidgetRegistry<EditTagsWindow>::set_widget(window, this);
  g_signal_connect(window, "delete-event", G_CALLBACK(delete_window_cb), NULL);
  windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(windowBox);
  gtk_container_add(GTK_CONTAINER(window), windowBox);

  // Make a label (title_label) and put it in windowBox
  std::string title = "Edit Project Tags - ";
  title += project_name;
  GtkWidget *title_label = gtk_label_new(title.c_str());
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
  right_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(right_scrolled_window),
      GTK_SHADOW_ETCHED_OUT);
  gtk_widget_show(GTK_WIDGET(right_scrolled_window));
  gtk_box_pack_start(GTK_BOX(right_vbox), right_scrolled_window, TRUE, TRUE, 0);


  rebuild_left_scrolled_vbox();
  rebuild_right_scrolled_vbox();

  // Make an entry and a button (create_tag_entry, create_tag_button), put them in an hbox
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
  g_signal_connect(add_tags_button, "clicked", G_CALLBACK(add_tags_button_clicked_cb), NULL);
  
  // Make a button (delete_global_tag_button) and put it in left_vbox
  GtkWidget *delete_known_tags_button = gtk_button_new_with_label("Delete Selected Known Tags");
  gtk_widget_show(delete_known_tags_button);
  gtk_box_pack_start(GTK_BOX(left_vbox), delete_known_tags_button, FALSE, FALSE, 0);
  g_signal_connect(delete_known_tags_button, "clicked",
      G_CALLBACK(delete_known_tags_button_clicked_cb), NULL);

  // Make a button (really_delete_global_tag_button) and put it in left_vbox
  really_delete_known_tags_button =
     gtk_button_new_with_label("Really ?? Delete Selected Known Tags?");
  gtk_box_pack_start(GTK_BOX(left_vbox), really_delete_known_tags_button, FALSE, FALSE, 0);
  g_signal_connect(really_delete_known_tags_button, "clicked",
      G_CALLBACK(really_delete_known_tags_button_clicked_cb), NULL);
  
  // Make a button (delete_project_tag_button), and put it in right_vbox
  GtkWidget *delete_project_tags_button =
      gtk_button_new_with_label("Delete Selected Project Tags");
  gtk_widget_show(delete_project_tags_button);
  gtk_box_pack_start(GTK_BOX(right_vbox), delete_project_tags_button, FALSE, FALSE, 0);
  g_signal_connect(delete_project_tags_button, "clicked",
      G_CALLBACK(delete_project_tags_button_clicked_cb), NULL);

  // Make button (done_button) and put it in an hbox (button_hbox) and put the hbox in windowBox
  GtkWidget *done_button = gtk_button_new_with_label("Done");
  GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(GTK_WIDGET(done_button));
  gtk_widget_show(GTK_WIDGET(button_hbox));
  gtk_box_pack_start(GTK_BOX(windowBox), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(button_hbox), done_button, FALSE, FALSE, 0);

  g_signal_connect(done_button, "clicked", G_CALLBACK(done_button_clicked_cb), NULL);

  adjust_size(right_scrolled_vbox, right_scrolled_window);
  adjust_size(left_scrolled_vbox, left_scrolled_window);


  gtk_widget_show(window);
}

void EditTagsWindow::rebuild_left_scrolled_vbox() {
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
  std::set<std::string> all_tags;
  db.get_all_tags_transaction(all_tags);
  BOOST_FOREACH(std::string name, all_tags) {
    // Make a button, pack it, show it and connect it.
    GtkWidget *button = gtk_check_button_new_with_label(name.c_str());
    gtk_box_pack_start(GTK_BOX(left_scrolled_vbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
  }
}

void EditTagsWindow::rebuild_right_scrolled_vbox() {
  // Destroy the left_scrolled_vbox if it already exists
  if (NULL != right_scrolled_vbox) {
    gtk_widget_destroy(right_scrolled_vbox);
  }

  // Make a vbox (right_scrolled_vbox) and put it the right_scrolled_window
  right_scrolled_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(GTK_WIDGET(right_scrolled_vbox));
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(right_scrolled_window),
      right_scrolled_vbox);

  // Put the project tags in the right_scrolled_vbox
  std::set<std::string> project_tags;
  db.get_project_tags_transaction(project_name, project_tags);
  BOOST_FOREACH(string tag_name, project_tags) {
    // Make a button, pack it, show it and connect it.
    GtkWidget *button = gtk_check_button_new_with_label(tag_name.c_str());
    gtk_box_pack_start(GTK_BOX(right_scrolled_vbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
  }
}

void EditTagsWindow::adjust_size(GtkWidget *scrolled_vbox, GtkWidget *scrolled_window) {
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

void EditTagsWindow::create_tag_button_clicked(std::string tag_name) {
  // TODO need to report invalid tag names to user
  // TODO need to scroll to duplicate tag and highlight it
  // TODO need to highlighted new tag
  if (valid_tag_name(tag_name)) {
    // Put the tag into the database
    db.insert_tag_transaction(tag_name);
    // Rebuild the all tags display in the UI
    rebuild_left_scrolled_vbox();
    // Rebuild the tag views on the notebook pages
    rebuild_all_tag_views();
  }
}

void EditTagsWindow::delete_window() {
  done();
}

void EditTagsWindow::add_tags_button_clicked() {
  std::list<std::string> activated_known_tags = get_activated_known_tags();
    BOOST_FOREACH(std::string tag_name, activated_known_tags) {
      // Put the project tag into the database
      db.insert_project_tag_transaction(tag_name, project_name);
    }
    // Rebuild the project tags display in the UI
    rebuild_right_scrolled_vbox();
    // Rebuild the all tags display in the UI (because we want them to be deactivated)
    rebuild_left_scrolled_vbox();
    // Rebuild the tag views on the notebook pages
    rebuild_all_tag_views();
}

void EditTagsWindow::delete_known_tags_button_clicked() {
  gtk_widget_show(really_delete_known_tags_button);
}

void EditTagsWindow::really_delete_known_tags_button_clicked() {
  std::list<std::string> activated_known_tags = get_activated_known_tags();
  delete_known_tags_transaction(activated_known_tags);
  // Rebuild the project tags display in the UI
  rebuild_right_scrolled_vbox();
  // Rebuild the all tags display in the UI (because we want them to be deactivated)
  rebuild_left_scrolled_vbox();
  // Rebuild the tag views on the notebook pages
  rebuild_all_tag_views();
  gtk_widget_hide(really_delete_known_tags_button);
}

void EditTagsWindow::delete_known_tags_op(const std::list<std::string> &activated_known_tags) {
  db.enter_operation();
  BOOST_FOREACH(std::string tag_name, activated_known_tags) {
    std::cout << "Delete known tag " << tag_name << std::endl;
    db.delete_known_tag_op(tag_name);
  }
}

bool EditTagsWindow::delete_known_tags_transaction(const std::list<std::string> &activated_known_tags) {
  boost::function<void (void)> f = boost::bind(&EditTagsWindow::delete_known_tags_op,
      this, boost::cref(activated_known_tags));
  return db.transaction(f);
}

void EditTagsWindow::delete_project_tags_button_clicked() {
  std::list<std::string> activated_project_tags = get_activated_project_tags();
    BOOST_FOREACH(std::string tag_name, activated_project_tags) {
      db.delete_project_tag_transaction(tag_name, project_name);
      std::cout << "Delete project tag " << tag_name << " project " << project_name << std::endl;
    }
    // Rebuild the project tags display in the UI
    rebuild_right_scrolled_vbox();
    // Rebuild the tag views on the notebook pages
    rebuild_all_tag_views();
}

std::list<std::string> EditTagsWindow::get_activated_known_tags() {
  std::list<std::string> activated_known_tags;
  GList *buttons = gtk_container_get_children(GTK_CONTAINER(left_scrolled_vbox));
  for (GList *button_entry = buttons; button_entry != NULL; button_entry = button_entry->next) {
    GtkWidget *button = GTK_WIDGET(button_entry->data);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
      std::string tag_name = gtk_button_get_label(GTK_BUTTON(button));
	activated_known_tags.push_back(tag_name);
    } else {
    }
  }
  return activated_known_tags;
}

std::list<std::string> EditTagsWindow::get_activated_project_tags() {
  std::list<std::string> activated_project_tags;
  GList *buttons = gtk_container_get_children(GTK_CONTAINER(right_scrolled_vbox));
  for (GList *button_entry = buttons; button_entry != NULL; button_entry = button_entry->next) {
    GtkWidget *button = GTK_WIDGET(button_entry->data);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
      std::string tag_name = gtk_button_get_label(GTK_BUTTON(button));
	activated_project_tags.push_back(tag_name);
    } else {
    }
  }
  return activated_project_tags;
}

bool EditTagsWindow::valid_tag_name(std::string tag_name) {
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

void EditTagsWindow::rebuild_all_tag_views() {
  static std::list<BaseWindow *> base_windows =  WidgetRegistry<BaseWindow>::get_objects();
  std::cout << "rebuild_all_tag_views got " << base_windows.size() << " BaseWindows" << std::endl;
  BOOST_FOREACH(BaseWindow *base_window, base_windows) {
    base_window->rebuild_all_tag_views();
  }
}

// Static member functions

/* static */ void EditTagsWindow::done_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  EditTagsWindow *editTagsWindow = WidgetRegistry<EditTagsWindow>::get_object(widget);
  if (editTagsWindow != 0) {
    editTagsWindow->done();
  }
}

/* static */ void EditTagsWindow::create_tag_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  EditTagsWindow *edit_tags_window = WidgetRegistry<EditTagsWindow>::get_object(widget);
  GtkWidget *create_tag_entry = GTK_WIDGET(callback_data);
  if (NULL != edit_tags_window && NULL != create_tag_entry) {
    std::string new_tag_name = gtk_entry_get_text(GTK_ENTRY(create_tag_entry));
    edit_tags_window->create_tag_button_clicked(new_tag_name);
  }
}

/* static */ void EditTagsWindow::add_tags_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  EditTagsWindow *edit_tags_window = WidgetRegistry<EditTagsWindow>::get_object(widget);
  if (NULL != edit_tags_window) {
    edit_tags_window->add_tags_button_clicked();
  }
}

/* static */ void EditTagsWindow::delete_known_tags_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  EditTagsWindow *edit_tags_window = WidgetRegistry<EditTagsWindow>::get_object(widget);
  if (NULL != edit_tags_window) {
    edit_tags_window->delete_known_tags_button_clicked();
  }
}

/* static */ void EditTagsWindow::really_delete_known_tags_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  EditTagsWindow *edit_tags_window = WidgetRegistry<EditTagsWindow>::get_object(widget);
  if (NULL != edit_tags_window) {
    edit_tags_window->really_delete_known_tags_button_clicked();
  }
}

/* static */ void EditTagsWindow::delete_project_tags_button_clicked_cb(GtkWidget *widget,
    gpointer callback_data) {
  EditTagsWindow *edit_tags_window = WidgetRegistry<EditTagsWindow>::get_object(widget);
  if (NULL != edit_tags_window) {
    edit_tags_window->delete_project_tags_button_clicked();
  }
}

/* static */ void EditTagsWindow::delete_window_cb(GtkWidget *widget, GdkEvent *event,
    gpointer user_data) {
  EditTagsWindow *edit_tags_window = WidgetRegistry<EditTagsWindow>::get_object(widget);
  if (NULL != edit_tags_window) {
    edit_tags_window->delete_window();
  }
}
