#ifndef PREFERENCES_H__
#define PREFERENCES_H__

#include <string>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <wordexp.h>
#include <boost/foreach.hpp>

class Preferences {
    std::string dbhost;    // the Db Host
    std::string user;      // the Db User
    std::string password;  // the Db Password
    std::string database;  // the DB database name
    std::list<std::string> checked_exif_selections;
    std::list<std::string> text_exif_selections;
    bool invalid;

    static const char * default_dbhost() { return "localhost"; }
    static const char * default_user() { return ""; }
    static const char * default_password() { return ""; }
    static const char * default_database() { return "PhotoSelect"; }

    //! Returns a list of strings given a name and a json object containing a
    //! (top level) named array of strings.
    std::list<std::string> get_list_of_strings_from_json(
        json_object *new_obj, std::string key) {
      std::list<std::string> result;
      json_object *object = json_object_object_get(new_obj, key.c_str());
      if (object && json_type_array == json_object_get_type(object)) {
	int num_elements = json_object_array_length(object);
        for (int i = 0; i < num_elements; i++) {
          json_object *element = json_object_array_get_idx(object, i);
          if (json_type_string == json_object_get_type(element)) {
	    std::string element_value = json_object_get_string(element);
	    result.push_back(element_value);
          }
        }
      }
      return result;
    }

    //! Returns a string from a json object given the string's name. If no string with
    //! the given name is found, returns a given default value.
    std::string get_string_from_json(
        json_object *new_obj, std::string key, std::string default_value) {
      std::string result;

      json_object *object = json_object_object_get(new_obj, key.c_str());
      if (object && json_type_string == json_object_get_type(object)) {
        result = json_object_get_string(object);
      } else {
        result = default_value;
      }
      return result;
    }

    void validate() {
      std::string preferences_text = get_preferences_text();

      if (preferences_text == "") {
        use_default_preferences();
      } else {
        struct json_object *new_obj = json_tokener_parse(preferences_text.c_str());
	if (0 == new_obj) {
          use_default_preferences();
        } else {
          dbhost = get_string_from_json(new_obj, "dbhost", default_dbhost());
          user = get_string_from_json(new_obj, "user", default_user());
          password = get_string_from_json(new_obj, "password", default_password());
          database = get_string_from_json(new_obj, "database", default_database());
          checked_exif_selections
              = get_list_of_strings_from_json(new_obj, "checked_exif_selections");
          text_exif_selections = get_list_of_strings_from_json(new_obj, "text_exif_selections");
        }
      }
      invalid = false;
    }

    void use_default_preferences() {
        dbhost = default_dbhost();
        user = default_user();
        password = default_password();
        database = default_database();
    }

    const std::string get_preferences_text() {
      wordexp_t exp_result;
      wordexp("~/.PhotoSelect", &exp_result, 0);
      std::ifstream infile(exp_result.we_wordv[0], std::ios::in);
      wordfree(&exp_result);
      std::string result;
      if (infile.fail()) {
        result = "";
      } else {
        std::stringstream buffer;
        buffer << infile.rdbuf();
        result = buffer.str();
      }
      infile.close();
      return result;
    }

  public:

    Preferences() : invalid(true) {
    }

    std::string get_dbhost() {
      if(invalid) validate();
      return dbhost;
    }

    std::string get_user() {
      if(invalid) validate();
      return user;
    }

    std::string get_password() {
      if(invalid) validate();
      return password;
    }

    std::string get_database() {
      if(invalid) validate();
      return database;
    }

    void set_dbhost(const char *dbhost) {
      this -> dbhost = dbhost;
    }

    void set_user(const char *user) {
      this -> user = user;
    }

    void set_password(const char *password) {
      this -> password = password;
    }

    void set_database(const char *database) {
      this -> database = database;
    }

    void set_exif_selections( std::list<std::string> checked_exif_selections,
        std::list<std::string> text_exif_selections) {
      this->checked_exif_selections = checked_exif_selections;
      this->text_exif_selections = text_exif_selections;
    }

    std::list<std::string> get_checked_exif_selections() {
      if(invalid) validate();
      return checked_exif_selections;
    }

    std::list<std::string> get_text_exif_selections() {
      if(invalid) validate();
      return text_exif_selections;
    }

    void invalidate() {
      invalid = true;
    }

    void writeback() {
      // Make a json representation of the preferences
      struct json_object *new_obj = json_object_new_object();
      json_object_object_add(new_obj, "dbhost", json_object_new_string(dbhost.c_str()));
      json_object_object_add(new_obj, "user", json_object_new_string(user.c_str()));
      json_object_object_add(new_obj, "password", json_object_new_string(password.c_str()));
      json_object_object_add(new_obj, "database", json_object_new_string(database.c_str()));
      // Add the checked_exif_selections
      json_object *checked_exif_selections_array = json_object_new_array();
      BOOST_FOREACH(std::string exif_name, checked_exif_selections) {
        json_object_array_add(checked_exif_selections_array,
          json_object_new_string(exif_name.c_str()));
      }
      json_object_object_add(new_obj, "checked_exif_selections", checked_exif_selections_array);
      // Add the text_exif_selections
      json_object *text_exif_selections_array = json_object_new_array();
      BOOST_FOREACH(std::string exif_name, text_exif_selections) {
        json_object_array_add(text_exif_selections_array,
          json_object_new_string(exif_name.c_str()));
      }
      json_object_object_add(new_obj, "text_exif_selections", text_exif_selections_array);
  
      // Write the json to a file.
      wordexp_t exp_result;
      wordexp("~/.PhotoSelect", &exp_result, 0);
      std::ofstream outfile(exp_result.we_wordv[0], std::ios::trunc | std::ios::out);
      wordfree(&exp_result);
      outfile << json_object_to_json_string(new_obj) << std::endl;
      outfile.close();
    }
};
#endif  // PREFERENCESWINDOW_H__
