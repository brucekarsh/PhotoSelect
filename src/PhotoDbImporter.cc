#include "PhotoDbImporter.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/ripemd.h>
#include <exiv2/exiv2.hpp>
#include <fcntl.h>
#include <fts.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <time.h>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>

#include "Db.h"
#include "ImportWindow.h"
#include "XStr.h"

#define X(str) XStr(str).unicodeForm()

using namespace std;
XERCES_CPP_NAMESPACE_USE

class ImportWindow;

PhotoDbImporter::PhotoDbImporter() {
  // Get import_time, the current UTC time. We attribute this time to a photo if we can't 
  // get the time from the photo's exif.
  struct timeb now_timeb;
  ftime(&now_timeb); 
  time_t now_time_t = now_timeb.time;
  struct tm now_tm;
  gmtime_r(&now_time_t, &now_tm);
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
  cout << "import_time " << import_time << endl;
  cout << "import_timezone " << import_timezone << endl;
}

boost::regex PhotoDbImporter::re_for_jpg_suffix() {
  return boost::regex(".*\\.jpg$", boost::regex::icase);
}

void PhotoDbImporter::set_dirs_to_process(queue<string> dirs_to_process) {
  this -> dirs_to_process = dirs_to_process;
}

void PhotoDbImporter::go_through_files(ImportWindow *importWindow)
{
  int nfiles = count_files_to_process(importWindow);
  if (importWindow -> is_cancel_requested()) {
    return;
  }
  process_files(importWindow, nfiles);
}

bool PhotoDbImporter::insert_into_database_transaction() {
  boost::function<void (void)> f = boost::bind(&PhotoDbImporter::insert_into_database_op, this);
  return db.transaction(f);
}

void PhotoDbImporter::insert_into_database_op() {
  db.enter_operation();
  BOOST_FOREACH(PhotoDbEntry photoDbEntry, photoDbEntries) {
    int64_t checksum_key;
    db.insert_into_Checksum_op(photoDbEntry.checksum, checksum_key);
    int64_t photoFile_key;
    db.insert_into_PhotoFile_op(photoDbEntry.filePath, checksum_key, photoFile_key);

    insert_into_exif_tables(photoDbEntry.exifEntries, checksum_key);
    photoDbEntry.exifEntries.clear();
  }
  photoDbEntries.clear();
}

void PhotoDbImporter::insert_into_exif_tables(const map<string, ExifEntry> &exifEntries,
    int64_t checksum_key) {
  // insert into ExifBlob
  string xmlString = make_exif_xml_string(exifEntries);
  db.insert_into_exifblob_op(checksum_key, xmlString);
  // find a time from the exif data
  list<string> fields;
  fields.push_back(string("Exif.Image.DateTime"));
  fields.push_back(string("Exif.Photo.DateTimeOriginal"));
  fields.push_back(string("Exif.Photo.DateTimeDigitized"));
  string camera_time = "";
  BOOST_FOREACH(string field, fields) {
    cout << field << endl;
    if (exifEntries.count(field)) {
      string exif_datetime = exifEntries.find(field)->second.value;
	string mysql_datetime = exif_datetime_to_mysql_datetime(exif_datetime);
      cout << "exif_datetime " << exif_datetime << " mysql_datetime " <<
          mysql_datetime << endl;
      camera_time = mysql_datetime;
	break;
    }
  }
  // If we didn't find a time in the exif use import_time.
  // TODO This doesn't set the right times into the database
  if (camera_time == "") {
    camera_time = import_time;
  }

  db.insert_into_time_op(checksum_key, camera_time, camera_time);
}

string PhotoDbImporter::exif_datetime_to_mysql_datetime(const string exif_datetime) {
  string mysql_datetime = exif_datetime;
  mysql_datetime[4] = '-';
  mysql_datetime[7] = '-';
  return mysql_datetime;
}

/** make a string containing an xml representation of the ExifEntries */
string PhotoDbImporter::make_exif_xml_string(const map<string, ExifEntry> &exifEntries) {
  DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));

  // First make a DOM tree of the elements.
  DOMDocument* doc = impl->createDocument(
      0,                    // root element namespace URI.
      X("exif"),            // root element name
      0);                   // document type object (DTD).

  DOMElement* rootElem = doc->getDocumentElement();

  typedef map<string, ExifEntry> map_t;
  BOOST_FOREACH(const map_t::value_type &exifEntry, exifEntries) {
    ostringstream convert;
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
  string result = string(theXMLString_Encoded);
  doc->release();

  return result;
}

void PhotoDbImporter::process_photo_file(const string &filename) {
  cout << filename << endl;
  string hashHexOutput;

  compute_picture_file_hash(filename, hashHexOutput);
  photoDbEntries.push_back(PhotoDbEntry(filename, hashHexOutput));
  try {
    process_exif(filename);
  } catch(...) {
    cout << "Caught exception in process_photo_file, file was: " << filename << endl;
  }
}

void PhotoDbImporter::process_exif(const string &filename) {
  Exiv2::Image::AutoPtr exiv2Image;
  exiv2Image = Exiv2::ImageFactory::open(filename);
  if (0 != exiv2Image.get()) {
    exiv2Image->readMetadata();
    Exiv2::ExifData &exifData = exiv2Image->exifData();

    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != exifData.end(); ++i) {
      const char* tn = i->typeName();
      if (i->value().size() > 40) continue;

      photoDbEntries.back().exifEntries.insert(
          map<string, ExifEntry>::value_type(
          i->key(), ExifEntry(string(tn), i->count(), i->value().toString())));
    }
  } else {
    // TODO what to do if exiv2Image.get fails?
  }
}

void PhotoDbImporter::compute_picture_file_hash(const string& filename, string &hashHexOutput) {
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

bool PhotoDbImporter::is_photo_file(char *filename) {
  return regex_match(filename, re_for_jpg_suffix());
}

int PhotoDbImporter::PhotoDbImporter::count_files_to_process(ImportWindow* importWindow) {
  const int process_rate = 50;
  int process_count = 0;
  int total_process_count = 0;

  // TODO go through all dirs in dirs_to_process
  queue<string> dirs_to_process_copy = dirs_to_process;
  string dir = dirs_to_process_copy.front();
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
          importWindow -> runUI(100);
          if (importWindow -> is_cancel_requested()) {
	      return 0;
          }
        }
      }
    }
  }
  return total_process_count;
}

int PhotoDbImporter::process_files(ImportWindow* importWindow, int file_count) {
  const int process_rate = 2;
  int process_count = 0;
  int total_process_count = 0;

  // TODO go through all dirs in dirs_to_process
  queue<string> dirs_to_process_copy = dirs_to_process;
  string dir = dirs_to_process_copy.front();
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
        process_photo_file(string(ftsentp->fts_path));
        process_count++;
        total_process_count++;
        if (importWindow -> is_cancel_requested()) {
          return 0;
        }
        if (process_count >= process_rate) {
          insert_into_database_transaction();
          process_count = 0;
          float fraction = (gdouble)total_process_count/(gdouble)file_count;
          importWindow -> setProgressBar(fraction);
          importWindow -> runUI(100);
        }
      }
    }
  }

  if (process_count) {
    insert_into_database_transaction();
    process_count = 0;
  }
  return 0;
}
