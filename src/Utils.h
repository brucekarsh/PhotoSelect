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

  static inline int get_rotation(sql::Connection *connection, std::string photoFileName) {
    int angle = 0;
    std::string sql = 
        "SELECT Rotation.angle "
        "FROM Rotation INNER JOIN Checksum on (Checksum.id = Rotation.checksumId) "
        "INNER JOIN PhotoFile ON (Checksum.id=PhotoFile.checksumId) "
        "WHERE PhotoFile.filePath=?";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, photoFileName);
    try {
        sql::ResultSet *rs = prepared_statement->executeQuery();
        if (rs->next()) {
          angle = rs->getDouble(1);
        }
    } catch (sql::SQLException &ex) {
    }
    return angle;
  }

  static inline void set_rotation(sql::Connection *connection, std::string photoFileName, int rotation) {
    std::string sql = "INSERT INTO Rotation (checksumId, angle) "
        "SELECT Checksum.id, ? "
        "FROM Checksum INNER JOIN PhotoFile ON (Checksum.id=PhotoFile.checksumId) "
        "WHERE PhotoFile.filePath=? "
        "ON DUPLICATE KEY UPDATE angle=?";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setDouble(1, rotation);
    prepared_statement->setString(2, photoFileName);
    prepared_statement->setDouble(3, rotation);
    try {
        prepared_statement->execute();
	connection->commit();
    } catch (sql::SQLException &ex) {
    }
  }

  static inline long
  insert_into_project(sql::Connection *connection, std::string project_name) {

    std::string project_insert_sql = "INSERT INTO Project (name) VALUES(?)";
    sql::PreparedStatement *project_insert_prepared_statement = connection->prepareStatement(
        project_insert_sql);

    std::string get_last_id_sql = "SELECT LAST_INSERT_ID() as id";
    sql::PreparedStatement *get_last_id_prepared_statement = connection->prepareStatement(
      get_last_id_sql);

    long projectId = -1;
    try {
      project_insert_prepared_statement->setString(1, project_name);
      project_insert_prepared_statement->execute();
      sql::ResultSet *rs = get_last_id_prepared_statement->executeQuery();
      if (rs->next()) {
        projectId = rs->getInt64("id");
      }
    } catch (sql::SQLException &ex) {
    }
    return projectId;
  }

  static inline long
  get_project_id(sql::Connection *connection, std::string project_name) {

    std::string project_select_sql = "SELECT id from Project where name = ?";
    sql::PreparedStatement *project_select_prepared_statement = connection->prepareStatement(
        project_select_sql);

    long project_id = -1;
    try {
      project_select_prepared_statement->setString(1,project_name);
      sql::ResultSet *rs =
      project_select_prepared_statement->executeQuery();
      if (rs->next()) {
        project_id = rs->getInt64("id");
      }
    } catch (sql::SQLException &ex) {
    }
    return project_id;
  }

  static inline std::list<std::string> get_project_names(sql::Connection *connection) {
    std::list<std::string> project_names;
    std::string sql = "SELECT DISTINCT name FROM Project ";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    while (rs->next()) {
      std::string project_name = rs->getString(1);
      project_names.push_back(project_name);
    }
    return project_names;
  }

  static inline bool delete_project(sql::Connection *connection, std::string project_name) {
    bool result = false;;
    std::string sql = "DELETE FROM Project WHERE name = ?";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    try {
      prepared_statement->setString(1, project_name);
      prepared_statement->execute();
      connection->commit();
      result = true;
    } catch (sql::SQLException &ex) {
      // Here result is false
    }
    return result;
  }

  static inline bool rename_project(sql::Connection *connection, std::string old_project_name,
      std::string new_project_name) {
    bool result = false;
    std::string sql = "UPDATE Project SET name=? WHERE name=?";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, new_project_name);
    prepared_statement->setString(2, old_project_name);
    try {
      prepared_statement->execute();
      connection->commit();
      result = true;
    } catch (sql::SQLException &ex) {
      // result will be false here
    }
    return result;
  }

  //! Add a photo file to a project. Note: does not do a commit. (Because usually this is
  //! used in a long loop).
  static inline void
  add_photo_to_project(sql::Connection *connection, long project_id, long photo_file_id) {
    std::string sql = "INSERT INTO ProjectPhotoFile (projectId, photoFileId) VALUES(?,?)";
    sql::PreparedStatement *project_photo_file_insert_prepared_statement =
        connection->prepareStatement(sql);
    project_photo_file_insert_prepared_statement->setInt64(1, project_id);
    project_photo_file_insert_prepared_statement->setInt64(2, photo_file_id);
    project_photo_file_insert_prepared_statement->execute();
  }

  //! Get all photo files for a project
  static inline std::list<std::string> get_project_photo_files(sql::Connection *connection,
      std::string project_name) {
    std::list<std::string> project_photo_files;
    std::string sql =
        "SELECT DISTINCT filePath FROM Project "
        "INNER JOIN ProjectPhotoFile ON (ProjectPhotoFile.projectId = Project.id) "
        "INNER JOIN PhotoFile ON (ProjectPhotoFile.photoFileId = PhotoFile.id) "
        "INNER JOIN Time ON (PhotoFile.checksumId = Time.checksumId) "
        "WHERE Project.name = ? "
        "ORDER by Time.adjustedDateTime, filePath ";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, project_name);
    sql::ResultSet *rs = prepared_statement->executeQuery();
    while ( rs->next()) {
      std::string file_path = rs->getString(1);
      project_photo_files.push_back(file_path);
    }
    return project_photo_files;
  }

  //! adds a tag to a checksum given a tag_name and a file_name
  static inline void
  add_tag_by_filename(sql::Connection *connection, std::string tag_name, std::string file_name) {
    std::string sql = "INSERT INTO TagChecksum (tagId, checksumId) "
        "SELECT DISTINCT Tag.id as tagId, Checksum.id as checksumId "
        "FROM Tag, Checksum, PhotoFile "
        "WHERE Tag.name = ? AND PhotoFile.filePath = ? AND Checksum.id = PhotoFile.checksumId";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, tag_name);
    prepared_statement->setString(2, file_name);
    prepared_statement->execute();
    connection->commit();
  }

  //! removes a tag to a checksum given a tag_name and a file_name
  static inline void
  remove_tag_by_filename(sql::Connection *connection, std::string tag_name, std::string file_name) {
    std::string sql = "DELETE FROM TagChecksum "
        "USING Tag, Checksum, PhotoFile, TagChecksum "
        "WHERE Tag.name = ? AND PhotoFile.filePath = ? "
        "AND Checksum.id = PhotoFile.checksumId AND TagChecksum.checksumId=Checksum.id "
        "AND TagChecksum.tagId=Tag.id";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setString(1, tag_name);
    prepared_statement->setString(2, file_name);
    prepared_statement->execute();
    connection->commit();
  }

  //! inserts an entry into the Time table. (It does not commit).
  static inline void
  insert_into_time(sql::Connection *connection, long checksum_id, std::string original_date_time,
      std::string adjusted_date_time) {
    std::string sql = "INSERT IGNORE INTO Time(checksumId, originalDateTime, adjustedDateTime ) "
        "Values(?,?,?)";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setInt64(1, checksum_id);
    prepared_statement->setString(2, original_date_time);
    prepared_statement->setString(3, adjusted_date_time);
    prepared_statement->executeUpdate();
  }

  //! inserts an entry into the ExifBlob table. (It does not commit).
  static inline void
  insert_into_exifblob(sql::Connection *connection, long checksum_id, std::string xml_string) {
    std::string sql = "INSERT IGNORE INTO ExifBlob(checksumId, value) Values (?,?)";
    sql::PreparedStatement *prepared_statement = connection->prepareStatement(sql);
    prepared_statement->setInt64(1, checksum_id);
    prepared_statement->setString(2, xml_string);
    prepared_statement->executeUpdate();
  }
};

#endif // UTILS_H__
