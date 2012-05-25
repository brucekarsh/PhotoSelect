#ifndef PHOTODBIMPORTER_H__
#define PHOTODBIMPORTER_H__

#include <exiv2/exiv2.hpp>
#include <fcntl.h>
#include <fts.h>
#include <sys/stat.h>

#include <crypto++/cryptlib.h>
#include <crypto++/hex.h>
#include <crypto++/ripemd.h>

#include <list>
#include <stdio.h>
#include <string>
#include <queue>

#include <boost/foreach.hpp>
#include <boost/regex.hpp>

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

class ImportWindow;

/**
 * Adds entries to the PhotoDb database for all files in a directory heirarchy
 */
class PhotoDbImporter {
  private:
  class ExifEntry {
    public:
    std::string name;
    std::string type_name;
    int count;
    std::string value;    
    ExifEntry(std::string name, std::string type_name, int count, std::string value)
        : name(name), type_name(type_name), count(count), value(value) {}
  };
  class PhotoDbEntry {
    public:
    std::string filePath;
    std::string checksum;
    std::list<ExifEntry> exifEntries;
    PhotoDbEntry(std::string filePath, std::string checksum) : filePath(filePath), checksum(checksum) {}

  };

  // PreparedStatements - a subclass that holds all the prepared statements used by PhotoDbImporter
  class PreparedStatements {
    public:
    sql::PreparedStatement *insert_into_ExifTag;
    sql::PreparedStatement *get_id_from_ExifTag;
    sql::PreparedStatement *insert_into_ExifTagValue;
    sql::PreparedStatement *insert_into_ExifTagName;
    sql::PreparedStatement *insert_into_Checksum;
    sql::PreparedStatement *get_id_from_Checksum;
    sql::PreparedStatement *insert_into_PhotoFile;
    sql::PreparedStatement *get_id_from_PhotoFile;
    sql::PreparedStatement *selectLastInsertId;
    void initialize(sql::Connection *connection) {
        insert_into_ExifTag = connection -> prepareStatement( "INSERT INTO ExifTag(ExifTagNameId,exifTagValueId,checksumId) Values (?,?,?)");
        get_id_from_ExifTag = connection -> prepareStatement( "SELECT id FROM ExifTag where ExifTagNameId = ? and ExifTagValueId = ? and checksumId = ?");

        insert_into_ExifTagValue = connection -> prepareStatement(
            "INSERT INTO ExifTagValue(typename, count, value, sha, nUpdates) " 
            "Values (?,?,?, sha(CONCAT_WS('/', sha(typename), sha(count), sha(value))),0) " 
            "ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id), nUpdates=nUpdates+1");

        insert_into_ExifTagName = connection -> prepareStatement( 
            "INSERT INTO ExifTagName(name, nUpdates) Values (?,0) ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id), nUpdates = nUpdates+1");

        insert_into_Checksum = connection -> prepareStatement( "INSERT IGNORE INTO Checksum(checksum) Values (?)");
        get_id_from_Checksum = connection -> prepareStatement("SELECT id FROM Checksum where checksum = ?");
        insert_into_PhotoFile = connection -> prepareStatement( "REPLACE INTO PhotoFile(filePath, checksumId) Values (?,?)");
        get_id_from_PhotoFile = connection -> prepareStatement( "SELECT id FROM PhotoFile where filePath = ? and checksumId = ?");
        selectLastInsertId = connection -> prepareStatement("Select LAST_INSERT_ID()");
    }
  } preparedStatements;

  boost::regex re_for_jpg_suffix() {
    return boost::regex(".*\\.jpg$", boost::regex::icase);
  }

  std::queue<std::string> dirs_to_process;

  public:
  std::list<PhotoDbEntry> photoDbEntries;
  std::list<std::string> errors;
  sql::Driver *driver;
  sql::Connection *connection;
  std::string url;
  std::string user;
  std::string password;
  std::string database;

  static std::string DEFAULT_DBHOST()   { return "localhost";};
  static std::string DEFAULT_USER()     { return "";};
  static std::string DEFAULT_PASSWORD() { return "";};
  static std::string DEFAULT_DATABASE() { return "PhotoSelect";};

  PhotoDbImporter() : connection(0)  {
  };

  void set_dirs_to_process(std::queue<std::string> dirs_to_process) {
    this -> dirs_to_process = dirs_to_process;
  }
  int count_files_to_process(ImportWindow* importWindow);
  int process_files(ImportWindow* importWindow, int file_count);


  int
  go_through_files(ImportWindow *importWindow)
  {
    int nfiles = count_files_to_process(importWindow);
    process_files(importWindow, nfiles);
  }

  int
  insert_into_database()
  {
   const int insert_rate = 100;
   int insert_count = 0;
    BOOST_FOREACH(PhotoDbEntry photoDbEntry, photoDbEntries) {

      int64_t checksum_key = insert_into_Checksum(photoDbEntry.checksum);


      int64_t photoFile_key =
          insert_into_PhotoFile(photoDbEntry.filePath, checksum_key);

      insert_into_exif_tables(photoDbEntry.exifEntries, checksum_key);
      photoDbEntry.exifEntries.clear();
      insert_count ++;
      if (insert_count >= insert_rate) {
          std::cout << "Committing " << insert_count << std::endl;
          connection -> commit();
          insert_count = 0;
      }
    }
    photoDbEntries.clear();
    std::cout << "Committing last " << insert_count << std::endl;
    connection -> commit();
    return 0;
  }

  void
  insert_into_exif_tables(const std::list<ExifEntry> &exifEntries, int64_t checksum_key)
  {
    std::cout << "insert_into_exif_tables entered, " << exifEntries.size() << " entries" << std::endl;
    BOOST_FOREACH(ExifEntry exifEntry, exifEntries) {
      int64_t exifTagName_key = insert_into_ExifTagName(exifEntry.name);
      int64_t exifTagValue_key =
          insert_into_ExifTagValue(exifEntry.type_name, exifEntry.count, exifEntry.value);
      insert_into_ExifTag(exifTagName_key, exifTagValue_key, checksum_key);
    }
  }

  void insert_into_ExifTag(int64_t exifTagName_key, int64_t exifTagValue_key, int64_t checksum_key)
  {
    // If it's already in the database, just return its id
    int64_t exifTag_key =
        get_id_from_ExifTag(exifTagName_key, exifTagValue_key, checksum_key);
    if (exifTag_key != -1) {
      return;
    }
    // It's not already in the database, so add it, find its id, and return it
    preparedStatements.insert_into_ExifTag -> setInt64(1, exifTagName_key);
    preparedStatements.insert_into_ExifTag -> setInt64(2, exifTagValue_key);
    preparedStatements.insert_into_ExifTag -> setInt64(3, checksum_key);
    int updateCount = preparedStatements.insert_into_ExifTag -> executeUpdate();

    exifTag_key =
        get_id_from_ExifTag(exifTagName_key, exifTagValue_key, checksum_key);
    if (exifTag_key == -1) {
      printf("insert failed in insert_into_Tag\n");
      exit(1);
    }
  }

  int64_t get_id_from_ExifTag(int64_t exifTagName_key, int64_t exifTagValue_key,
      int64_t checksum_key)
  {
    preparedStatements.get_id_from_ExifTag -> setInt64(1, exifTagName_key);
    preparedStatements.get_id_from_ExifTag -> setInt64(2, exifTagValue_key);
    preparedStatements.get_id_from_ExifTag -> setInt64(3, checksum_key);
    sql::ResultSet *rs  = preparedStatements.get_id_from_ExifTag -> executeQuery();
    bool has_first = rs -> first();
    if (has_first) {
      bool is_first = rs->isFirst();
      bool is_last = rs->isLast();
      if (!is_first || ! is_last) {
        printf("More than one key found in results in get_id_from_ExifTag\n");
        std::cout << "isFirst(): " << rs->isFirst() << std::endl;
        std::cout << "isLast(): " << rs->isLast() << std::endl;
        exit(1);
      }
      int64_t exifTag_key = rs->getInt64("id");
      return exifTag_key;
    } else {
      return -1;
    }
  }


  int64_t insert_into_ExifTagValue(const std::string &type_name, int count, const std::string &value) {
    preparedStatements.insert_into_ExifTagValue -> setString(1, type_name.c_str());
    preparedStatements.insert_into_ExifTagValue -> setInt64(2, count);
    preparedStatements.insert_into_ExifTagValue -> setString(3, value.c_str());
    int updateCount = preparedStatements.insert_into_ExifTagValue -> executeUpdate();
    return selectLastInsertId();
  }

  int64_t selectLastInsertId() {
    sql::ResultSet *rs = preparedStatements.selectLastInsertId -> executeQuery();
    bool has_first = rs -> first();
    if (has_first) {
      bool is_first = rs->isFirst();
      bool is_last = rs->isLast();
      if (!is_first || ! is_last) {
        printf("More than one key found in results in selectLastInsertId after insert_into_ExifTagValue\n");
        std::cout << "isFirst(): " << rs->isFirst() << std::endl;
        std::cout << "isLast(): " << rs->isLast() << std::endl;
        exit(1);
      }
      sql::ResultSetMetaData *rsmd = rs->getMetaData();
      int64_t exifTagValue_key = rs->getInt64(1);
      return exifTagValue_key;
    } else {
      return -1;
    }

  }

  int64_t insert_into_ExifTagName(const std::string &name) {
    preparedStatements.insert_into_ExifTagName -> setString(1, name.c_str());
    int updateCount = preparedStatements.insert_into_ExifTagName -> executeUpdate();
    return selectLastInsertId();
  }

  int64_t insert_into_Checksum(const std::string &checksum) {

    preparedStatements.insert_into_Checksum -> setString(1, checksum.c_str());
    int updateCount = preparedStatements.insert_into_Checksum -> executeUpdate();
    int64_t checksum_key = get_id_from_Checksum(checksum);
    return checksum_key;
  }

  int64_t get_id_from_Checksum(const std::string &checksum) {
    preparedStatements.get_id_from_Checksum -> setString(1, checksum.c_str());
    sql::ResultSet *rs  = preparedStatements.get_id_from_Checksum -> executeQuery();

    bool has_first = rs->first();
    if (!has_first) {
      printf("Cannot get a result set in insert_into_Checksum\n");
      exit(1);
    }
    bool is_first = rs->isFirst();
    bool is_last = rs->isLast();
    if (!is_first || ! is_last) {
      printf("More than one key found in results in insert_into_Checksum\n");
      std::cout << "isFirst(): " << rs->isFirst() << std::endl;
      std::cout << "isLast(): " << rs->isLast() << std::endl;
      exit(1);
    }
    int64_t checksum_key = rs->getInt64("id");
    return checksum_key;
  }

  int64_t insert_into_PhotoFile(const std::string &filePath, int64_t checksum_key) {

    // If it's already in the database, just return its id
    int64_t photoFile_key = get_id_from_PhotoFile(filePath, checksum_key);
    if (photoFile_key != -1) {
      return photoFile_key;
    }

    preparedStatements.insert_into_PhotoFile -> setString(1, filePath.c_str());
    preparedStatements.insert_into_PhotoFile -> setInt64(2, checksum_key);
    int updateCount = preparedStatements.insert_into_PhotoFile -> executeUpdate();

    photoFile_key = get_id_from_PhotoFile(filePath, checksum_key);
    return photoFile_key;
  }

  int64_t get_id_from_PhotoFile(const std::string &filePath, int64_t checksum_key) {
    preparedStatements.get_id_from_PhotoFile -> setString(1, filePath.c_str());
    preparedStatements.get_id_from_PhotoFile -> setInt64(2, checksum_key);
    sql::ResultSet *rs  = preparedStatements.get_id_from_PhotoFile -> executeQuery();
    bool has_first = rs -> first();
    if (has_first) {
      bool is_first = rs->isFirst();
      bool is_last = rs->isLast();
      if (!is_first || ! is_last) {
        printf("More than one key found in results in get_id_from_PhotoFile\n");
        std::cout << "isFirst(): " << rs->isFirst() << std::endl;
        std::cout << "isLast(): " << rs->isLast() << std::endl;
        exit(1);
      }
      int64_t photoFile_key = rs->getInt64("id");
      return photoFile_key;
    } else {
      return -1;
    }
  }
  
  void
  process_photo_file(const std::string &filename)
  {
    std::cout << filename << std::endl;
    std::string hashHexOutput;
  
    compute_picture_file_hash(filename, hashHexOutput);
    photoDbEntries.push_back(PhotoDbEntry(filename, hashHexOutput));
    try {
      std::cout << "calling process_exif(filename)" << std::endl;
      process_exif(filename);
    } catch(...) {
      std::cout << "Caught exception in process_photo_file, file was: " << filename << std::endl;
    }
    std::cout << "no exception" << std::endl;
  }
  
  void
  process_exif(const std::string &filename)
  {
    Exiv2::Image::AutoPtr exiv2Image;
    exiv2Image = Exiv2::ImageFactory::open(filename);
    if (0 != exiv2Image.get()) {
      exiv2Image->readMetadata();
      Exiv2::ExifData &exifData = exiv2Image->exifData();
  
      for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != exifData.end(); ++i) {
        const char* tn = i->typeName();
        if (i->value().size() > 40) continue;
  
        if(0) {
          std::cout << std::setw(44) << std::setfill(' ') << std::left
              << i->key() << " "
              << "0x" << std::setw(4) << std::setfill('0') << std::right
              << std::hex << i->tag() << " "
              << std::setw(9) << std::setfill(' ') << std::left
              << (tn ? tn : "Unknown") << " "
              << std::dec << std::setw(3)
              << std::setfill(' ') << std::right
              << i->count() << "  "
              << std::dec << i->value()
              << "\n";
          std::cout << "Add to table ExifTagName: name=" << i->key() << " tagnum=" << std::hex << i->tag()
              << std::endl;
          std::cout << "Add to table ExifTagValue: typename=" << i->typeName() << " count=" << i->count()
              << " value=" << i->value() << std::endl;
          std::cout << "Add to table Tag" << std::endl;
        }
        photoDbEntries.back().exifEntries.push_back(ExifEntry(
            i->key(), std::string(tn), i->count(), i->value().toString()));
      }
    } else {
      // TODO what to do if exiv2Image.get fails?
    }
  }
  
  void
  compute_picture_file_hash(const std::string& filename, std::string &hashHexOutput)
  {
    CryptoPP::RIPEMD160 hash;
    byte digest [CryptoPP::RIPEMD160::DIGESTSIZE];
    struct stat statbuf;
    long filelength = 0;
    byte *filecontents = 0;
  
    int ret = stat(filename.c_str(), &statbuf);
    if (ret == 0) {
      filelength = statbuf.st_size;
      filecontents = (byte *)malloc(filelength);
      if (0 == filecontents) {
        // TODO handle malloc failure
        abort();
      }
      int fd = open(filename.c_str(), O_RDONLY);
      if (fd < 0) {
        // TODO handle open failure
        abort();
      }
      ret = read(fd, filecontents, filelength);
      if (ret != filelength) {
        // TODO handle read failure
        abort();
      }
      close(fd);
      hash.CalculateDigest(digest, filecontents, filelength);
      free(filecontents);
      CryptoPP::HexEncoder encoder;
      encoder.Attach( new CryptoPP::StringSink( hashHexOutput ) );
      encoder.Put( digest, sizeof(digest) );
      encoder.MessageEnd();
    } else {
      // TODO handle stat failure
        abort();
    }
  }
  
  bool
  is_photo_file(char *filename)
  {
    return regex_match(filename, re_for_jpg_suffix());
  }
  
  int
  open_database(std::string dbhost, std::string user, std::string password, std::string database) {

    /* initiate url, user, password and database variables */
    std::cout << "Url " << url << " User " << user << " Password " << password << " Database " << database << std::endl;

    driver = get_driver_instance();
    if (0 == driver) {
      fprintf(stderr, "get_driver_instance() failed.\n");
      exit(1);
    }

    connection = driver -> connect(url, user, password);
    if (NULL == connection) {
      printf("driver -> connect() failed\n");
      exit(1);
      // TODO handle db open failure.
    }

    connection -> setAutoCommit(0);
    connection -> setSchema(database); 

    preparedStatements.initialize(connection);

    return 0;
  }
  
};


#include "ImportWindow.h"

  /** Count the number of files to process */
  int inline PhotoDbImporter::count_files_to_process(ImportWindow* importWindow) {
    const int process_rate = 50;
    int process_count = 0;
    int total_process_count = 0;

    // TODO go through all dirs in dirs_to_process
    std::queue<std::string> dirs_to_process_copy = dirs_to_process;
    std::string dir = dirs_to_process_copy.front();
    // TODO is there a way to get around this strdup and free?
    char *c_dir = strdup(dir.c_str());
    if (c_dir) {
      char * const fts_path_argv[2] = { c_dir, 0};
      FTS *ftsp = fts_open(fts_path_argv, FTS_LOGICAL | FTS_NOCHDIR, 0);
      free(c_dir);
      FTSENT * ftsentp = 0;
      while (ftsentp = fts_read(ftsp) ) {
        if (FTS_F == ftsentp->fts_info && is_photo_file(ftsentp->fts_name)) {
	  process_count++;
	  total_process_count++;
          if (process_count >= process_rate) {
	    process_count = 0;
            importWindow -> pulseProgressBar();
            importWindow -> runUI();
          }
        }
      }
    }
    return total_process_count;
  }

  int inline PhotoDbImporter::process_files(ImportWindow* importWindow, int file_count) {
    const int process_rate = 2;
    int process_count = 0;
    int total_process_count = 0;

    // TODO go through all dirs in dirs_to_process
    std::queue<std::string> dirs_to_process_copy = dirs_to_process;
    std::string dir = dirs_to_process_copy.front();
    // TODO is there a way to get around this strdup and free?
    char *c_dir = strdup(dir.c_str());
    if (c_dir) {
      char * const fts_path_argv[2] = { c_dir, 0};
      FTS *ftsp = fts_open(fts_path_argv, FTS_LOGICAL | FTS_NOCHDIR, 0);
      free(c_dir);
      FTSENT * ftsentp = 0;
      while (ftsentp = fts_read(ftsp) ) {
        if (FTS_F == ftsentp->fts_info && is_photo_file(ftsentp->fts_name)) {
	  importWindow -> display_on_UI(ftsentp->fts_path);
          process_photo_file(std::string(ftsentp->fts_path));
          process_count++;
          total_process_count++;
          if (process_count >= process_rate) {
            insert_into_database();
            process_count = 0;
            float fraction = (gdouble)total_process_count/(gdouble)file_count;
            importWindow -> setProgressBar(fraction);
            importWindow -> runUI();
          }
        }
      }
    }

    if (process_count >= process_rate) {
      insert_into_database();
      process_count = 0;
    }
    return 0;
  }
#endif // PHOTODBIMPORTER_H__
