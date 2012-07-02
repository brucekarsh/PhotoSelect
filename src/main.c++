#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include "PhotoSelectWindow.h"
#include <xercesc/util/PlatformUtils.hpp>

sql::Connection *
open_database(std::string dbhost, std::string user, std::string password, std::string database);

using namespace std;

main(int argc, char **argv)
{
  Preferences preferences;

  gtk_init(&argc, &argv);

  // Initialize the XML4C2 system.
  try {
    XMLPlatformUtils::Initialize();
  } catch(const XMLException& toCatch) {
    char *pMsg = XMLString::transcode(toCatch.getMessage());
    std::cerr << "Error during Xerces-c Initialization.\n"
        << "  Exception message:"
        << pMsg;
    XMLString::release(&pMsg);
    exit(1);
  }

  list<string> photoFilenameList1;
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/AW100/DSCN0651.JPG");
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/AW100/DSCN0551.JPG");
  photoFilenameList1.push_back("/home/bruce/Tanzania2012/D7000-6/DSC_8557.JPG");
  std::string dbhost = preferences.get_dbhost();
  std::string user = preferences.get_user();
  std::string password = preferences.get_password();
  std::string database = preferences.get_database();
  sql::Connection *connection = open_database(dbhost, user, password, database);
  PhotoSelectWindow photoSelectWindow1(connection);
  photoSelectWindow1.setup(photoFilenameList1, &preferences);

  //list<string> photoFilenameList2;
  //photoFilenameList2.push_back("/home/bruce/Tanzania2012/AW100/DSCN0351.JPG");
  //photoFilenameList2.push_back("/home/bruce/Tanzania2012/AW100/DSCN0451.JPG");
  //PhotoSelectWindow photoSelectWindow2;
  //photoSelectWindow2.setup(photoFilenameList2);
  //GtkSettings *default_settings = gtk_settings_get_default();
  //g_object_set(default_settings, "gtk-button-images", TRUE, NULL);
  gtk_main();
}

sql::Connection *
open_database(std::string dbhost, std::string user, std::string password, std::string database) {

  /* initiate url, user, password and database variables */
  sql::Driver *driver = get_driver_instance();
  if (0 == driver) {
    fprintf(stderr, "get_driver_instance() failed.\n");
    exit(1);
  }

  std::string url;
  sql::Connection *connection = driver -> connect(url, user, password);
  if (NULL == connection) {
    printf("driver -> connect() failed\n");
    exit(1);
    // TODO handle db open failure.
  }

  connection -> setAutoCommit(0);
  connection -> setSchema(database);

  return connection;
}
