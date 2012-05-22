#ifndef PREFERENCES_H__
#define PREFERENCES_H__

#include <string>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <wordexp.h>

class Preferences {
    std::string dbhost;    // the Db Host
    std::string user;      // the Db User
    std::string password;  // the Db Password
    std::string database;  // the DB database name
    bool invalid;

    static const char * default_dbhost() { return "localhost"; }
    static const char * default_user() { return ""; }
    static const char * default_password() { return ""; }
    static const char * default_database() { return "PhotoSelect"; }

    std::string get_string_from_json(
        json_object *new_obj, std::string key, std::string default_value) {
      std::string result;

      std::cout << "key " << key << std::endl;
      std::cout << "default_value " << default_value << std::endl;
      json_object *object = json_object_object_get(new_obj, key.c_str());
      if (object && json_type_string == json_object_get_type(object)) {
        result = json_object_get_string(object);
      } else {
        result = default_value;
      }
      std::cout << "result " << result << std::endl;
      return result;
    }
    void validate() {
      printf("\nValidating\n");

      std::string preferences_text = get_preferences_text();

      std::cout << "preferences_text " << preferences_text << std::endl;

      if (preferences_text == "") {
        use_default_preferences();
      } else {
        struct json_object *new_obj = json_tokener_parse(preferences_text.c_str());
	if (0 == new_obj) {
          use_default_preferences();
        } else {
	  std::cout << "using parsed preferences" << std::endl;
          dbhost = get_string_from_json(new_obj, "dbhost", default_dbhost());
          user = get_string_from_json(new_obj, "user", default_user());
          password = get_string_from_json(new_obj, "password", default_password());
          database = get_string_from_json(new_obj, "database", default_database());
        }
      }
      invalid = false;
    }

    void use_default_preferences() {
	std::cout << "use_default_preferences called" << std::endl;
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
      printf("get_dbhost returns %s\n", dbhost.c_str());
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
      printf("set_dbhost sets %s\n", dbhost);
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

    void invalidate() {
      invalid = true;
    }

    void writeback() {
      struct json_object *new_obj = json_object_new_object();
      json_object_object_add(new_obj, "dbhost", json_object_new_string(dbhost.c_str()));
      json_object_object_add(new_obj, "user", json_object_new_string(user.c_str()));
      json_object_object_add(new_obj, "password", json_object_new_string(password.c_str()));
      json_object_object_add(new_obj, "database", json_object_new_string(database.c_str()));
      wordexp_t exp_result;
      wordexp("~/.PhotoSelect", &exp_result, 0);
      std::ofstream outfile(exp_result.we_wordv[0], std::ios::trunc | std::ios::out);
      wordfree(&exp_result);
      outfile << json_object_to_json_string(new_obj) << std::endl;
      outfile.close();
    }
};
#endif  // PREFERENCESWINDOW_H__
