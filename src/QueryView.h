#ifndef QUERYVIEW_H__
#define QUERYVIEW_H__
#include "Db.h"
#include <gtk/gtk.h>

class QueryView {
  public:
    class QueryViewRow {
      public:
        GtkWidget *hbox;     // The hbox containing the row
        GtkWidget *fieldNameComboBox;
        GtkWidget *relationComboBox;
        GtkWidget *textEntryBox;
        GtkWidget *addButton;
        GtkWidget *removeButton;
    };
    Db db;
    std::map<GtkWidget *, QueryViewRow*> queryViewRows; // map from hbox to QueryViewRow
    GtkWidget *queryViewBox;
    GtkWidget *error_label;
    GtkWidget *verticalBox;
    GtkWidget *scrollTextView;
    GtkWidget *scrollWindow;
    GtkWidget *scrollBox;
    GtkWidget *status_label;
    GtkWidget *accept_button;
    GtkWidget *quitButton;
    std::vector<std::string> photoFilenameVector;
    std::list<long> photoFileIdList;
    bool is_limited_to_a_project;
    std::string limit_project_name; // Name of the project that the query is limited to

    QueryView();
    ~QueryView();
    std::string queryJSONToSqlQueryString(
        const std::string &queryJson, std::vector<std::string> &value_vector);
    std::string makeQueryJSON();
    GtkWidget *get_widget();
    GtkWidget *get_accept_button();
    GtkWidget *get_quit_button();
    const std::vector<std::string> &getPhotoFilenameVector();
    const std::list<long> &getPhotoFileIdList();
    void limit_to_a_project(std::string name);
    std::string translate_field_name(const std::string &fieldName);
    bool query_transaction(
        const std::string &sql_query_string, const std::vector<std::string> &value_vector);
    void query_op(const std::string &sql_query_string,
        const std::vector<std::string> &value_vector);
    QueryViewRow * make_row();
    static void query_submit_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void query_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void empty_row_add_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    static void query_remove_button_clicked_cb(GtkWidget *widget, gpointer callback_data);
    int get_queryViewRowi(QueryViewRow * queryViewRow);
    void runUI(int maxtimes);
    void run();
    void set_error_label(std::string text);
    void submit();
}; // end class QueryView
#endif // QUERYVIEW_H__
