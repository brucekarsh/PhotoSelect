#ifndef EXPORTPROJECTWINDOW_H__
#define EXPORTPROJECTWINDOW_H__
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

class ExportProjectWindow {
  public:
  GtkWidget *extra_widgets;
  GtkWidget *file_chooser;
  GtkWidget *export_all_button;
  GtkWidget *export_labeled_button;

  sql::Connection *connection;
  Preferences *preferences;
  BaseWindow *baseWindow;
  std::string project_name;
  std::list<GtkWidget *> label_buttons;

  ExportProjectWindow(sql::Connection *connection_,
      std::string project_name_,
      Preferences *preferences_,
      BaseWindow* baseWindow_) :
      connection(connection_), project_name(project_name_), preferences(preferences_),
      baseWindow(baseWindow_) {
  }

  ~ExportProjectWindow() {
    WidgetRegistry<ExportProjectWindow>::forget_widget(file_chooser);
  }

  static void
  export_all_button_toggled_cb(GtkToggleButton *togglebutton, gpointer user_data) {
    ExportProjectWindow *exportProjectWindow =
        WidgetRegistry<ExportProjectWindow>::get_object(GTK_WIDGET(togglebutton));
    exportProjectWindow->export_all_button_toggled(togglebutton, user_data);
  }

  void
  export_all_button_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean is_export_all_set = gtk_toggle_button_get_active(togglebutton);
    BOOST_FOREACH (GtkWidget *label_button, label_buttons) {
      gtk_widget_set_sensitive(label_button, !is_export_all_set);
    }
  }

  void
  run() {
    extra_widgets = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(extra_widgets);

    // Make a label (title_label) and put it in extra_widgets
    GtkWidget *title_label = gtk_label_new("Export Project");
    gtk_widget_show(GTK_WIDGET(title_label));
    gtk_box_pack_start(GTK_BOX(extra_widgets), title_label, FALSE, FALSE, 0);

    // Make some radio buttons and put them in extra_widgets
    export_all_button = gtk_radio_button_new_with_label(NULL, "all photos");
    g_signal_connect(export_all_button, "toggled", G_CALLBACK(export_all_button_toggled_cb), NULL);
    GSList *export_button_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(export_all_button));
    export_labeled_button = gtk_radio_button_new_with_label(export_button_group,
        "only those with the following labels:");
    gtk_widget_show(GTK_WIDGET(export_all_button));
    gtk_widget_show(GTK_WIDGET(export_labeled_button));
    gtk_box_pack_start(GTK_BOX(extra_widgets), export_all_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(extra_widgets), export_labeled_button, FALSE, FALSE, 0);

    std::map<std::string, Db::project_tag_s> project_tags;
    project_tags = Db::get_project_tags(connection, project_name);
    typedef std::pair<std::string, Db::project_tag_s> map_entry_t;
    BOOST_FOREACH(map_entry_t map_entry, project_tags) {
      std::string tag_name = map_entry.first;
      GtkWidget *label_button = gtk_check_button_new_with_label(tag_name.c_str());
      gtk_widget_set_margin_left(label_button, 15);
      gtk_widget_set_sensitive(label_button, false);
      gtk_box_pack_start(GTK_BOX(extra_widgets), label_button, FALSE, FALSE, 0);
      gtk_widget_show(label_button);
      label_buttons.push_back(label_button);
    }
    
    file_chooser = gtk_file_chooser_dialog_new("Output file", NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    WidgetRegistry<ExportProjectWindow>::set_widget(file_chooser, this);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(file_chooser), true);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(file_chooser), extra_widgets);
    gtk_widget_show(file_chooser);

    if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
      char *filename_p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
      std::string filename(filename_p);
      write_file(filename);
      g_free(filename_p);
    } else {
    }
    gtk_widget_destroy(file_chooser);
    delete this;
  }

  void write_file(std::string out_filename) {
    // Make a list of all the files in the project
    std::vector<std::string> photoFilenameVector;
    std::vector<std::string> adjusted_date_time_vector;
    Db::get_project_photo_files(connection, project_name, photoFilenameVector,
        adjusted_date_time_vector);

    // Make a list of all the tags, keyed by filename
    Db::all_photo_tags_map_t all_photo_tags_map =
        Db::get_all_photo_tags_for_project(connection, project_name);
    std::set<std::string> selected_tags;

    // Make a list of the selected tags
    BOOST_FOREACH (GtkWidget *widget, label_buttons) {
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        std::string tag_name = gtk_button_get_label(GTK_BUTTON(widget));
        selected_tags.insert(tag_name);
      }
    }

    // Decide if we want all files, or just those that are selected
    bool want_all_files = false;
    if (0 == label_buttons.size()) {
      want_all_files = true;
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(export_all_button))) {
      want_all_files = true;
    }

    // Open the output file
    std::ofstream outfile(out_filename, std::ios::trunc | std::ios::out);

    int index = 0;
    BOOST_FOREACH(std::string filename, photoFilenameVector) {
      bool want_this_one = true;
      if (!want_all_files) {
        BOOST_FOREACH (std::string tagname, selected_tags) {
          // Make sure that the filename is in the all_photo_tags_map. (it won't be if there are no
          // tags set on this file
          if (0 == all_photo_tags_map.count(filename)) {
            want_this_one = false;
            break;
          }
          std::map<std::string, Db::photo_tag_s> tags = all_photo_tags_map[filename];
          // Make sure the tag is set
          if (0 == tags.count(tagname)) {
            want_this_one = false;
            break;
          }
        }
      }
      if (want_this_one) {
        outfile << filename << std::endl;
      }
    }
    outfile.close();
  }
};

#endif // EXPORTPROJECTWINDOW_H__
