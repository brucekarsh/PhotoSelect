#ifndef DB_H__
#define DB_H__

/* MySQL Connector/C++ specific headers */
#include <driver.h>
#include <connection.h>

#include <list>
#include <map>
#include <set>
#include <boost/function.hpp>

//! A class to hold database access procedures.
//! Most of the methods are either database transactions (method name ends in _transaction) or
//! database operations (method name ends in _op). A transaction executes a sequence of database
//! operations as an SQL transaction with retry and reacquistion of the connection.

class Db {
  public:
    static const int NRETRIES = 4;
    sql::Connection *connection;
    bool transaction_is_running;
    static std::string dbhost;
    static std::string user;
    static std::string password;
    static std::string database;

    Db();
    ~Db();

    void close_connection();

    //! this method is called before every _op procedure. It exits if the procedure was called
    //! from outside of a transaction.
    void enter_operation();

    //! Issue a transaction with retries and reaquisition of a connection
    //! Sometimes a transaction will fail due to a transient cause, e.g. deadlock or database
    //! server restart. This procedure issues a transaction repeatedly until the transaction
    //! succeeds. On each failure, it reopens the connection. If after retrying NRETRIES times
    //! it still has failed, it returns false. Otherwise it returns true.
    //! \param f a function that issues the database operations of the trasaction.
    bool transaction(const boost::function<void(void)> &f);

    sql::Connection *get_connection();
    void get_photo_tags_op(const std::string &project_name,
        const std::string &file_name, std::set<std::string> &tags) ;
    bool get_photo_tags_transaction(const std::string &project_name,
        const std::string &file_name, std::set<std::string> &result);
    typedef std::map<std::string, std::set<std::string> > all_photo_tags_map_t;
    typedef std::pair<std::string, std::set<std::string> > all_photo_tags_map_entry_t;
    void get_all_photo_tags_for_project_op(const std::string &project_name,
        all_photo_tags_map_t &result);
    bool get_all_photo_tags_for_project_transaction(const std::string &project_name,
        all_photo_tags_map_t &result);
    void get_project_tags_op(const std::string &project_name, std::set<std::string> &tags);
    bool get_project_tags_transaction(const std::string &project_name, std::set<std::string> &tags);
    void get_all_tags_op(std::set<std::string> &tags);
    bool get_all_tags_transaction(std::set<std::string>& tags);
    void insert_tag_op(const std::string &tag_name);
    bool insert_tag_transaction(const std::string &tag_name);
    void delete_project_tag_op(const std::string &tag_name, const std::string &project_name);
    bool delete_project_tag_transaction(const std::string &tag_name,
        const std::string &project_name);
    void insert_project_tag_op(const std::string &tag_name, const std::string &project_name);
    bool insert_project_tag_transaction(const std::string &tag_name,
        const std::string &project_name);
    void get_rotation_op(const std::string &photoFileName, int &angle);
    bool get_rotation_transaction(const std::string &photoFileName, int &angle);
    void set_rotation_op(const std::string &photoFileName, int rotation);
    bool set_rotation_transaction(const std::string &photoFileName, int rotation);

  //! database operation to insert a record into the Project table
  void insert_into_project_op(const std::string &project_name, long &project_id);
  void get_project_id_op(const std::string &project_name, long &project_id);
  void get_project_names_op(std::list<std::string> &project_names);
  bool get_project_names_transaction(std::list<std::string> &project_names);
  void delete_project_op(const std::string &project_name);
  bool delete_project_transaction(const std::string &project_name);
  void rename_project_op(const std::string &old_project_name, const std::string &new_project_name);
  bool rename_project_transaction(const std::string &old_project_name,
      const std::string &new_project_name);

    //! database operation to add a photo file to a project.
    void add_photo_to_project_op(long project_id, long photo_file_id);

    //! Get all photo files for a project
    void get_project_photo_files_op(
        const std::string &project_name, std::vector<std::string> &project_photo_files,
        std::vector<std::string> &project_adjusted_date_times);
    bool get_project_photo_files_transaction(
        const std::string &project_name, std::vector<std::string> &project_photo_files,
        std::vector<std::string> &project_adjusted_date_times);

    //! adds a tag to a checksum given a tag_name and a file_name
    void add_tag_by_filename_op(const std::string &tag_name, const std::string &file_name);
    bool add_tag_by_filename_transaction(const std::string &tag_name, const std::string &file_name);

    //! removes a tag to a checksum given a tag_name and a file_name
    void remove_tag_by_filename_op(const std::string &tag_name, const std::string &file_name);
    bool remove_tag_by_filename_transaction(const std::string &tag_name,
        const std::string &file_name);

    //! database operation to insert a record into the Time table
    void insert_into_time_op(long checksum_id, const std::string &original_date_time,
        const std::string &adjusted_date_time);
    void get_adjusted_date_time_op(const std::string &filename,
        std::string& adjusted_date_time, bool &ret);

    //! database operation to insert an entry into the ExifBlob table.
    void insert_into_exifblob_op(long checksum_id, const std::string &xml_string);
    void get_from_exifblob_by_filePath_op(const std::string &filePath, std::string &value);
    bool get_from_exifblob_by_filePath_transaction(const std::string &filePath, std::string &value);

    //! database operation to insert a checksum into the Checksum table
    int64_t insert_into_Checksum_op(
        const std::string &checksum, int64_t &checksum_key);
    void get_id_from_Checksum_op(const std::string &checksum, int64_t &checksum_key);

    //! database operation to insert a record into the PhotoFile table
    //! does not insert and just returns the PhotoFileId if duplicate
    void insert_into_PhotoFile_op(
        const std::string &filePath, const int64_t &checksum_key, int64_t &photoFile_key);
    void get_id_from_PhotoFile_op(const std::string &filePath,
        int64_t checksum_key, int64_t &photoFile_key);
    sql::Driver *get_driver_instance();

    //! Returns a connection to the database, or NULL if it can't get a connection
    sql::Connection *get_connection(
        sql::Driver *driver, const std::string &url, const std::string &user,
        const std::string &password);

    //! database operation to remove a Photo from the database
    void remove_photo_from_project_op(long project_id, long photo_file_id);
    void delete_known_tag_op(const std::string &tag_name);
};
#endif // DB_H__
