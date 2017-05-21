#include "Db.h"

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

#include <boost/bind.hpp>

using namespace std;

Db::Db() : connection(NULL), transaction_is_running(false) {}

Db::~Db() {
  if (NULL != connection) {
    close_connection();
  }
}

void Db::close_connection() {
  BOOST_ASSERT(NULL != connection);
  delete connection;
  connection = NULL;
}

void Db::enter_operation() {
  if (!transaction_is_running) {
    cout << "Operation was called from outside a transaction, exiting." << endl;
    abort();
  }
}

bool Db::transaction(const boost::function<void(void)> &f) {
  // First make sure that we are not already in a transaction. We don't allow nested
  // transactions
  if (transaction_is_running) {
    cout << "Nested Transaction detected, exiting." << endl;
    exit(1);
  }
  transaction_is_running = true;

  bool succeed = false;
  for (int retry_count = 0; retry_count < NRETRIES; retry_count++) {
    sql::Connection *connection = get_connection();
    if (NULL == connection) {
      cout << "get_connection failed" << endl;
      abort();
    }

    succeed = true;
    try {
      f();
    } catch (sql::SQLException &ex) {
      cout << "exception during transaction " << ex.what() << ex.getSQLState()
          << " " << ex.getErrorCode()<< endl;
      succeed = false;
    }
    if (succeed) {
      try { 
        connection->commit();
      } catch (sql::SQLException &ex) {
      cout << "exception during commit " << ex.what() << ex.getSQLState()
          << " " << ex.getErrorCode()<< endl;
      succeed = false;
      }
      break;
    } else {
      try {
        connection->rollback();
      } catch (sql::SQLException &ex) {
      // the rollback failed. Probably because the connection is gone.
      cout << "exception during rollback " << ex.what() << ex.getSQLState()
          << " " << ex.getErrorCode()<< endl;
      }
    }
    // close the connection here. It will be repoened next time get_connection is called.
    // TODO don't close it if we don't have to.
    cout << "Retrying transaction " << retry_count << endl;
    close_connection();
  }
  transaction_is_running = false;
  return succeed;
}

sql::Connection *Db::get_connection() {
  if (connection) {
    return connection;
  }
  // TODO don't get the driver instance over and over. Just get it once.
  /* initiate url, user, password and database variables */
  sql::Driver *driver = get_driver_instance();
  if (0 == driver) {
    cerr <<  "get_driver_instance() failed.\n" << endl;
    // TODO do something better than exit here
    exit(1);
  }

  sql::Connection *connection = get_connection(driver, dbhost, user, password);
  if (NULL == connection) {
    cerr << "driver -> connect() failed\n" << endl;
  } else {
    connection->setSchema(database);
    Db::connection = connection;
  }
  return connection;
}

void Db::get_photo_tags_op(const string &project_name, const string &file_name, set<string> &tags) {
  enter_operation();
  tags.clear();
  string sql = "SELECT Tag.name FROM Tag "
      "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
      "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
      "INNER JOIN TagChecksum ON (TagChecksum.tagId = Tag.id) "
      "INNER JOIN PhotoFile ON (TagChecksum.checksumId = PhotoFile.checksumId) "
      "WHERE (Project.name = ? and PhotoFile.filePath = ?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, project_name);
  prepared_statement->setString(2, file_name);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  while (rs->next()) {
    string name = rs->getString(1);
    tags.insert(name);
  }
};

bool Db::get_photo_tags_transaction(const string &project_name,
    const string &file_name, set<string> &result) {
  boost::function<void (void)> f = boost::bind(&Db::get_photo_tags_op, this,
      boost::cref(project_name), boost::cref(file_name), boost::ref(result));
  return transaction(f);
}

typedef map<string, set<string> > all_photo_tags_map_t;
typedef pair<string, set<string> > all_photo_tags_map_entry_t;
void Db::get_all_photo_tags_for_project_op(const string &project_name,
    all_photo_tags_map_t &result) {
  enter_operation();
  result.clear();
  string sql = "SELECT PhotoFile.filePath, Tag.name "
      "FROM Project "
      "INNER JOIN ProjectPhotoFile ON (Project.id = ProjectPhotoFile.projectId) "
      "INNER JOIN PhotoFile ON (ProjectPhotoFile.photoFileId = PhotoFile.id) "
      "INNER JOIN TagChecksum ON (PhotoFile.checksumId = TagChecksum.checksumId) "
      "INNER JOIN Tag ON (Tag.id = TagChecksum.tagId) "
      "WHERE (Project.name = ?) "
      "ORDER BY PhotoFile.filePath ";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, project_name);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  while (rs->next()) {
    string filePath = rs->getString(1);
    string tag = rs->getString(2);
    result[filePath].insert(tag);
  }
  return;
}

bool Db::get_all_photo_tags_for_project_transaction(const string &project_name,
    all_photo_tags_map_t &result) {
  boost::function<void (void)> f = boost::bind(&Db::get_all_photo_tags_for_project_op,
      this, boost::cref(project_name), boost::ref(result));
  bool b = transaction(f);
  return b;
}

void Db::get_project_tags_op(const string &project_name, set<string> &tags) {
  enter_operation();
  tags.clear();
  string sql = "SELECT Tag.name FROM Tag "
      "INNER JOIN ProjectTag ON (Tag.id = ProjectTag.tagId) "
      "INNER JOIN Project ON (ProjectTag.projectId = Project.id) "
      "WHERE (Project.name = ?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, project_name);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  while (rs->next()) {
    string name = rs->getString(1);
    tags.insert(name);
  } 
}

bool Db::get_project_tags_transaction(const string &project_name, set<string> &tags) {
  boost::function<void (void)> f = boost::bind(&Db::get_project_tags_op, this,
      boost::cref(project_name), boost::ref(tags));
  bool b = transaction(f);
  return b;
}

void Db::get_all_tags_op(set<string> &tags) {
  enter_operation();
  tags.clear();
  string sql = "SELECT DISTINCT Tag.name FROM Tag";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  while (rs->next()) {
    string name = rs->getString(1);
    tags.insert(name);
  }
}

bool Db::get_all_tags_transaction(set<string>& tags) {
  boost::function<void (void)> f = boost::bind(&Db::get_all_tags_op, this, boost::ref(tags));
  bool b = transaction(f);
  return b;
}

void Db::insert_tag_op(const string &tag_name) {
  enter_operation();
  string sql = "INSERT INTO Tag(name) VALUE(?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, tag_name);
  prepared_statement->execute();
}

bool Db::insert_tag_transaction(const string &tag_name) {
  boost::function<void (void)> f = boost::bind(&Db::insert_tag_op, this, boost::cref(tag_name));
  bool b = transaction(f);
  return b;
}

void Db::delete_project_tag_op(const string &tag_name, const string &project_name) {
  enter_operation();
  string sql = "DELETE FROM ProjectTag USING Tag, Project, ProjectTag "
      "WHERE Tag.name=? "
      "AND Project.name=? "
      "AND ProjectTag.projectId = Project.id "
      "AND ProjectTag.tagId = Tag.id";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1,tag_name);
  prepared_statement->setString(2,project_name);
  prepared_statement->execute();
}

bool Db::delete_project_tag_transaction(const string &tag_name, const string &project_name) {
  boost::function<void (void)> f = boost::bind(&Db::delete_project_tag_op, this,
      boost::cref(tag_name), boost::cref(project_name));
  bool b = transaction(f);
  return b;
}

void Db::insert_project_tag_op(const string &tag_name, const string &project_name) {
  enter_operation();
  string sql = "INSERT INTO ProjectTag(tagId, projectId, hasValue) "
      "SELECT Tag.id, Project.id, ? FROM Tag, Project WHERE Tag.name=? AND Project.name=?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setBoolean(1, false);
  prepared_statement->setString(2,tag_name);
  prepared_statement->setString(3,project_name);
  prepared_statement->execute();
}

bool Db::insert_project_tag_transaction(const string &tag_name, const string &project_name) {
  boost::function<void (void)> f = boost::bind(&Db::insert_project_tag_op, this,
      boost::cref(tag_name), boost::cref(project_name));
  bool b = transaction(f);
  return b;
}

void Db::get_rotation_op(const string &photoFileName, int &angle) {
  enter_operation();
  angle = 0;
  string sql = 
      "SELECT Rotation.angle "
      "FROM Rotation INNER JOIN Checksum on (Checksum.id = Rotation.checksumId) "
      "INNER JOIN PhotoFile ON (Checksum.id=PhotoFile.checksumId) "
      "WHERE PhotoFile.filePath=?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, photoFileName);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  if (rs->next()) {
    angle = rs->getDouble(1);
  }
}

bool Db::get_rotation_transaction(const string &photoFileName, int &angle) {
  boost::function<void (void)> f = boost::bind(&Db::get_rotation_op, this,
      boost::cref(photoFileName), boost::ref(angle));
  bool b = transaction(f);
  return b;
}

void Db::set_rotation_op(const string &photoFileName, int rotation) {
  enter_operation();
  string sql = "INSERT INTO Rotation (checksumId, angle) "
      "SELECT Checksum.id, ? "
      "FROM Checksum INNER JOIN PhotoFile ON (Checksum.id=PhotoFile.checksumId) "
      "WHERE PhotoFile.filePath=? "
      "ON DUPLICATE KEY UPDATE angle=?";
    unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setDouble(1, rotation);
    prepared_statement->setString(2, photoFileName);
    prepared_statement->setDouble(3, rotation);
    prepared_statement->execute();
}

bool Db::set_rotation_transaction(const string &photoFileName, int rotation) {
  boost::function<void (void)> f = boost::bind(&Db::set_rotation_op, this,
      boost::cref(photoFileName), rotation);
  return transaction(f);
}

void Db::insert_into_project_op(const string &project_name, long &project_id) {
  enter_operation();
  string project_insert_sql = "INSERT INTO Project (name) VALUES(?)";
  unique_ptr<sql::PreparedStatement> project_insert_prepared_statement(
      connection->prepareStatement(project_insert_sql));

  string get_last_id_sql = "SELECT LAST_INSERT_ID() as id";
  unique_ptr<sql::PreparedStatement> get_last_id_prepared_statement(
      connection->prepareStatement(get_last_id_sql));

  project_id = -1;
  project_insert_prepared_statement->setString(1, project_name);
  project_insert_prepared_statement->execute();
  unique_ptr<sql::ResultSet> rs(get_last_id_prepared_statement->executeQuery());
  if (rs->next()) {
    project_id = rs->getInt64("id");
  }
}

void Db::get_project_id_op(const string &project_name, long &project_id) {
  enter_operation();
  string sql = "SELECT id from Project where name = ?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));

  project_id = -1;
  prepared_statement->setString(1,project_name);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  if (rs->next()) {
    project_id = rs->getInt64("id");
  }
}

void Db::get_project_names_op(list<string> &project_names) {
  enter_operation();
  string sql = "SELECT DISTINCT name FROM Project ";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  project_names.clear();
  while (rs->next()) {
    string project_name = rs->getString(1);
    project_names.push_back(project_name);
  }
}

bool Db::get_project_names_transaction(list<string> &project_names) {
  boost::function<void (void)> f = boost::bind(&Db::get_project_names_op, this,
      boost::ref(project_names));
  return transaction(f);
}

void Db::delete_project_op(const string &project_name) {
  enter_operation();
  string sql = "DELETE FROM Project WHERE name = ?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, project_name);
  prepared_statement->execute();
}

bool Db::delete_project_transaction(const string &project_name) {
  boost::function<void (void)> f = boost::bind(&Db::delete_project_op, this,
      boost::cref(project_name));
  return transaction(f);
}

void Db::rename_project_op(const string &old_project_name, const string &new_project_name) {
  enter_operation();
  string sql = "UPDATE Project SET name=? WHERE name=?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, new_project_name);
  prepared_statement->setString(2, old_project_name);
  prepared_statement->execute();
}

bool Db::rename_project_transaction(const string &old_project_name,
    const string &new_project_name) {
  boost::function<void (void)> f = boost::bind(&Db::rename_project_op, this,
      boost::cref(old_project_name), boost::cref(new_project_name));
  return transaction(f);
}

void Db::add_photo_to_project_op(long project_id, long photo_file_id) {
  enter_operation();
  string sql = "INSERT INTO ProjectPhotoFile (projectId, photoFileId) VALUES(?,?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setInt64(1, project_id);
  prepared_statement->setInt64(2, photo_file_id);
  prepared_statement->execute();
}

void Db::get_project_photo_files_op(const string &project_name,
    vector<string> &project_photo_files, vector<string> &project_adjusted_date_times) {
  enter_operation();
  if (NULL == connection) {
    cout << "get_project_photo_files_op null connection" << endl;
    abort();
  }

  string sql =
      "SELECT DISTINCT filePath, Time.adjustedDateTime FROM Project "
      "INNER JOIN ProjectPhotoFile ON (ProjectPhotoFile.projectId = Project.id) "
      "INNER JOIN PhotoFile ON (ProjectPhotoFile.photoFileId = PhotoFile.id) "
      "INNER JOIN Time ON (PhotoFile.checksumId = Time.checksumId) "
      "WHERE Project.name = ? "
      "ORDER by Time.adjustedDateTime, filePath ";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, project_name);
  cout << "executing statement " << sql << endl;
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  project_photo_files.clear();
  project_adjusted_date_times.clear();
  while ( rs->next()) {
    string file_path = rs->getString(1);
    string adjusted_date_time = rs->getString(2);
    project_photo_files.push_back(file_path);
    project_adjusted_date_times.push_back(adjusted_date_time);
  }
}

bool Db::get_project_photo_files_transaction(const string &project_name,
    vector<string> &project_photo_files, vector<string> &project_adjusted_date_times) {
  boost::function<void (void)> f = boost::bind(&Db::get_project_photo_files_op, this,
      boost::cref(project_name),
      boost::ref(project_photo_files),
      boost::ref(project_adjusted_date_times));
  return transaction(f);
}

void Db::add_tag_by_filename_op(const string &tag_name, const string &file_name) {
  enter_operation();
  string sql = "INSERT INTO TagChecksum (tagId, checksumId) "
      "SELECT DISTINCT Tag.id as tagId, Checksum.id as checksumId "
      "FROM Tag, Checksum, PhotoFile "
      "WHERE Tag.name = ? AND PhotoFile.filePath = ? AND Checksum.id = PhotoFile.checksumId";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, tag_name);
  prepared_statement->setString(2, file_name);
  prepared_statement->execute();
}

bool Db::add_tag_by_filename_transaction(const string &tag_name, const string &file_name) {
  boost::function<void (void)> f = boost::bind(&Db::add_tag_by_filename_op, this,
      boost::cref(tag_name), boost::cref(file_name));
  return transaction(f);
}

void Db::remove_tag_by_filename_op(const string &tag_name, const string &file_name) {
  enter_operation();
  string sql = "DELETE FROM TagChecksum "
      "USING Tag, Checksum, PhotoFile, TagChecksum "
      "WHERE Tag.name = ? AND PhotoFile.filePath = ? "
      "AND Checksum.id = PhotoFile.checksumId AND TagChecksum.checksumId=Checksum.id "
      "AND TagChecksum.tagId=Tag.id";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, tag_name);
  prepared_statement->setString(2, file_name);
  prepared_statement->execute();
}

bool Db::remove_tag_by_filename_transaction(const string &tag_name, const string &file_name) {
  boost::function<void (void)> f = boost::bind(&Db::remove_tag_by_filename_op, this,
      boost::cref(tag_name), boost::cref(file_name));
  return transaction(f);
}


void Db::insert_into_time_op(long checksum_id, const string &original_date_time,
    const string &adjusted_date_time) {
  enter_operation();
  string sql = "INSERT IGNORE INTO Time(checksumId, originalDateTime, adjustedDateTime ) "
      "Values(?,?,?) "
      "ON DUPLICATE KEY UPDATE originalDateTime=?, adjustedDateTime=?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setInt64(1, checksum_id);
  prepared_statement->setString(2, original_date_time);
  prepared_statement->setString(3, adjusted_date_time);
  prepared_statement->setString(4, original_date_time);
  prepared_statement->setString(5, adjusted_date_time);
  prepared_statement->executeUpdate();
}

void Db::get_adjusted_date_time_op(const string &filename, string& adjusted_date_time, bool &ret) {
  enter_operation();
  string sql = "SELECT adjustedDateTime from PhotoFile "
      "INNER JOIN Time ON (PhotoFile.checksumId = Time.checksumId) WHERE PhotoFile.filePath = ?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, filename);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  bool has_first = rs->first();
  if (!has_first) {
    cout << "Cannot get a result set in get_adjusted_date_time" << endl;
    ret = false;
    return;
  }
  bool is_first = rs->isFirst();
  bool is_last = rs->isLast();
  if (!is_first || ! is_last) {
    cout << "More than one key found in results in get_adjusted_date_time" << endl;
    cout << "isFirst(): " << rs->isFirst() << endl;
    cout << "isLast(): " << rs->isLast() << endl;
    ret = false;
    return;
  }
  adjusted_date_time = rs->getString(1);
  ret = true;
}

void Db::insert_into_exifblob_op(long checksum_id, const string &xml_string) {
  enter_operation();
  string sql = "INSERT IGNORE INTO ExifBlob(checksumId, value) Values (?,?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setInt64(1, checksum_id);
  prepared_statement->setString(2, xml_string);
  prepared_statement->executeUpdate();
}

void Db::get_from_exifblob_by_filePath_op(const string &filePath, string &value) {
  enter_operation();
  string sql = "SELECT value from PhotoFile "
      "INNER JOIN ExifBlob ON (PhotoFile.checksumId = ExifBlob.checksumId) "
      "WHERE PhotoFile.filePath = ?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, filePath);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  bool has_first = rs->first();
  if (!has_first) {
    cout << "Cannot get a result set in get_from_exifblob_by_filePath" << endl;
    // TODO do something better than exit here
    exit(1);
  }
  bool is_first = rs->isFirst();
  bool is_last = rs->isLast();
  if (!is_first || ! is_last) {
    cout << "More than one key found in results in get_id_from_Checksum" << endl;
    cout << "isFirst(): " << rs->isFirst() << endl;
    cout << "isLast(): " << rs->isLast() << endl;
    // TODO do something better than exit here
    exit(1);
  }
  value = rs->getString(1);
}

bool Db::get_from_exifblob_by_filePath_transaction(const string &filePath, string &value) {
  boost::function<void (void)> f = boost::bind(&Db::get_from_exifblob_by_filePath_op, this,
      boost::cref(filePath), boost::ref(value));
  return transaction(f);
}

int64_t Db::insert_into_Checksum_op(const string &checksum, int64_t &checksum_key) {
  enter_operation();
  string sql = "INSERT IGNORE INTO Checksum(checksum) Values (?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, checksum.c_str());
  prepared_statement->executeUpdate();
  get_id_from_Checksum_op(checksum, checksum_key);
  return checksum_key;
}

void Db::get_id_from_Checksum_op(const string &checksum, int64_t &checksum_key) {
  enter_operation();
  string sql = "SELECT id FROM Checksum where checksum = ?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, checksum);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());

  bool has_first = rs->first();
  if (!has_first) {
    cout << "Cannot get a result set in get_id_from_Checksum" << endl;
    // TODO do something better than exit
    exit(1);
  }
  bool is_first = rs->isFirst();
  bool is_last = rs->isLast();
  if (!is_first || ! is_last) {
    cout << "More than one key found in results in get_id_from_Checksum" << endl;
    cout << "isFirst(): " << rs->isFirst() << endl;
    cout << "isLast(): " << rs->isLast() << endl;
    // TODO do something better than exit
    exit(1);
  }
  checksum_key = rs->getInt64("id");
}

void Db::insert_into_PhotoFile_op(
    const string &filePath, const int64_t &checksum_key, int64_t &photoFile_key) {
  enter_operation();
  // If it's already in the database, just return its id
  get_id_from_PhotoFile_op(filePath, checksum_key, photoFile_key);
  if (photoFile_key != -1) {
    return;
  }

  string sql = "REPLACE INTO PhotoFile(filePath, checksumId) Values (?,?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, filePath.c_str());
  prepared_statement->setInt64(2, checksum_key);
  prepared_statement->executeUpdate();
  get_id_from_PhotoFile_op(filePath, checksum_key, photoFile_key);
}

void Db::get_id_from_PhotoFile_op(const string &filePath,
    int64_t checksum_key, int64_t &photoFile_key) {
  enter_operation();
  string sql = "SELECT id FROM PhotoFile where filePath = ? and checksumId = ?";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1, filePath.c_str());
  prepared_statement->setInt64(2, checksum_key);
  unique_ptr<sql::ResultSet> rs(prepared_statement->executeQuery());
  bool has_first = rs -> first();
  if (has_first) {
    bool is_first = rs->isFirst();
    bool is_last = rs->isLast();
    if (!is_first || ! is_last) {
      cout << "More than one key found in results in get_id_from_PhotoFile" << endl;
      cout << "isFirst(): " << rs->isFirst() << endl;
      cout << "isLast(): " << rs->isLast() << endl;
      // TODO do something better than exit here
      exit(1);
    }
    photoFile_key = rs->getInt64("id");
  } else {
    photoFile_key = -1;
  }
}

sql::Driver *Db::get_driver_instance() {
  return ::get_driver_instance();
}

sql::Connection *Db::get_connection( sql::Driver *driver, const string &url, const string &user,
    const string &password) {
  sql::Connection *connection;
  try {
    connection = driver->connect(url, user, password);
    connection->setAutoCommit(0);
  } catch (sql::SQLException &ex) {
    connection = NULL;
  }
  return connection;
}

void Db::remove_photo_from_project_op(long project_id, long photo_file_id) {
  enter_operation();
  cout << "remove photo from project "<< project_id << " " << photo_file_id << endl;
  string sql = "DELETE FROM ProjectPhotoFile "
      "WHERE (ProjectPhotoFile.projectId = ?) "
      "AND (ProjectPhotoFile.photoFileId= ?)";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
    prepared_statement->setInt64(1, project_id);
    prepared_statement->setInt64(2, photo_file_id);
    prepared_statement->execute();
}

void Db::delete_known_tag_op(const string &tag_name) {
  enter_operation();
  string sql = "DELETE FROM Tag WHERE Tag.name=? ";
  unique_ptr<sql::PreparedStatement> prepared_statement(connection->prepareStatement(sql));
  prepared_statement->setString(1,tag_name);
  prepared_statement->execute();
}
