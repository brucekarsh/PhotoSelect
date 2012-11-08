#include "QueryView.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "WidgetRegistry.h"
//TODO Don't forget a WidgetRegistry set_object


#include <json_spirit.h>
#include <json_spirit_reader_template.h>
#include <json_spirit_writer_template.h>

using namespace std;

string QueryView::queryJSONToSqlQueryString(
    const string &queryJson, vector<string> &value_vector) {
  json_spirit::mValue value;
  json_spirit::read(queryJson, value);
  json_spirit::mArray array;
  array = value.get_array();
  string last_part = "";
  for (json_spirit::mArray::iterator it = array.begin();
       it != array.end(); ++it) {
    json_spirit::mObject object = it->get_obj();
    if ( 0 != object.count("value") && 0 != object.count("relation")
        && 0 != object.count("fieldName")) {
      string fieldName_value = object["fieldName"].get_str();
      string relation_symbol = object["relation"].get_str();
      string value_value = object["value"].get_str();
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
  if (is_limited_to_a_project) {
    last_part.append("AND (pr.name = ?) ");
  }

  last_part.append("ORDER BY t.adjustedDateTime, filePath ");

  string first_part =
    "SELECT DISTINCT filePath, p.id FROM PhotoFile p "
    "INNER JOIN Checksum c ON p.checksumId = c.id "
    "INNER JOIN Time t ON t.checksumId = c.id ";

  if (is_limited_to_a_project) {
    first_part.append(
        "INNER JOIN ProjectPhotoFile pf ON pf.photoFileId = p.id "
        "INNER JOIN Project pr on pr.id = pf.projectId ");
  }

  string sql_statement = first_part + last_part;
  return sql_statement;
}

string
QueryView::makeQueryJSON() {
  json_spirit::Array query_rows;
  GList *rows = gtk_container_get_children(GTK_CONTAINER(verticalBox));

  for (GList *row = rows; row != NULL; row = row ->next) {
    if (queryViewRows.count(GTK_WIDGET(row->data))) {
      QueryViewRow* queryViewRow = queryViewRows[GTK_WIDGET(row->data)];

      const gchar *value =  gtk_entry_get_text(GTK_ENTRY(queryViewRow->textEntryBox));

      gchar *relation = gtk_combo_box_text_get_active_text(
          GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox));

      gchar *fieldName = gtk_combo_box_text_get_active_text(
          GTK_COMBO_BOX_TEXT(queryViewRow->fieldNameComboBox));

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
  string result = json_spirit::write( query_rows, json_spirit::pretty_print );
  return result;
}


  QueryView::QueryView() :
      is_limited_to_a_project(false), limit_project_name("") {
  }

  QueryView::~QueryView() {
    if (NULL != queryViewBox) {
      WidgetRegistry<QueryView>::forget_widget(queryViewBox);
    }
  }

  GtkWidget *QueryView::get_widget() { return queryViewBox; }
  GtkWidget *QueryView::get_accept_button() { return accept_button; }
  GtkWidget *QueryView::get_quit_button() { return quitButton; }
  const vector<std::string> &QueryView::getPhotoFilenameVector() {return photoFilenameVector;};
  const std::list<long> &QueryView::getPhotoFileIdList() {return photoFileIdList;};
  void QueryView::limit_to_a_project(std::string name) {
    limit_project_name = name;
    is_limited_to_a_project = true;
  }


  std::string QueryView::translate_field_name(const std::string &fieldName) {
    if (fieldName == "Path") {
      return "p.filePath";
    }
    if (fieldName == "Date") {
      return "t.adjustedDateTime";
    }
    return fieldName;
  }

  bool QueryView::query_transaction(const std::string &sql_query_string,
      const std::vector<std::string> &value_vector) {
    boost::function<void (void)> f = boost::bind(&QueryView::query_op, this,
        boost::cref(sql_query_string), boost::cref(value_vector));
    return Db::transaction(f);
  }

  void QueryView::query_op(const std::string &sql_query_string, const std::vector<std::string> &value_vector) {
    Db::enter_operation();
    sql::Connection *connection = Db::get_connection();

    gtk_widget_show(scrollBox);
    GtkTextBuffer *scrollTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(scrollTextView));
    std::string txt = "\nIssuing query...\n\n";
    gtk_widget_set_sensitive(GTK_WIDGET(accept_button), FALSE);
    gtk_text_buffer_set_text(scrollTextBuffer, txt.c_str(), txt.size());
    gtk_label_set_text(GTK_LABEL(status_label), "status: Query started.");
    runUI(100);
    sql::PreparedStatement *queryPreparedStatement = connection->prepareStatement(sql_query_string);
    int i;
    for (i=0; i<value_vector.size(); i++) {
      queryPreparedStatement->setString(i+1,value_vector[i]);
    }
    if (is_limited_to_a_project) {
      queryPreparedStatement->setString(i+1, limit_project_name);
    }
    sql::ResultSet *rs = queryPreparedStatement->executeQuery();
    gtk_text_buffer_set_text(scrollTextBuffer, "", 0);
    runUI(100);
    std::string label;
    photoFilenameVector.clear();
    int count = 0;
    int total_count = 0;
    while (rs->next()) {
      std::string filePath = rs->getString(1);
      long id = rs->getInt64(2);
      photoFilenameVector.push_back(filePath);
      photoFileIdList.push_back(id);
      gtk_text_buffer_insert_at_cursor(scrollTextBuffer, filePath.c_str(), filePath.size());
      gtk_text_buffer_insert_at_cursor(scrollTextBuffer, "\n", 1);
      count++;
      total_count++;
      if(count >= 300) {
        label  = "status: Query in progress. ";
        label +=  boost::lexical_cast<std::string>(total_count);
        label += " image files so far";
        gtk_label_set_text(GTK_LABEL(status_label), label.c_str());
        runUI(100);
        count = 0;
      }
    }
    label  = "status: Query finished. ";
    label +=  boost::lexical_cast<std::string>(total_count);
    label += " image files found";
    gtk_label_set_text(GTK_LABEL(status_label), label.c_str());
    gtk_widget_set_sensitive(GTK_WIDGET(accept_button), TRUE);
    runUI(100);
  }
  
  QueryView::QueryViewRow *
  QueryView::make_row()
  {
    QueryViewRow *queryViewRow = new QueryViewRow();
    queryViewRow->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    queryViewRow->fieldNameComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->fieldNameComboBox), 0, "Path");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->fieldNameComboBox), 1, "Date");
    gtk_combo_box_set_active(GTK_COMBO_BOX(queryViewRow->fieldNameComboBox), 0);
    queryViewRow->relationComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 0, "like");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 1, "not like");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 2, "regexp");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 3, "not regexp");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 4, "=");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 5, "<");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 6, ">");
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(queryViewRow->relationComboBox), 7, "!=");
    gtk_combo_box_set_active(GTK_COMBO_BOX(queryViewRow->relationComboBox), 0);

    queryViewRow->textEntryBox = gtk_entry_new();
    queryViewRow->addButton = gtk_button_new_with_label("Add");
    queryViewRow->removeButton = gtk_button_new_with_label("Remove");
    gtk_box_pack_start(GTK_BOX(queryViewRow->hbox), queryViewRow->fieldNameComboBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(queryViewRow->hbox), queryViewRow->relationComboBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(queryViewRow->hbox), queryViewRow->textEntryBox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(queryViewRow->hbox), queryViewRow->addButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(queryViewRow->hbox), queryViewRow->removeButton, FALSE, FALSE, 0);
    g_signal_connect(queryViewRow->addButton, "clicked", G_CALLBACK(query_add_button_clicked_cb),
        (gpointer)queryViewRow);
    g_signal_connect(queryViewRow->removeButton, "clicked", G_CALLBACK(query_remove_button_clicked_cb),
        (gpointer)queryViewRow);
    gtk_widget_show(queryViewRow->hbox);
    gtk_widget_show(queryViewRow->fieldNameComboBox);
    gtk_widget_show(queryViewRow->relationComboBox);
    gtk_widget_show(queryViewRow->textEntryBox);
    gtk_widget_show(queryViewRow->addButton);
    gtk_widget_show(queryViewRow->removeButton);
    return queryViewRow;
  }

  /* static */ void
  QueryView::query_submit_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryView *queryView = WidgetRegistry<QueryView>::get_object(widget);
    queryView->submit();
  }

  /* static */ void
  QueryView::query_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryView *queryView = WidgetRegistry<QueryView>::get_object(widget);
    QueryViewRow *queryViewRow = (QueryViewRow*) callback_data;
    int queryViewRowi = queryView->get_queryViewRowi(queryViewRow);
    QueryViewRow *new_queryViewRow = queryView->make_row();
    queryView->queryViewRows.insert(std::pair<GtkWidget*,
        QueryViewRow*>(new_queryViewRow->hbox, new_queryViewRow));
    gtk_box_pack_start(GTK_BOX(queryView->verticalBox), new_queryViewRow->hbox, FALSE, FALSE, 0);
    gtk_box_reorder_child(GTK_BOX(queryView->verticalBox), new_queryViewRow->hbox, queryViewRowi+1);
  }

  /* static */ void
  QueryView::empty_row_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryView *queryView = WidgetRegistry<QueryView>::get_object(widget);
    QueryViewRow *new_queryViewRow = queryView->make_row();
    queryView->queryViewRows.insert(std::pair<GtkWidget*,
        QueryViewRow*>(new_queryViewRow->hbox, new_queryViewRow));
    gtk_box_pack_start(GTK_BOX(queryView->verticalBox), new_queryViewRow->hbox, FALSE, FALSE, 0);
    gtk_box_reorder_child(GTK_BOX(queryView->verticalBox), new_queryViewRow->hbox, 1);
  }

  /* static */ void
  QueryView::query_remove_button_clicked_cb(GtkWidget *widget, gpointer callback_data) {
    QueryView *queryView = WidgetRegistry<QueryView>::get_object(widget);
    QueryViewRow *queryViewRow = (QueryViewRow*) callback_data;
      gtk_container_remove(GTK_CONTAINER(queryView->verticalBox), queryViewRow->hbox);
      queryView->queryViewRows.erase(queryViewRow->hbox);
      if (queryView->queryViewRows.empty()) {
        QueryViewRow *queryViewRow = queryView->make_row();
        queryView->queryViewRows.insert(std::pair<GtkWidget*,
            QueryViewRow*>(queryViewRow->hbox, queryViewRow));
        gtk_box_pack_start(GTK_BOX(queryView->verticalBox), queryViewRow->hbox, FALSE, FALSE, 0);
    }
  }

  void
  QueryView::print_child_properties(QueryViewRow *queryViewRow)  {
    guint n_properties;
    GParamSpec ** params = gtk_container_class_list_child_properties(
        G_OBJECT_GET_CLASS(queryViewRow->hbox), &n_properties);
    for (GParamSpec **pp = params; *pp != NULL; pp++) {
      GParamSpec *p = *pp;
    }
  }

  int
  QueryView::get_queryViewRowi(QueryViewRow * queryViewRow) {
    
    GValue value = {0};
    g_value_init(&value, G_TYPE_UINT);
    gtk_container_child_get_property(GTK_CONTAINER(verticalBox),
        GTK_WIDGET(queryViewRow->hbox), "position", &value);
    return g_value_get_uint(&value);
  }

  void
  QueryView::runUI(int maxtimes) {
    while (gtk_events_pending() && maxtimes-- > 0) {
      gtk_main_iteration ();
    }
  }

  void
  QueryView::run() {
    GtkWidget *submitButton;
    GtkWidget *buttonHBox;
    GtkWidget *empty_row_hbox;
    GtkWidget *empty_row_add_button;
    GtkWidget *status_row;

    queryViewBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(queryViewBox);
    WidgetRegistry<QueryView>::set_widget(queryViewBox, this);

    // Add a label (error_label) to queryViewBox
    error_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(error_label),
        "<span color=\"red\">This is the error label</span>");
    gtk_widget_hide(error_label);
    gtk_box_pack_start(GTK_BOX(queryViewBox), error_label, FALSE, FALSE, 0);

    // Add a box (verticalBox) for the query rows to queryViewBox
    verticalBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(verticalBox);
    gtk_box_pack_start(GTK_BOX(queryViewBox), verticalBox, FALSE, FALSE, 0);

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
    QueryViewRow *queryViewRow = make_row();
    queryViewRows.insert(std::pair<GtkWidget*, QueryViewRow*>(queryViewRow->hbox, queryViewRow));
    gtk_box_pack_start(GTK_BOX(verticalBox), queryViewRow->hbox, FALSE, FALSE, 0);

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

    // Add a box for the scrollable text (scrollBox) to queryViewBox
    scrollBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(queryViewBox), scrollBox, TRUE, TRUE, 0);

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

    g_signal_connect(submitButton, "clicked", G_CALLBACK(query_submit_button_clicked_cb), NULL);
  }

  void
  QueryView::set_error_label(std::string text) {
    gtk_label_set_markup(GTK_LABEL(error_label),
        ("<span color=\"red\">" + text + "</span>").c_str());
    gtk_widget_show(GTK_WIDGET(error_label));
  }
  void
  QueryView::submit() {
    std::vector<std::string> value_vector;
    std::string queryJSON = makeQueryJSON();
    std::string sql_query_string = queryJSONToSqlQueryString(queryJSON, value_vector);
    bool b = query_transaction(sql_query_string, value_vector);
  }
