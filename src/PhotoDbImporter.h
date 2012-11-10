#ifndef PHOTODBIMPORTER_H__
#define PHOTODBIMPORTER_H__

#include <boost/regex.hpp>
#include <list>
#include <queue>
#include <string>
#include "Db.h"
#include "XStr.h"

class ImportWindow;

/**
 * Adds entries to the PhotoDb database for all files in a directory heirarchy
 */
class PhotoDbImporter {
  private:
    class ExifEntry {
      public:
      std::string type_name;
      int count;
      std::string value;    
      ExifEntry(std::string type_name, int count, std::string value)
          : type_name(type_name), count(count), value(value) {}
    };
    class PhotoDbEntry {
      public:
      std::string filePath;
      std::string checksum;
      std::map<std::string, ExifEntry> exifEntries;
      PhotoDbEntry(std::string filePath, std::string checksum) : filePath(filePath),
          checksum(checksum) {}
  
    };

    boost::regex re_for_jpg_suffix();

  public:
    Db db;
    std::queue<std::string> dirs_to_process;
    std::list<PhotoDbEntry> photoDbEntries;
    std::list<std::string> errors;
    std::string user;
    std::string password;
    std::string database;
    std::string import_time;
    std::string import_timezone;

    PhotoDbImporter();

    void set_dirs_to_process(std::queue<std::string> dirs_to_process);
    int count_files_to_process(ImportWindow* importWindow);
    int process_files(ImportWindow* importWindow, int file_count); 
    void go_through_files(ImportWindow *importWindow); 
    bool insert_into_database_transaction(); 
    void insert_into_database_op();
    void insert_into_exif_tables(const std::map<std::string, ExifEntry> &exifEntries,
        int64_t checksum_key);
    std::string exif_datetime_to_mysql_datetime(const std::string exif_datetime);
    /** make a string containing an xml representation of the ExifEntries */
    std::string make_exif_xml_string(const std::map<std::string, ExifEntry> &exifEntries);
    void process_photo_file(const std::string &filename);
    void process_exif(const std::string &filename);
    void compute_picture_file_hash(const std::string& filename, std::string &hashHexOutput);
    bool is_photo_file(char *filename);
};
#endif // PHOTODBIMPORTER_H__
