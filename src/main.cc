#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <gtk/gtk.h>

#include "BaseWindow.h"
#include "SinglePhotoPage.h"
#include "MultiPhotoPage.h"
#include "Preferences.h"
#include "PhotoFileCache.h"

namespace sql {
  class Driver;
  class Connection;
}

void open_initial_project(sql::Connection *connection, BaseWindow *base_window,
    Preferences *preferences, PhotoFileCache *photoFileCache);
std::string get_last_project_name();
sql::Connection *
open_database(std::string dbhost, std::string user, std::string password, std::string database);

using namespace std;

main(int argc, char **argv)
{
  Preferences preferences;
  PhotoFileCache photoFileCache;

  gtk_init(&argc, &argv);

  // Initialize the XML4C2 system.
  try {
    xercesc::XMLPlatformUtils::Initialize();
  } catch(const xercesc::XMLException& toCatch) {
    char *pMsg = xercesc::XMLString::transcode(toCatch.getMessage());
    std::cerr << "Error during Xerces-c Initialization.\n"
        << "  Exception message:"
        << pMsg;
    xercesc::XMLString::release(&pMsg);
    exit(1);
  }

  std::string dbhost = preferences.get_dbhost();
  std::string user = preferences.get_user();
  std::string password = preferences.get_password();
  std::string database = preferences.get_database();
  sql::Connection *connection = open_database(dbhost, user, password, database);

  BaseWindow *baseWindow = new BaseWindow(connection, &preferences, &photoFileCache);
  baseWindow->run();

  if (connection) {
    open_initial_project(connection, baseWindow, &preferences, &photoFileCache);
  }

  gtk_main();
}

void
open_initial_project(sql::Connection *connection, BaseWindow *base_window,
    Preferences *preferences, PhotoFileCache *photoFileCache) {
  std::string project_name = get_last_project_name();
  std::cout << "project name " << project_name << std::endl;
  if (0 == project_name.size()) {
    return;
  }
  std::list<std::string> photoFilenameList = Utils::get_project_photo_files(connection,
      project_name);
  SinglePhotoPage *photoSelectPage = new SinglePhotoPage(connection, photoFileCache);
  photoSelectPage->setup(photoFilenameList, project_name, preferences);
  base_window->add_page(photoSelectPage->get_tab_label(),
      photoSelectPage->get_notebook_page(), project_name);
  MultiPhotoPage *multiPhotoPage = new MultiPhotoPage(connection, photoFileCache);
  multiPhotoPage->setup(photoFilenameList, project_name, preferences);
  base_window->add_page(multiPhotoPage->get_tab_label(),
      multiPhotoPage->get_notebook_page(), project_name);
}

std::string get_last_project_name() {
  wordexp_t exp_result;
  wordexp("~/.PhotoSelect.last", &exp_result, 0);
  std::ifstream infile(exp_result.we_wordv[0], std::ios::in);
  wordfree(&exp_result);
  std::string result;
  if (infile.fail()) {
    result = "";
  } else {
    std::stringstream buffer;
    buffer << infile.rdbuf();
    result = buffer.str();
  }
  infile.close();
  // Strip a newline and anything that follows it;
  size_t f = result.find_first_of("\n\r");
  if (std::string::npos != f) {
    result.erase(f);
  }
  return result;
}

sql::Connection *
open_database(std::string dbhost, std::string user, std::string password, std::string database) {

  /* initiate url, user, password and database variables */
  sql::Driver *driver = Utils::get_driver_instance();
  if (0 == driver) {
    std::cerr <<  "get_driver_instance() failed.\n" << std::endl;
    exit(1);
  }

  std::string url=dbhost;
  sql::Connection *connection = Utils::get_connection(driver, url, user, password);
  if (NULL == connection) {
    std::cerr << "driver -> connect() failed\n" << std::endl;
  } else {
    Utils::set_schema(connection, database);
  }
  return connection;
}
