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
  GtkWidget *windowBox;
  GtkWidget *verticalBox;
  GtkWidget *scrollTextView;
  GtkWidget *scrollWindow;
  GtkWidget *scrollBox;
  GtkWidget *status_label;
  GtkWidget *accept_button;
  sql::Connection *connection;

  QueryWindow(sql::Connection *connection_) : connection(connection_) {
  }

  void
  accept() {
    std::cout << "QueryWindow::accept() called" << std::endl;
  }

  std::string
  makeQueryJSON() {
    json_spirit::Array query_rows;
    GList *rows = gtk_container_get_children(GTK_CONTAINER(verticalBox));
  
    for (GList *row = rows; row != NULL; row = row ->next) {
      if (queryWindowRows.count(GTK_WIDGET(row->data))) {
        QueryWindowRow* qwr = queryWindowRows[GTK_WIDGET(row->data)];

        const gchar *value =  gtk_entry_get_text(GTK_ENTRY(qwr->textEntryBox));
        std::cout << "value " << value << std::endl;
  
        gchar *relation = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(qwr->relationComboBox));
        std::cout << "got relation " << std::endl;
        if (relation) {
          std::cout << "relation " << relation << std::endl;
        }

        gchar *fieldName = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(qwr->fieldNameComboBox));
        std::cout << "got fieldName " << std::endl;
        if (fieldName) {
          std::cout << "fieldName " << fieldName << std::endl;
        }

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
        std::cout << "Row " 
            << object["fieldName"].get_str() << " "
            << object["relation"].get_str() << " "
            << object["value"].get_str() << std::endl;
      }
    }

    std::string first_part =
      "SELECT filePath FROM PhotoFile p "
      "INNER JOIN Checksum c ON p.checksumId = c.id "
      "INNER JOIN Time t ON t.checksumId = c.id ";

    std::string sql_statement = first_part + last_part;

    std::cout << "queryJSONToQuery statement: " << sql_statement << std::endl;
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
    while (rs->next()) {
      std::string filePath = rs->getString(1);
      gtk_text_buffer_insert_at_cursor(scrollTextBuffer, filePath.c_str(), filePath.size());
      gtk_text_buffer_insert_at_cursor(scrollTextBuffer, "\n", 1);
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

    std::cout << "queryJSONToQuery done: " << sql_statement << std::endl;
    return first_part + last_part;
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
    g_signal_connect(qwr->addButton, "clicked", G_CALLBACK(query_add_button_clicked_cb), (gpointer)qwr);
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
    std::cout << "accept_button_clicked_cb" << std::endl;
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    queryWindow->accept();
  }

  static void
  query_submit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *qwr = (QueryWindowRow*) callback_data;
    std::cout << "query_submit_button_clicked_cb" << std::endl;
    std::string queryJSON = queryWindow->makeQueryJSON();
    std::cout << queryJSON << std::endl;
    queryWindow->queryJSONToQuery(queryJSON);
  }

  static void
  query_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    std::cout << "query_add_button_clicked_cb entered" << std::endl;
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *qwr = (QueryWindowRow*) callback_data;
    int qwri = queryWindow->get_qwri(qwr);
    QueryWindowRow *new_qwr = queryWindow->make_row();
    queryWindow->queryWindowRows.insert(std::pair<GtkWidget*,
        QueryWindowRow*>(new_qwr->hbox, new_qwr));
    gtk_box_pack_start(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, FALSE, FALSE, 0);
    std::cout << "reorder to " << qwri+1 << std::endl;
    gtk_box_reorder_child(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, qwri+1);
    std::cout << "query_add_button_clicked_cb done " << qwri << std::endl;
  }

  static void
  empty_row_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    std::cout << "empty_row_add_button_clicked_cb" << std::endl;
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *new_qwr = queryWindow->make_row();
    queryWindow->queryWindowRows.insert(std::pair<GtkWidget*,
        QueryWindowRow*>(new_qwr->hbox, new_qwr));
    gtk_box_pack_start(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, FALSE, FALSE, 0);
    std::cout << "reorder to " << 1 << std::endl;
    gtk_box_reorder_child(GTK_BOX(queryWindow->verticalBox), new_qwr->hbox, 1);
    std::cout << "empty_row_add_button_clicked_cb done " << std::endl;

  }

  static void
  query_remove_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    std::cout << "query_remove_button_clicked_cb entered" << std::endl;
    QueryWindow *queryWindow = WindowRegistry::getQueryWindow(widget);
    QueryWindowRow *qwr = (QueryWindowRow*) callback_data;
      gtk_container_remove(GTK_CONTAINER(queryWindow->verticalBox), qwr->hbox);
      std::cout << "query_remove_button_clicked_cb queryWindowRows #elements =" <<
          queryWindow->queryWindowRows.size() << std::endl;
      queryWindow->queryWindowRows.erase(qwr->hbox);
      std::cout << "query_remove_button_clicked_cb queryWindowRows #elements =" <<
          queryWindow->queryWindowRows.size() << std::endl;
      if (queryWindow->queryWindowRows.empty()) {
        QueryWindowRow *qwr = queryWindow->make_row();
        queryWindow->queryWindowRows.insert(std::pair<GtkWidget*,
            QueryWindowRow*>(qwr->hbox, qwr));
        gtk_box_pack_start(GTK_BOX(queryWindow->verticalBox), qwr->hbox, FALSE, FALSE, 0);
    }
    std::cout << "query_remove_button_clicked_cb queryWindowRows #elements ="
        << queryWindow->queryWindowRows.size() << std::endl;
    std::cout << "query_remove_button_clicked_cb done " << std::endl;
  }

  void
  print_child_properties(QueryWindowRow *qwr)  {
    guint n_properties;
    GParamSpec ** params = gtk_container_class_list_child_properties(G_OBJECT_GET_CLASS(qwr->hbox), &n_properties);
    for (GParamSpec **pp = params; *pp != NULL; pp++) {
      GParamSpec *p = *pp;
      std::cout << "got one " << p->name << std::endl;
    }
    std::cout << "n_properties " << n_properties << std::endl;
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
    GtkWidget *window;
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

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(quitButton, "clicked", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(submitButton, "clicked", G_CALLBACK(query_submit_button_clicked_cb), NULL);
    g_signal_connect(accept_button, "clicked", G_CALLBACK(accept_button_clicked_cb), NULL);

    gtk_widget_show(window);
  }
}; // end class QueryWindow
#endif // QUERYWINDOW_H__
