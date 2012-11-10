#ifndef DB_H__
#define DB_H__

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
#include <boost/function.hpp>
#include <boost/bind.hpp>

//! A class to hold database access procedures.
//! Most of the methods are either database transactions (method name ends in _transaction) or
//! database operations (method name ends in _op). A transaction executes a sequence of database
//! operations as an SQL transaction with retry and reacquistion of the connection.

class Db {
  public:
    static const int NRETRIES = 4;
    static sql::Connection *connection;
    static bool transaction_is_running;
    static std::string dbhost;
    static std::string user;
    static std::string password;
    static std::string database;

  static inline void close_connection() {
    BOOST_ASSERT(NULL != Db::connection);
    delete Db::connection;
    Db::connection = NULL;
  }

  //! this method is called before every _op procedure. It exits if the procedure was called
  //! from outside of a transaction.
  static inline void enter_operation() {
    if (!transaction_is_running) {
      std::cout << "Operation was called from outside a transaction, exiting." << std::endl;
      abort();
    }
  }

    //! Issue a transaction with retries and reaquisition of a connection
    //! Sometimes a transaction will fail due to a transient cause, e.g. deadlock or database
    //! server restart. This procedure issues a transaction repeatedly until the transaction
    //! succeeds. On each failure, it reopens the connection. If after retrying NRETRIES times
    //! it still has failed, it returns false. Otherwise it returns true.
    //! \param f a function that issues the database operations of the trasaction.
    static bool transaction(const boost::function<void(void)> &f) {
      // First make sure that we are not already in a transaction. We don't allow nested
      // transactions
      if (transaction_is_running) {
        std::cout << "Nested Transaction detected, exiting." << std::endl;
        exit(1);
      }
      transaction_is_running = true;

      bool succeed = false;
      for (int retry_count = 0; retry_count < NRETRIES; retry_count++) {
        sql::Connection *connection = Db::get_connection();

        succeed = true;
        try {
          f();
        } catch (sql::SQLException &ex) {
          std::cout << "exception during transaction " << ex.what() << ex.getSQLState()
              << " " << ex.getErrorCode()<< std::endl;
          succeed = false;
        }
        if (succeed) {
          try { 
            connection->commit();
          } catch (sql::SQLException &ex) {
          std::cout << "exception during commit " << ex.what() << ex.getSQLState()
              << " " << ex.getErrorCode()<< std::endl;
          succeed = false;
          }
          break;
        } else {
          try {
            connection->rollback();
          } catch (sql::SQLException &ex) {
          // the rollback failed. Probably because the connection is gone.
          std::cout << "exception during rollback " << ex.what() << ex.getSQLState()
              << " " << ex.getErrorCode()<< std::endl;
          }
        }
        // close the connection here. It will be repoened next time get_connection is called.
        // TODO don't close it if we don't have to.
        std::cout << "Retrying transaction " << retry_count << std::endl;
        close_connection();
      }
      transaction_is_running = false;
      return succeed;
    }

  static inline sql::Connection *get_connection() {
    if (Db::connection) {
      return Db::connection;
    }
    // TODO don't get the driver instance over and over. Just get it once.
    /* initiate url, user, password and database variables */
    sql::Driver *driver = Db::get_driver_instance();
    if (0 == driver) {
      std::cerr <<  "get_driver_instance() failed.\n" << std::endl;
      // TODO do something better than exit here
      exit(1);
    }

    sql::Connection *connection = Db::get_connection(driver, dbhost, user, password);
    if (NULL == connection) {
      std::cerr << "driver -> connect() failed\n" << std::endl;
    } else {
      connection->setSchema(database);
      Db::connection = connection;
    }
    return connection;
  }

  static inline void get_photo_tags_op(const std::string &project_name,
      const std::string &file_name, std::set<std::string> &tags) {
    Db::enter_operation();
    tags.clear();
    std::string sql = "SELECT Tag.name FROM Tag "
        "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
        "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
        "INNER JOIN TagChecksum ON (TagChecksum.tagId = Tag.id) "
        "INNER JOIN PhotoFile ON (TagChecksum.checksumId = PhotoFile.checksumId) "
        "WHERE (Project.name = ? and PhotoFile.filePath = ?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, project_name);
    prepared_statement->setString(2, file_name);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    while (rs->next()) {
      std::string name = rs->getString(1);
      tags.insert(name);
    }
  };

  static inline bool get_photo_tags_transaction(const std::string &project_name,
      const std::string &file_name, std::set<std::string> &result) {
    boost::function<void (void)> f = boost::bind(&get_photo_tags_op, boost::cref(project_name),
        boost::cref(file_name), boost::ref(result));
    return transaction(f);
  }

  typedef std::map<std::string, std::set<std::string> > all_photo_tags_map_t;
  typedef std::pair<std::string, std::set<std::string> > all_photo_tags_map_entry_t;
  static inline void get_all_photo_tags_for_project_op(const std::string &project_name,
      all_photo_tags_map_t &result) {
    Db::enter_operation();
    result.clear();
    std::string sql = "SELECT PhotoFile.filePath, Tag.name "
        "FROM Project "
        "INNER JOIN ProjectPhotoFile ON (Project.id = ProjectPhotoFile.projectId) "
        "INNER JOIN PhotoFile ON (ProjectPhotoFile.photoFileId = PhotoFile.id) "
        "INNER JOIN TagChecksum ON (PhotoFile.checksumId = TagChecksum.checksumId) "
        "INNER JOIN Tag ON (Tag.id = TagChecksum.tagId) "
        "WHERE (Project.name = ?) "
        "ORDER BY PhotoFile.filePath ";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, project_name);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    while (rs->next()) {
      std::string filePath = rs->getString(1);
      std::string tag = rs->getString(2);
      result[filePath].insert(tag);
    }
    return;
  }

  static inline bool get_all_photo_tags_for_project_transaction(const std::string &project_name,
      all_photo_tags_map_t &result) {
    boost::function<void (void)> f = boost::bind(&get_all_photo_tags_for_project_op,
        boost::cref(project_name), boost::ref(result));
    bool b = transaction(f);
    return b;
  }
  
  static inline void get_project_tags_op(const std::string &project_name,
      std::set<std::string> &tags) {
    Db::enter_operation();
    tags.clear();
    std::string sql = "SELECT Tag.name FROM Tag "
        "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
        "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
        "WHERE (Project.name = ?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, project_name);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    while (rs->next()) {
      std::string name = rs->getString(1);
      tags.insert(name);
    } 
  }

  static inline bool get_project_tags_transaction(const std::string &project_name,
      std::set<std::string> &tags) {
    boost::function<void (void)> f = boost::bind(&get_project_tags_op,
        boost::cref(project_name), boost::ref(tags));
    bool b = transaction(f);
    return b;
  }

  static inline void get_all_tags_op(std::set<std::string> &tags) {
    Db::enter_operation();
    tags.clear();
    std::string sql = "SELECT DISTINCT Tag.name FROM Tag";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    while (rs->next()) {
      std::string name = rs->getString(1);
      tags.insert(name);
    }
  }

  static inline bool get_all_tags_transaction(std::set<std::string>& tags) {
    boost::function<void (void)> f = boost::bind(&get_all_tags_op, boost::ref(tags));
    bool b = transaction(f);
    return b;
  }

  static inline void insert_tag_op(const std::string &tag_name) {
    Db::enter_operation();
    std::string sql = "INSERT INTO Tag(name) VALUE(?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, tag_name);
    prepared_statement->execute();
  }

  static inline bool insert_tag_transaction(const std::string &tag_name) {
    boost::function<void (void)> f = boost::bind(&insert_tag_op, boost::cref(tag_name));
    bool b = transaction(f);
    return b;
  }

  static inline void delete_project_tag_op(const std::string &tag_name,
      const std::string &project_name) {
    Db::enter_operation();
    std::string sql = "DELETE FROM ProjectTag USING Tag, Project, ProjectTag "
        "WHERE Tag.name=? "
        "AND Project.name=? "
        "AND ProjectTag.projectId = Project.id "
        "AND ProjectTag.tagId = Tag.id";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1,tag_name);
    prepared_statement->setString(2,project_name);
    prepared_statement->execute();
  }

  static inline bool delete_project_tag_transaction(const std::string &tag_name,
      const std::string &project_name) {
    boost::function<void (void)> f = boost::bind(&delete_project_tag_op, boost::cref(tag_name),
        boost::cref(project_name));
    bool b = transaction(f);
    return b;
  }

  static inline void insert_project_tag_op(const std::string &tag_name,
      const std::string &project_name) {
    Db::enter_operation();
    std::string sql = "INSERT INTO ProjectTag(tagId, projectId) "
        "SELECT Tag.id, Project.id FROM Tag, Project WHERE Tag.name=? AND Project.name=?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1,tag_name);
    prepared_statement->setString(2,project_name);
    prepared_statement->execute();
  }

  static inline bool insert_project_tag_transaction(const std::string &tag_name,
      const std::string &project_name) {
    boost::function<void (void)> f = boost::bind(&insert_project_tag_op, boost::cref(tag_name),
        boost::cref(project_name));
    bool b = transaction(f);
    return b;
  }

  static inline void get_rotation_op(const std::string &photoFileName, int &angle) {
    Db::enter_operation();
    angle = 0;
    std::string sql = 
        "SELECT Rotation.angle "
        "FROM Rotation INNER JOIN Checksum on (Checksum.id = Rotation.checksumId) "
        "INNER JOIN PhotoFile ON (Checksum.id=PhotoFile.checksumId) "
        "WHERE PhotoFile.filePath=?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, photoFileName);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    if (rs->next()) {
      angle = rs->getDouble(1);
    }
  }

  static inline bool get_rotation_transaction(const std::string &photoFileName, int &angle) {
    boost::function<void (void)> f = boost::bind(&get_rotation_op, boost::cref(photoFileName),
        boost::ref(angle));
    bool b = transaction(f);
    return b;
  }

  static inline void set_rotation_op(const std::string &photoFileName, int rotation) {
    Db::enter_operation();
    std::string sql = "INSERT INTO Rotation (checksumId, angle) "
        "SELECT Checksum.id, ? "
        "FROM Checksum INNER JOIN PhotoFile ON (Checksum.id=PhotoFile.checksumId) "
        "WHERE PhotoFile.filePath=? "
        "ON DUPLICATE KEY UPDATE angle=?";
      std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
      prepared_statement->setDouble(1, rotation);
      prepared_statement->setString(2, photoFileName);
      prepared_statement->setDouble(3, rotation);
      prepared_statement->execute();
  }

  static inline bool set_rotation_transaction(const std::string &photoFileName, int rotation) {
    boost::function<void (void)> f = boost::bind(&set_rotation_op, boost::cref(photoFileName),
        rotation);
    return transaction(f);
  }

  //! database operation to insert a record into the Project table
  static inline void insert_into_project_op(const std::string &project_name, long &project_id) {
    Db::enter_operation();
    std::string project_insert_sql = "INSERT INTO Project (name) VALUES(?)";
    std::unique_ptr<sql::PreparedStatement> project_insert_prepared_statement(
        connection->prepareStatement(project_insert_sql));

    std::string get_last_id_sql = "SELECT LAST_INSERT_ID() as id";
    std::unique_ptr<sql::PreparedStatement> get_last_id_prepared_statement(
        connection->prepareStatement(get_last_id_sql));

    project_id = -1;
    project_insert_prepared_statement->setString(1, project_name);
    project_insert_prepared_statement->execute();
    std::unique_ptr<sql::ResultSet> rs(get_last_id_prepared_statement->executeQuery());
    if (rs->next()) {
      project_id = rs->getInt64("id");
    }
  }

  static inline long get_project_id_op(const std::string &project_name, long &project_id) {
    Db::enter_operation();
    std::string sql = "SELECT id from Project where name = ?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));

    project_id = -1;
    prepared_statement->setString(1,project_name);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    if (rs->next()) {
      project_id = rs->getInt64("id");
    }
  }

  static inline void get_project_names_op(std::list<std::string> &project_names) {
    Db::enter_operation();
    std::string sql = "SELECT DISTINCT name FROM Project ";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    project_names.clear();
    while (rs->next()) {
      std::string project_name = rs->getString(1);
      project_names.push_back(project_name);
    }
  }

  static inline bool get_project_names_transaction(std::list<std::string> &project_names) {
    boost::function<void (void)> f = boost::bind(&get_project_names_op,
        boost::ref(project_names));
    return transaction(f);
  }

  static inline void delete_project_op(const std::string &project_name) {
    Db::enter_operation();
    std::string sql = "DELETE FROM Project WHERE name = ?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, project_name);
    prepared_statement->execute();
  }

  static inline bool delete_project_transaction(const std::string &project_name) {
    boost::function<void (void)> f = boost::bind(&delete_project_op, boost::cref(project_name));
    return transaction(f);
  }

  static inline void rename_project_op(const std::string &old_project_name,
      const std::string &new_project_name) {
    Db::enter_operation();
    std::string sql = "UPDATE Project SET name=? WHERE name=?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, new_project_name);
    prepared_statement->setString(2, old_project_name);
    prepared_statement->execute();
  }

  static inline bool rename_project_transaction(const std::string &old_project_name,
      const std::string &new_project_name) {
    boost::function<void (void)> f = boost::bind(&rename_project_op, boost::cref(old_project_name),
        boost::cref(new_project_name));
    return transaction(f);
  }

  //! database operation to add a photo file to a project.
  static inline void add_photo_to_project_op(long project_id, long photo_file_id) {
    Db::enter_operation();
    std::string sql = "INSERT INTO ProjectPhotoFile (projectId, photoFileId) VALUES(?,?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setInt64(1, project_id);
    prepared_statement->setInt64(2, photo_file_id);
    prepared_statement->execute();
  }

  //! Get all photo files for a project
  static inline bool get_project_photo_files_op(
      const std::string &project_name, std::vector<std::string> &project_photo_files,
      std::vector<std::string> &project_adjusted_date_times) {
    Db::enter_operation();

    std::string sql =
        "SELECT DISTINCT filePath, Time.adjustedDateTime FROM Project "
        "INNER JOIN ProjectPhotoFile ON (ProjectPhotoFile.projectId = Project.id) "
        "INNER JOIN PhotoFile ON (ProjectPhotoFile.photoFileId = PhotoFile.id) "
        "INNER JOIN Time ON (PhotoFile.checksumId = Time.checksumId) "
        "WHERE Project.name = ? "
        "ORDER by Time.adjustedDateTime, filePath ";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, project_name);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    project_photo_files.clear();
    project_adjusted_date_times.clear();
    while ( rs->next()) {
      std::string file_path = rs->getString(1);
      std::string adjusted_date_time = rs->getString(2);
      project_photo_files.push_back(file_path);
      project_adjusted_date_times.push_back(adjusted_date_time);
    }
  }

  static inline bool get_project_photo_files_transaction(
      const std::string &project_name, std::vector<std::string> &project_photo_files,
      std::vector<std::string> &project_adjusted_date_times) {
    boost::function<void (void)> f = boost::bind(&get_project_photo_files_op,
        boost::cref(project_name),
        boost::ref(project_photo_files),
        boost::ref(project_adjusted_date_times));
    return transaction(f);
  }
  //! adds a tag to a checksum given a tag_name and a file_name
  static inline void add_tag_by_filename_op(const std::string &tag_name,
      const std::string &file_name) {
    Db::enter_operation();
    std::string sql = "INSERT INTO TagChecksum (tagId, checksumId) "
        "SELECT DISTINCT Tag.id as tagId, Checksum.id as checksumId "
        "FROM Tag, Checksum, PhotoFile "
        "WHERE Tag.name = ? AND PhotoFile.filePath = ? AND Checksum.id = PhotoFile.checksumId";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, tag_name);
    prepared_statement->setString(2, file_name);
    prepared_statement->execute();
  }

  static inline bool add_tag_by_filename_transaction(const std::string &tag_name,
      const std::string &file_name) {
    boost::function<void (void)> f = boost::bind(&add_tag_by_filename_op,
        boost::cref(tag_name), boost::cref(file_name));
    return transaction(f);
  }

  //! removes a tag to a checksum given a tag_name and a file_name
  static inline void remove_tag_by_filename_op(const std::string &tag_name,
      const std::string &file_name) {
    Db::enter_operation();
    std::string sql = "DELETE FROM TagChecksum "
        "USING Tag, Checksum, PhotoFile, TagChecksum "
        "WHERE Tag.name = ? AND PhotoFile.filePath = ? "
        "AND Checksum.id = PhotoFile.checksumId AND TagChecksum.checksumId=Checksum.id "
        "AND TagChecksum.tagId=Tag.id";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, tag_name);
    prepared_statement->setString(2, file_name);
    prepared_statement->execute();
  }

  static inline bool remove_tag_by_filename_transaction(const std::string &tag_name,
      const std::string &file_name) {
    boost::function<void (void)> f = boost::bind(&remove_tag_by_filename_op,
        boost::cref(tag_name), boost::cref(file_name));
    return transaction(f);
  }


  //! database operation to insert a record into the Time table
  static inline void insert_into_time_op(long checksum_id, const std::string &original_date_time,
      const std::string &adjusted_date_time) {
    Db::enter_operation();
    std::string sql = "INSERT IGNORE INTO Time(checksumId, originalDateTime, adjustedDateTime ) "
        "Values(?,?,?) "
        "ON DUPLICATE KEY UPDATE originalDateTime=?, adjustedDateTime=?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setInt64(1, checksum_id);
    prepared_statement->setString(2, original_date_time);
    prepared_statement->setString(3, adjusted_date_time);
    prepared_statement->setString(4, original_date_time);
    prepared_statement->setString(5, adjusted_date_time);
    prepared_statement->executeUpdate();
  }

  static inline void get_adjusted_date_time_op(const std::string &filename,
      std::string& adjusted_date_time, bool &ret) {
    Db::enter_operation();
    std::string sql = "SELECT adjustedDateTime from PhotoFile "
        "INNER JOIN Time ON (PhotoFile.checksumId = Time.checksumId) WHERE PhotoFile.filePath = ?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, filename);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    bool has_first = rs->first();
    if (!has_first) {
      std::cout << "Cannot get a result set in get_adjusted_date_time" << std::endl;
      ret = false;
      return;
    }
    bool is_first = rs->isFirst();
    bool is_last = rs->isLast();
    if (!is_first || ! is_last) {
      std::cout << "More than one key found in results in get_adjusted_date_time" << std::endl;
      std::cout << "isFirst(): " << rs->isFirst() << std::endl;
      std::cout << "isLast(): " << rs->isLast() << std::endl;
      ret = false;
      return;
    }
    adjusted_date_time = rs->getString(1);
    ret = true;
  }

  //! database operation to insert an entry into the ExifBlob table.
  static inline void insert_into_exifblob_op(long checksum_id, const std::string &xml_string) {
    Db::enter_operation();
    std::string sql = "INSERT IGNORE INTO ExifBlob(checksumId, value) Values (?,?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setInt64(1, checksum_id);
    prepared_statement->setString(2, xml_string);
    prepared_statement->executeUpdate();
  }

  static inline void get_from_exifblob_by_filePath_op(
      const std::string &filePath, std::string &value) {
    Db::enter_operation();
    std::string sql = "SELECT value from PhotoFile "
        "INNER JOIN ExifBlob ON (PhotoFile.checksumId = ExifBlob.checksumId) "
        "WHERE PhotoFile.filePath = ?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, filePath);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    bool has_first = rs->first();
    if (!has_first) {
      std::cout << "Cannot get a result set in get_from_exifblob_by_filePath" << std::endl;
      // TODO do something better than exit here
      exit(1);
    }
    bool is_first = rs->isFirst();
    bool is_last = rs->isLast();
    if (!is_first || ! is_last) {
      std::cout << "More than one key found in results in get_id_from_Checksum" << std::endl;
      std::cout << "isFirst(): " << rs->isFirst() << std::endl;
      std::cout << "isLast(): " << rs->isLast() << std::endl;
      // TODO do something better than exit here
      exit(1);
    }
    value = rs->getString(1);
  }

  static inline bool get_from_exifblob_by_filePath_transaction(
      const std::string &filePath, std::string &value) {
    boost::function<void (void)> f = boost::bind(&get_from_exifblob_by_filePath_op,
        boost::cref(filePath), boost::ref(value));
    return transaction(f);
  }

  //! database operation to insert a checksum into the Checksum table
  static inline int64_t insert_into_Checksum_op(
      const std::string &checksum, int64_t &checksum_key) {
    Db::enter_operation();
    std::string sql = "INSERT IGNORE INTO Checksum(checksum) Values (?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, checksum.c_str());
    int updateCount = prepared_statement->executeUpdate();
    Db::get_id_from_Checksum_op(checksum, checksum_key);
  }

  static inline void
  get_id_from_Checksum_op(const std::string &checksum, int64_t &checksum_key) {
    Db::enter_operation();
    std::string sql = "SELECT id FROM Checksum where checksum = ?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, checksum);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());

    bool has_first = rs->first();
    if (!has_first) {
      std::cout << "Cannot get a result set in get_id_from_Checksum" << std::endl;
      // TODO do something better than exit
      exit(1);
    }
    bool is_first = rs->isFirst();
    bool is_last = rs->isLast();
    if (!is_first || ! is_last) {
      std::cout << "More than one key found in results in get_id_from_Checksum" << std::endl;
      std::cout << "isFirst(): " << rs->isFirst() << std::endl;
      std::cout << "isLast(): " << rs->isLast() << std::endl;
      // TODO do something better than exit
      exit(1);
    }
    checksum_key = rs->getInt64("id");
  }

  //! database operation to insert a record into the PhotoFile table
  //! does not insert and just returns the PhotoFileId if duplicate
  static inline void insert_into_PhotoFile_op(
      const std::string &filePath, const int64_t &checksum_key, int64_t &photoFile_key) {
    Db::enter_operation();
    // If it's already in the database, just return its id
    Db::get_id_from_PhotoFile_op(filePath, checksum_key, photoFile_key);
    if (photoFile_key != -1) {
      return;
    }

    std::string sql = "REPLACE INTO PhotoFile(filePath, checksumId) Values (?,?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, filePath.c_str());
    prepared_statement->setInt64(2, checksum_key);
    int updateCount = prepared_statement->executeUpdate();

    get_id_from_PhotoFile_op(filePath, checksum_key, photoFile_key);
  }

  static inline void get_id_from_PhotoFile_op(const std::string &filePath,
      int64_t checksum_key, int64_t &photoFile_key) {
    Db::enter_operation();
    std::string sql = "SELECT id FROM PhotoFile where filePath = ? and checksumId = ?";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1, filePath.c_str());
    prepared_statement->setInt64(2, checksum_key);
    std::unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
    bool has_first = rs -> first();
    if (has_first) {
      bool is_first = rs->isFirst();
      bool is_last = rs->isLast();
      if (!is_first || ! is_last) {
        std::cout << "More than one key found in results in get_id_from_PhotoFile" << std::endl;
        std::cout << "isFirst(): " << rs->isFirst() << std::endl;
        std::cout << "isLast(): " << rs->isLast() << std::endl;
        // TODO do something better than exit here
        exit(1);
      }
      photoFile_key = rs->getInt64("id");
    } else {
      photoFile_key = -1;
    }
  }

  static inline sql::Driver *get_driver_instance() {
    return ::get_driver_instance();
  }

  //!
  //! Returns a connection to the database, or NULL if it can't get a connection
  static inline sql::Connection *get_connection(
      sql::Driver *driver, const std::string &url, const std::string &user,
      const std::string &password) {
    sql::Connection *connection;
    try {
      connection = driver->connect(url, user, password);
      connection->setAutoCommit(0);
    } catch (sql::SQLException &ex) {
      connection = NULL;
    }
    return connection;
  }

  //! database operation to remove a Photo from the database
  static inline void remove_photo_from_project_op(long project_id, long photo_file_id) {
    Db::enter_operation();
    std::cout << "remove photo from project "<< project_id << " " << photo_file_id << std::endl;
    std::string sql = "DELETE FROM ProjectPhotoFile "
        "WHERE (ProjectPhotoFile.projectId = ?) "
        "AND (ProjectPhotoFile.photoFileId= ?)";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
      prepared_statement->setInt64(1, project_id);
      prepared_statement->setInt64(2, photo_file_id);
      prepared_statement->execute();
  }

  static inline void delete_known_tag_op(const std::string &tag_name) {
    Db::enter_operation();
    std::string sql = "DELETE FROM Tag WHERE Tag.name=? ";
    std::unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setString(1,tag_name);
    prepared_statement->execute();
  }
};

#endif // DB_H__
