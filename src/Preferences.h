#ifndef PREFERENCES_H__
#define PREFERENCES_H__

#include <string>
#include <json/json.h>
#include <list>

class Preferences {
    std::string dbhost;    // the Db Host
    std::string user;      // the Db User
    std::string password;  // the Db Password
    std::string database;  // the DB database name
    std::list<std::string> checked_exif_selections;
    std::list<std::string> text_exif_selections;
    bool invalid;

    static const char * default_dbhost();
    static const char * default_user();
    static const char * default_password();
    static const char * default_database();

    //! Returns a list of strings given a name and a json object containing a
    //! (top level) named array of strings.
    std::list<std::string> get_list_of_strings_from_json(
        json_object *new_obj, std::string key);

    //! Returns a string from a json object given the string's name. If no string with
    //! the given name is found, returns a given default value.
    std::string get_string_from_json(
        json_object *new_obj, std::string key, std::string default_value);
    void validate();
    void use_default_preferences();
    const std::string get_preferences_text();
  public:
    Preferences();
    std::string get_dbhost();
    std::string get_user();
    std::string get_password();
    std::string get_database();
    void set_dbhost(const char *dbhost);
    void set_user(const char *user);
    void set_password(const char *password);
    void set_database(const char *database);
    void set_exif_selections( std::list<std::string> checked_exif_selections,
        std::list<std::string> text_exif_selections);
    std::list<std::string> get_checked_exif_selections();
    std::list<std::string> get_text_exif_selections();
    void invalidate();
    void writeback();
};
#endif  // PREFERENCESWINDOW_H__
