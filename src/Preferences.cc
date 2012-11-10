#include "Preferences.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <wordexp.h>
#include <boost/foreach.hpp>

using namespace std;

/* static */ const char * Preferences::default_dbhost() { return "localhost"; }
/* static */ const char * Preferences::default_user() { return ""; }
/* static */ const char * Preferences::default_password() { return ""; }
/* static */ const char * Preferences::default_database() { return "PhotoSelect"; }

//! Returns a list of strings given a name and a json object containing a
//! (top level) named array of strings.
list<string> Preferences::get_list_of_strings_from_json(
    json_object *new_obj, string key) {
  list<string> result;
  json_object *object = json_object_object_get(new_obj, key.c_str());
  if (object && json_type_array == json_object_get_type(object)) {
	int num_elements = json_object_array_length(object);
    for (int i = 0; i < num_elements; i++) {
      json_object *element = json_object_array_get_idx(object, i);
      if (json_type_string == json_object_get_type(element)) {
	    string element_value = json_object_get_string(element);
	    result.push_back(element_value);
      }
    }
  }
  return result;
}

//! Returns a string from a json object given the string's name. If no string with
//! the given name is found, returns a given default value.
string Preferences::get_string_from_json(
    json_object *new_obj, string key, string default_value) {
  string result;

  json_object *object = json_object_object_get(new_obj, key.c_str());
  if (object && json_type_string == json_object_get_type(object)) {
    result = json_object_get_string(object);
  } else {
    result = default_value;
  }
  return result;
}

void Preferences::validate() {
  string preferences_text = get_preferences_text();

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

void Preferences::use_default_preferences() {
    dbhost = default_dbhost();
    user = default_user();
    password = default_password();
    database = default_database();
}

const string Preferences::get_preferences_text() {
  wordexp_t exp_result;
  wordexp("~/.PhotoSelect", &exp_result, 0);
  ifstream infile(exp_result.we_wordv[0], ios::in);
  wordfree(&exp_result);
  string result;
  if (infile.fail()) {
    result = "";
  } else {
    stringstream buffer;
    buffer << infile.rdbuf();
    result = buffer.str();
  }
  infile.close();
  return result;
}

Preferences::Preferences() : invalid(true) {
}

string Preferences::get_dbhost() {
  if(invalid) validate();
  return dbhost;
}

string Preferences::get_user() {
  if(invalid) validate();
  return user;
}

string Preferences::get_password() {
  if(invalid) validate();
  return password;
}

string Preferences::get_database() {
  if(invalid) validate();
  return database;
}

void Preferences::set_dbhost(const char *dbhost) {
  this -> dbhost = dbhost;
}

void Preferences::set_user(const char *user) {
  this -> user = user;
}

void Preferences::set_password(const char *password) {
  this -> password = password;
}

void Preferences::set_database(const char *database) {
  this -> database = database;
}

void Preferences::set_exif_selections(list<string> checked_exif_selections,
    list<string> text_exif_selections) {
  this->checked_exif_selections = checked_exif_selections;
  this->text_exif_selections = text_exif_selections;
}

list<string> Preferences::get_checked_exif_selections() {
  if(invalid) validate();
  return checked_exif_selections;
}

list<string> Preferences::get_text_exif_selections() {
  if(invalid) validate();
  return text_exif_selections;
}

void Preferences::invalidate() {
  invalid = true;
}

void Preferences::writeback() {
  // Make a json representation of the preferences
  struct json_object *new_obj = json_object_new_object();
  json_object_object_add(new_obj, "dbhost", json_object_new_string(dbhost.c_str()));
  json_object_object_add(new_obj, "user", json_object_new_string(user.c_str()));
  json_object_object_add(new_obj, "password", json_object_new_string(password.c_str()));
  json_object_object_add(new_obj, "database", json_object_new_string(database.c_str()));
  // Add the checked_exif_selections
  json_object *checked_exif_selections_array = json_object_new_array();
  BOOST_FOREACH(string exif_name, checked_exif_selections) {
    json_object_array_add(checked_exif_selections_array,
      json_object_new_string(exif_name.c_str()));
  }
  json_object_object_add(new_obj, "checked_exif_selections", checked_exif_selections_array);
  // Add the text_exif_selections
  json_object *text_exif_selections_array = json_object_new_array();
  BOOST_FOREACH(string exif_name, text_exif_selections) {
    json_object_array_add(text_exif_selections_array,
      json_object_new_string(exif_name.c_str()));
  }
  json_object_object_add(new_obj, "text_exif_selections", text_exif_selections_array);
  
  // Write the json to a file.
  wordexp_t exp_result;
  wordexp("~/.PhotoSelect", &exp_result, 0);
  ofstream outfile(exp_result.we_wordv[0], ios::trunc | ios::out);
  wordfree(&exp_result);
  outfile << json_object_to_json_string(new_obj) << endl;
  outfile.close();
}
