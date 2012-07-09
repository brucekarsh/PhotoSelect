#ifndef QUERYWINDOW_H__
#define QUERYWINDOW_H__
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

class QueryWindow {
  public:

  class QueryWindowRow {
    public:
      GtkWidget *hbox;     // The hbox containing the row
      GtkWidget *fieldNameComboBox;
      GtkWidget *relationComboBox;
      GtkWidget *textEntryBox;
      GtkWidget *addButton;
      GtkWidget *removeButton;
  };

  std::map<GtkWidget *, QueryWindowRow*> queryWindowRows; // map from hbox to QueryWindowRow
  GtkWidget *window;
  GtkWidget *windowBox;
  GtkWidget *verticalBox;
  GtkWidget *scrollTextView;
  GtkWidget *scrollWindow;
  GtkWidget *scrollBox;
  GtkWidget *status_label;
  GtkWidget *accept_button;
  sql::Connection *connection;
  Preferences *preferences;
  BaseWindow *baseWindow;
  PhotoFileCache *photoFileCache;
  std::list<std::string> photoFilenameList;

  QueryWindow(sql::Connection *connection_, Preferences *preferences_,
      PhotoFileCache *photoFileCache_, BaseWindow* baseWindow_) :
      connection(connection_), preferences(preferences_), photoFileCache(photoFileCache_),
      baseWindow(baseWindow_) {
  }

  void accept();

  std::string
  makeQueryJSON() {
    json_spirit::Array query_rows;
    GList *rows = gtk_container_get_children(GTK_CONTAINER(verticalBox));
  
    for (GList *row = rows; row != NULL; row = row ->next) {
      if (queryWindowRows.count(GTK_WIDGET(row->data))) {
        QueryWindowRow* qwr = queryWindowRows[GTK_WIDGET(row->data)];

        const gchar *value =  gtk_entry_get_text(GTK_ENTRY(qwr->textEntryBox));
  
        gchar *relation = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox));

        gchar *fieldName = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(qwr->fieldNameComboBox));

        json_spirit::Object query_row;
        if (fieldName) {
          query_row.push_back( json_spirit::Pair( "fieldName", fieldName));
	  g_free(fieldName);
        }
        if (relation) {
          query_row.push_back( json_spirit::Pair( "relation", relation));
	  g_free(relation);
        }
        query_row.push_back( json_spirit::Pair( "value", value));
        query_rows.push_back(query_row);
      }
    }
    std::string result = json_spirit::write( query_rows, json_spirit::pretty_print );
    return result;
  }

  std::string translate_field_name(const std::string &fieldName) {
    if (fieldName == "Path") {
      return "p.filePath";
    }
    if (fieldName == "Date") {
      return "t.adjustedDateTime";
    }
    return fieldName;
  }

  std::string
  queryJSONToQuery(std::string queryJson) {
    std::vector<std::string> value_vector;
    json_spirit::mValue value;
    json_spirit::read(queryJson, value);
    json_spirit::mArray array = value.get_array();
    std::string last_part = "";
    for (json_spirit::mArray::iterator it = array.begin();
         it != array.end(); ++it) {
      json_spirit::mObject object = it->get_obj();
      if ( 0 != object.count("value") && 0 != object.count("relation")
          && 0 != object.count("fieldName")) {
        std::string fieldName_value = object["fieldName"].get_str();
        std::string relation_symbol = object["relation"].get_str();
        std::string value_value = object["value"].get_str();
        value_vector.push_back(value_value);

        // TODO This is susceptible to an sql injection. Fix before releasing.
        if (it != array.begin()) last_part.append(" AND ");
        if (it == array.begin()) last_part.append(" WHERE ");
        last_part.append(" (");
        last_part.append(translate_field_name(fieldName_value));
        last_part.append(" ");
        last_part.append(relation_symbol);
        last_part.append(" ");
        last_part.append("?");
        last_part.append(") ");
      }
    }

    last_part.append("ORDER BY t.adjustedDateTime, filePath ");

    std::string first_part =
      "SELECT DISTINCT filePath FROM PhotoFile p "
      "INNER JOIN Checksum c ON p.checksumId = c.id "
      "INNER JOIN Time t ON t.checksumId = c.id ";
      "ORDER BY t.adjustedTime, filePath ";

    std::string sql_statement = first_part + last_part;

    gtk_widget_show(scrollBox);
    sql::PreparedStatement *queryPreparedStatement = connection->prepareStatement( sql_statement);
    for (int i=0; i<value_vector.size(); i++) {
      queryPreparedStatement->setString(i+1,value_vector[i]);
    }
    GtkTextBuffer *scrollTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(scrollTextView));
    std::string txt = "\nIssuing query...\n\n";
    gtk_widget_set_sensitive(GTK_WIDGET(accept_button), FALSE);
    gtk_text_buffer_set_text(scrollTextBuffer, txt.c_str(), txt.size());
    gtk_label_set_text(GTK_LABEL(status_label), "status: Query started.");
    runUI();
    sql::ResultSet *rs = queryPreparedStatement->executeQuery();
    gtk_text_buffer_set_text(scrollTextBuffer, "", 0);
    runUI();
    int count = 0;
    int total_count = 0;
    std::string label;
    photoFilenameList.clear();
    while (rs->next()) {
      std::string filePath = rs->getString(1);
      gtk_text_buffer_insert_at_cursor(scrollTextBuffer, filePath.c_str(), filePath.size());
      gtk_text_buffer_insert_at_cursor(scrollTextBuffer, "\n", 1);
      photoFilenameList.push_back(filePath);
      count++;
      total_count++;
      if(count >= 300) {
        count = 0;
        label  = "status: Query in progress. ";
        label +=  boost::lexical_cast<std::string>(total_count);
        label += " image files so far";
        gtk_label_set_text(GTK_LABEL(status_label), label.c_str());
        runUI();
      }
    }
    label  = "status: Query finished. ";
    label +=  boost::lexical_cast<std::string>(total_count);
    label += " image files found";
    gtk_label_set_text(GTK_LABEL(status_label), label.c_str());
    gtk_widget_set_sensitive(GTK_WIDGET(accept_button), TRUE);
    runUI();

    return first_part + last_part;
  }

  void
  quit() {
    gtk_widget_destroy(GTK_WIDGET(window));
  }

  QueryWindowRow *
  make_row()
  {
    QueryWindowRow *qwr = new QueryWindowRow();
    qwr->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    qwr->fieldNameComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->fieldNameComboBox), 0, "Path");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->fieldNameComboBox), 1, "Date");
    gtk_combo_box_set_active(GTK_COMBO_BOX(qwr->fieldNameComboBox), 0);
    qwr->relationComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 0, "like");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 1, "not like");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 2, "regexp");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 3, "not regexp");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 4, "=");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 5, "<");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 6, ">");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox), 7, "!=");
    gtk_combo_box_set_active(GTK_COMBO_BOX(qwr->relationComboBox), 0);

    qwr->textEntryBox = gtk_entry_new();
    qwr->addButton = gtk_button_new_with_label("Add");
    qwr->removeButton = gtk_button_new_with_label("Remove");
    gtk_box_pack_start(GTK_BOX(qwr->hbox), qwr->fieldNameComboBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(qwr->hbox), qwr->relationComboBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(qwr->hbox), qwr->textEntryBox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(qwr->hbox), qwr->addButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(qwr->hbox), qwr->removeButton, FALSE, FALSE, 0);
    g_signal_connect(qwr->addButton, "clicked", G_CALLBACK(query_add_button_clicked_cb),
        (gpointer)qwr);
    g_signal_connect(qwr->removeButton, "clicked", G_CALLBACK(query_remove_button_clicked_cb),
        (gpointer)qwr);
    gtk_widget_show(qwr->hbox);
    gtk_widget_show(qwr->fieldNameComboBox);
    gtk_widget_show(qwr->relationComboBox);
    gtk_widget_show(qwr->textEntryBox);
    gtk_widget_show(qwr->addButton);
    gtk_widget_show(qwr->removeButton);
    return qwr;
  }

  static void
  accept_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    queryWindow->accept();
  }

  static void
  query_submit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *qwr = (QueryWindowRow*) callback_data;
    std::string queryJSON = queryWindow->makeQueryJSON();
    queryWindow->queryJSONToQuery(queryJSON);
  }

  static void
  query_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *qwr = (QueryWindowRow*) callback_data;
    int qwri = queryWindow->get_qwri(qwr);
    QueryWindowRow *new_qwr = queryWindow->make_row();
    queryWindow->queryWindowRows.insert(std::pair<GtkWidget*,
        QueryWindowRow*>(new_qwr->hbox, new_qwr));
    gtk_box_pack_start(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, FALSE, FALSE, 0);
    gtk_box_reorder_child(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, qwri+1);
  }

  static void
  empty_row_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *new_qwr = queryWindow->make_row();
    queryWindow->queryWindowRows.insert(std::pair<GtkWidget*,
        QueryWindowRow*>(new_qwr->hbox, new_qwr));
    gtk_box_pack_start(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, FALSE, FALSE, 0);
    gtk_box_reorder_child(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, 1);

  }

  static void
  query_remove_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *qwr = (QueryWindowRow*) callback_data;
      gtk_container_remove(GTK_CONTAINER(queryWindow->verticalBox), qwr->hbox);
      queryWindow->queryWindowRows.erase(qwr->hbox);
      if (queryWindow->queryWindowRows.empty()) {
        QueryWindowRow *qwr = queryWindow->make_row();
        queryWindow->queryWindowRows.insert(std::pair<GtkWidget*,
            QueryWindowRow*>(qwr->hbox, qwr));
        gtk_box_pack_start(GTK_BOX(queryWindow->verticalBox), qwr->hbox, FALSE, FALSE, 0);
    }
  }

  static void
  quit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    queryWindow->quit();
  }

  void
  print_child_properties(QueryWindowRow *qwr)  {
    guint n_properties;
    GParamSpec ** params = gtk_container_class_list_child_properties(G_OBJECT_GET_CLASS(qwr->hbox), &n_properties);
    for (GParamSpec **pp = params; *pp != NULL; pp++) {
      GParamSpec *p = *pp;
    }
  }

  int
  get_qwri(QueryWindowRow * qwr) {
    
    GValue value = {0};
    g_value_init(&value, G_TYPE_UINT);
    gtk_container_child_get_property(GTK_CONTAINER(verticalBox), GTK_WIDGET(qwr->hbox), "position", &value);
    return g_value_get_uint(&value);
  }

  void
  runUI() {
    while (gtk_events_pending ()) {
      gtk_main_iteration ();
    }
  }

  void
  run() {
    GtkWidget *submitButton;
    GtkWidget *quitButton;
    GtkWidget *buttonHBox;
    GtkWidget *empty_row_hbox;
    GtkWidget *empty_row_add_button;
    GtkWidget *status_row;

    // Make a window with a vertical box (windowBox) in it.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    WindowRegistry::setQueryWindow(window, this);
    windowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(windowBox);
    gtk_container_add(GTK_CONTAINER(window), windowBox);

    // Add a box (verticalBox) for the query rows to windowBox
    verticalBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(verticalBox);
    gtk_box_pack_start(GTK_BOX(windowBox), verticalBox, FALSE, FALSE, 0);

    // Add empty add button row to verticalBox
    empty_row_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    empty_row_add_button = gtk_button_new_with_label("Add");
    gtk_box_pack_end(GTK_BOX(empty_row_hbox), empty_row_add_button, FALSE, FALSE, 0);
    g_signal_connect(empty_row_add_button, "clicked", G_CALLBACK(empty_row_add_button_clicked_cb),
        (gpointer)0);
    gtk_widget_show(empty_row_hbox);
    gtk_widget_show(empty_row_add_button);
    gtk_box_pack_start(GTK_BOX(verticalBox), empty_row_hbox, FALSE, FALSE, 0);

    // Add query rows to verticalBox
    QueryWindowRow *qwr = make_row();
    queryWindowRows.insert(std::pair<GtkWidget*, QueryWindowRow*>(qwr->hbox, qwr));
    gtk_box_pack_start(GTK_BOX(verticalBox), qwr->hbox, FALSE, FALSE, 0);

    // Add quit and submit buttons to verticalBox
    quitButton = gtk_button_new_with_label("quit");
    submitButton = gtk_button_new_with_label("submit");
    buttonHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_end(GTK_BOX(buttonHBox), quitButton, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(buttonHBox), submitButton, FALSE, FALSE, 0);
    gtk_widget_show(quitButton);
    gtk_widget_show(submitButton);
    gtk_widget_show(buttonHBox);
    gtk_box_pack_end(GTK_BOX(verticalBox), buttonHBox, FALSE, FALSE, 0);

    // Add a box for the scrollable text (scrollBox) to windowBox
    scrollBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(windowBox), scrollBox, TRUE, TRUE, 0);

    // Add a box for status information (status_row) to scrollBox
    status_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(status_row);
    gtk_box_pack_start(GTK_BOX(scrollBox), status_row, FALSE, FALSE, 0);

    // Add a label for status to status_row
    status_label = gtk_label_new("This is the status label");
    gtk_widget_show(status_label);
    gtk_box_pack_start(GTK_BOX(status_row), status_label, FALSE, FALSE, 0);

    // Add a button (accept_button) to status_row
    accept_button = gtk_button_new_with_label("Accept");
    gtk_widget_set_sensitive(GTK_WIDGET(accept_button), FALSE);
    gtk_widget_show(accept_button);
    gtk_box_pack_end(GTK_BOX(status_row), accept_button, FALSE, FALSE, 0);

    // Add a scrollable window (scrollWindow) to scrollBox
    scrollWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrollWindow);
    gtk_widget_set_size_request(scrollWindow, 0, 400);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    //gtk_widget_show(scrollWindow);
    gtk_box_pack_start(GTK_BOX(scrollBox), scrollWindow, TRUE, TRUE, 0);

    // Add a text view (scrollTextView) to scrollWindow
    
    scrollTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(scrollTextView), FALSE);
    gtk_widget_show(scrollTextView);
    gtk_container_add(GTK_CONTAINER(scrollWindow), scrollTextView);

    g_signal_connect(window, "destroy", G_CALLBACK(quit_button_clicked_cb), NULL);
    g_signal_connect(quitButton, "clicked", G_CALLBACK(quit_button_clicked_cb), NULL);
    g_signal_connect(submitButton, "clicked", G_CALLBACK(query_submit_button_clicked_cb), NULL);
    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);

    gtk_widget_show(window);
  }
}; // end class QueryWindow

#include "BaseWindow.h"
#include "PhotoSelectPage.h"

inline  void
QueryWindow::accept() {
  PhotoSelectPage *photoSelectPage = new PhotoSelectPage(connection, photoFileCache);
  photoSelectPage->setup(photoFilenameList, preferences);
  baseWindow->add_page(photoSelectPage->get_tab_label(), photoSelectPage->get_notebook_page());
  quit();
}
#endif // QUERYWINDOW_H__
