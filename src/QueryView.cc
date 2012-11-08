#include "QueryView.h"

#include <json_spirit.h>
#include <json_spirit_reader_template.h>
#include <json_spirit_writer_template.h>

  std::string QueryView::queryJSONToSqlQueryString(
      const std::string &queryJson, std::vector<std::string> &value_vector) {
    json_spirit::mValue value;
    json_spirit::read(queryJson, value);
    json_spirit::mArray array;
    array = value.get_array();
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
    if (is_limited_to_a_project) {
      last_part.append("AND (pr.name = ?) ");
    }

    last_part.append("ORDER BY t.adjustedDateTime, filePath ");

    std::string first_part =
      "SELECT DISTINCT filePath, p.id FROM PhotoFile p "
      "INNER JOIN Checksum c ON p.checksumId = c.id "
      "INNER JOIN Time t ON t.checksumId = c.id ";

    if (is_limited_to_a_project) {
      first_part.append(
          "INNER JOIN ProjectPhotoFile pf ON pf.photoFileId = p.id "
          "INNER JOIN Project pr on pr.id = pf.projectId ");
    }

    std::string sql_statement = first_part + last_part;
    return sql_statement;
  }

  std::string
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
    std::string result = json_spirit::write( query_rows, json_spirit::pretty_print );
    return result;
  }

