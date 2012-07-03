#ifndef PHOTODBIMPORTER_H__
#define PHOTODBIMPORTER_H__

#include <exiv2/exiv2.hpp>
#include <fcntl.h>
#include <fts.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/timeb.h>

#include <crypto++/cryptlib.h>
#include <crypto++/hex.h>
#include <crypto++/ripemd.h>

#include <list>
#include <stdio.h>
#include <string>
#include <queue>
#include <sstream>

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

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <iostream>

#include "XStr.h"

XERCES_CPP_NAMESPACE_USE

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

  // PreparedStatements - a subclass that holds all the prepared statements used by PhotoDbImporter
  class PreparedStatements {
    public:

    sql::PreparedStatement *insert_into_Checksum;
    sql::PreparedStatement *get_id_from_Checksum;
    sql::PreparedStatement *insert_into_PhotoFile;
    sql::PreparedStatement *get_id_from_PhotoFile;
    sql::PreparedStatement *insert_into_ExifBlob;
    sql::PreparedStatement *insert_into_Time;

    void initialize(sql::Connection *connection) {
std::cout << "A" << std::endl;
      insert_into_Checksum = connection -> prepareStatement(
          "INSERT IGNORE INTO Checksum(checksum) Values (?)");
std::cout << "B" << std::endl;
      get_id_from_Checksum = connection -> prepareStatement(
          "SELECT id FROM Checksum where checksum = ?");
std::cout << "C" << std::endl;
      insert_into_PhotoFile = connection -> prepareStatement(
          "REPLACE INTO PhotoFile(filePath, checksumId) Values (?,?)");
std::cout << "D" << std::endl;
      get_id_from_PhotoFile = connection -> prepareStatement(
          "SELECT id FROM PhotoFile where filePath = ? and checksumId = ?");
std::cout << "E" << std::endl;
      insert_into_ExifBlob = connection -> prepareStatement(
          "INSERT IGNORE INTO ExifBlob(checksumId, value) Values (?,?)");
std::cout << "F" << std::endl;
      insert_into_Time = connection -> prepareStatement(
          "INSERT IGNORE INTO "
          "Time(checksumId, originalDateTime, adjustedDateTime ) "
          "Values(?,?,?)");
std::cout << "G" << std::endl;
    }
  } preparedStatements;

  boost::regex re_for_jpg_suffix() {
    return boost::regex(".*\\.jpg$", boost::regex::icase);
  }

  std::queue<std::string> dirs_to_process;

  public:
  std::list<PhotoDbEntry> photoDbEntries;
  std::list<std::string> errors;
  std::string user;
  std::string password;
  std::string database;
  std::string import_time;
  std::string import_timezone;
  sql::Connection *connection;

  static std::string DEFAULT_DBHOST()   { return "localhost";};
  static std::string DEFAULT_USER()     { return "";};
  static std::string DEFAULT_PASSWORD() { return "";};
  static std::string DEFAULT_DATABASE() { return "PhotoSelect";};

  PhotoDbImporter(sql::Connection *connection_) : connection(connection_)  {
    struct timeb now_timeb;
    ftime(&now_timeb); 
    time_t now_time_t = now_timeb.time;
    struct tm now_tm;
    localtime_r(&now_time_t, &now_tm);
    char buf[20];
    snprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d",
        1900+now_tm.tm_year,
        now_tm.tm_mon,
        now_tm.tm_mday,
        now_tm.tm_hour,
        now_tm.tm_min,
        now_tm.tm_sec,
        now_tm.tm_isdst);
    import_time = buf;
    import_timezone = tzname[now_tm.tm_isdst];
    std::cout << "import_time " << import_time << std::endl;
    std::cout << "import_timezone " << import_timezone << std::endl;
    preparedStatements.initialize(connection);
  };

  void set_dirs_to_process(std::queue<std::string> dirs_to_process) {
    this -> dirs_to_process = dirs_to_process;
  }
  int count_files_to_process(ImportWindow* importWindow);
  int process_files(ImportWindow* importWindow, int file_count);


  void
  go_through_files(ImportWindow *importWindow)
  {
    int nfiles = count_files_to_process(importWindow);
    if (importWindow -> is_cancel_requested()) {
      return;
    }
    process_files(importWindow, nfiles);
  }

  int
  insert_into_database() {
    BOOST_FOREACH(PhotoDbEntry photoDbEntry, photoDbEntries) {
      int64_t checksum_key = insert_into_Checksum(photoDbEntry.checksum);
      int64_t photoFile_key = insert_into_PhotoFile(photoDbEntry.filePath, checksum_key);

      insert_into_exif_tables(photoDbEntry.exifEntries, checksum_key);
      photoDbEntry.exifEntries.clear();
    }
    photoDbEntries.clear();
    connection -> commit();
    return 0;
  }


#define X(str) XStr(str).unicodeForm()
  void
  insert_into_exif_tables(const std::map<std::string, ExifEntry> &exifEntries,
      int64_t checksum_key) {
    // insert into ExifBlob
    std::string xmlString = make_exif_xml_string(exifEntries);
    preparedStatements.insert_into_ExifBlob -> setInt64(1, checksum_key);
    preparedStatements.insert_into_ExifBlob -> setString(2, xmlString.c_str());
    int updateCount = preparedStatements.insert_into_ExifBlob -> executeUpdate();
    // find a time from the exif data
    std::list<std::string> fields;
    fields.push_back(std::string("Exif.Image.DateTime"));
    fields.push_back(std::string("Exif.Photo.DateTimeOriginal"));
    fields.push_back(std::string("Exif.Photo.DateTimeDigitized"));
 
    std::string camera_time = "";

    BOOST_FOREACH(std::string field, fields) {
      std::cout << field << std::endl;
      if (exifEntries.count(field)) {
        std::string exif_datetime = exifEntries.find(field)->second.value;
	std::string mysql_datetime = exif_datetime_to_mysql_datetime(exif_datetime);
        std::cout << "exif_datetime " << exif_datetime << " mysql_datetime " <<
            mysql_datetime << std::endl;
	break;
      }
    }

    // If we didn't find a time in the exif use import_time.

    if (camera_time == "") {
      camera_time = import_time;
    }

    preparedStatements.insert_into_Time -> setInt64(1, checksum_key);
    preparedStatements.insert_into_Time -> setString(2, camera_time);
    preparedStatements.insert_into_Time -> setString(3, camera_time);
    updateCount = preparedStatements.insert_into_Time -> executeUpdate();
  }

std::string
exif_datetime_to_mysql_datetime(const std::string exif_datetime)
{
  std::string mysql_datetime = exif_datetime;
  mysql_datetime[4] = '-';
  mysql_datetime[7] = '-';
  return mysql_datetime;
}

  /** make a string containing an xml representation of the ExifEntries */
  std::string
  make_exif_xml_string(const std::map<std::string, ExifEntry> &exifEntries) {
    DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));

    // First make a DOM tree of the elements.
    DOMDocument* doc = impl->createDocument(
        0,                    // root element namespace URI.
        X("exif"),            // root element name
        0);                   // document type object (DTD).

    DOMElement* rootElem = doc->getDocumentElement();

    typedef std::map<std::string, ExifEntry> map_t;
    BOOST_FOREACH(const map_t::value_type &exifEntry, exifEntries) {
      std::ostringstream convert;
      convert << exifEntry.second.count;

      DOMElement *elem = doc->createElement(X("t"));
      elem->setAttribute(X("name"), X(exifEntry.first.c_str()));
      elem->setAttribute(X("type"), X(exifEntry.second.type_name.c_str()));
      elem->setAttribute(X("count"), X(convert.str().c_str()));
      elem->setAttribute(X("value"), X(exifEntry.second.value.c_str())); 

      rootElem->appendChild(elem);
    }

    // Then make an XML serializer
    DOMLSSerializer* serializer = ((DOMImplementationLS*)impl)->createLSSerializer();
    if (serializer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true)) {
        serializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true);
    }
    if (serializer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, false)){
        serializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, false);
    }

    // Then serialize the DOM tree into an xml string
    XMLFormatTarget *xMLFormatTarget = new MemBufFormatTarget();
    DOMLSOutput* theOutput = ((DOMImplementationLS*)impl)->createLSOutput();
    theOutput->setByteStream(xMLFormatTarget);
    serializer->write(rootElem, theOutput);
    char* theXMLString_Encoded = (char*) ((MemBufFormatTarget*)xMLFormatTarget)->getRawBuffer();
    std::string result = std::string(theXMLString_Encoded);
    doc->release();

    return result;
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
      process_exif(filename);
    } catch(...) {
      std::cout << "Caught exception in process_photo_file, file was: " << filename << std::endl;
    }
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
  
        photoDbEntries.back().exifEntries.insert(
            std::map<std::string, ExifEntry>::value_type(
            i->key(), ExifEntry(std::string(tn), i->count(), i->value().toString())));
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
            if (importWindow -> is_cancel_requested()) {
	      return 0;
            }
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
          if (importWindow -> is_cancel_requested()) {
            return 0;
          }
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