#ifndef UTILS_H__
#define UTILS_H__

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

#include <map>
#include <set>

//! A class to hold some commonly used procedures.

class Utils {
  public:
    struct photo_tag_s {
    };

    struct project_tag_s {
    };

  static inline std::map<std::string, photo_tag_s>
      get_photo_tags(sql::Connection *connection, std::string project_name, std::string file_name) {
    std::map<std::string, photo_tag_s> tags;
    std::string sql = "SELECT Tag.name FROM Tag "
        "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
        "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
        "INNER JOIN TagChecksum ON (TagChecksum.tagId = Tag.id) "
        "INNER JOIN PhotoFile ON (TagChecksum.checksumId = PhotoFile.checksumId) "
        "WHERE (Project.name = ? and PhotoFile.filePath = ?)";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, project_name);
    prepared_statement->setString(2, file_name);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    while (rs->next()) {
      photo_tag_s tag;
      std::string name = rs->getString(1);
      tags[name] = tag;
    }
    return tags;
  }
  
  static inline std::map<std::string, project_tag_s>
      get_project_tags(sql::Connection *connection, std::string project_name) {
    std::map<std::string, project_tag_s> tags;
    std::string sql = "SELECT Tag.name FROM Tag "
        "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
        "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
        "WHERE (Project.name = ?)";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, project_name);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    while (rs->next()) {
      project_tag_s tag;
      std::string name = rs->getString(1);
      tags[name]=tag;
    } 
    return tags;
  }

  static inline std::set<std::string> get_all_tags(sql::Connection *connection) {
    std::set<std::string> tags;
    std::string sql = "SELECT DISTINCT Tag.name FROM Tag";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    while (rs->next()) {
      std::string name = rs->getString(1);
      tags.insert(name);
    }
    return tags;
  }

  static inline void insert_tag(sql::Connection *connection, std::string tag_name) {
    std::string sql = "INSERT INTO Tag(name) VALUE(?)";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, tag_name);
    try {
        prepared_statement->execute();
	connection->commit();
    } catch (sql::SQLException &ex) {
    }
  }

  static inline void delete_project_tag(sql::Connection *connection, std::string tag_name,
      std::string project_name) {
    std::string sql = "DELETE FROM ProjectTag USING Tag, Project, ProjectTag "
        "WHERE Tag.name=? "
        "AND Project.name=? "
        "AND ProjectTag.projectId = Project.id "
        "AND ProjectTag.tagId = Tag.id";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1,tag_name);
    prepared_statement->setString(2,project_name);
    try {
        prepared_statement->execute();
	connection->commit();
    } catch (sql::SQLException &ex) {
    }
  }

  static inline void insert_project_tag(sql::Connection *connection, std::string tag_name,
      std::string project_name) {
    std::string sql = "INSERT INTO ProjectTag(tagId, projectId) "
        "SELECT Tag.id, Project.id FROM Tag, Project WHERE Tag.name=? AND Project.name=?";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1,tag_name);
    prepared_statement->setString(2,project_name);
    try {
        prepared_statement->execute();
	connection->commit();
    } catch (sql::SQLException &ex) {
    }
  }
};

#endif // UTILS_H__
