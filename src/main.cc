#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <gtk/gtk.h>
#include <boost/foreach.hpp>
#include <wordexp.h>
#include <iostream>
#include <fstream>
#include <sstream>


#include "Db.h"
#include "BaseWindow.h"
#include "SinglePhotoPage.h"
#include "MultiPhotoPage.h"
#include "Preferences.h"
#include "PhotoFileCache.h"
#include "WorkList.h"
#include "Worker.h"
#include "StockThumbnails.h"
#include "TicketRegistry.h"

namespace sql {
  class Driver;
  class Connection;
}

void open_initial_project(BaseWindow *base_window,
    Preferences *preferences, PhotoFileCache *photoFileCache);
std::string get_last_project_name();
sql::Connection *
open_database(std::string dbhost, std::string user, std::string password, std::string database);
void *thread_proc(void *arg);

using namespace std;

WorkList work_list;
StockThumbnails *stock_thumbnails;
TicketRegistry ticket_registry;
Db db;

main(int argc, char **argv)
{
  Preferences preferences;
  PhotoFileCache photoFileCache;

  gdk_threads_init();
  gdk_threads_enter();
  gtk_init(&argc, &argv);
  stock_thumbnails = new StockThumbnails();

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

  Db::dbhost = preferences.get_dbhost();
  Db::user = preferences.get_user();
  Db::password = preferences.get_password();
  Db::database = preferences.get_database();

  BaseWindow *baseWindow = new BaseWindow(&preferences, &photoFileCache);
  baseWindow->run();
  open_initial_project(baseWindow, &preferences, &photoFileCache);

  list<boost::thread *> worker_list;
  const int NWORKERS = 2;
  for (int i = 0; i < NWORKERS; i++) {
    boost::thread * thread = new boost::thread(*new Worker());
    worker_list.push_back(thread);
  }

  gtk_main();
  gdk_threads_leave ();
  work_list.shutdown_work_list();
  BOOST_FOREACH(boost::thread *worker, worker_list) {
    worker->join();
  }
  delete stock_thumbnails;
}

void
open_initial_project(BaseWindow *base_window,
    Preferences *preferences, PhotoFileCache *photoFileCache) {
  std::string project_name = get_last_project_name();
  std::cout << "project name " << project_name << std::endl;
  if (0 == project_name.size()) {
    return;
  }
  std::vector<std::string> photoFilenameVector;
  std::vector<std::string> adjusted_date_time_vector;
  bool b = db.get_project_photo_files_transaction(project_name, photoFilenameVector,
      adjusted_date_time_vector);
  if (!b) {
    // TODO handle get_project_photo_files_transaction failure
  }
  MultiPhotoPage *multiPhotoPage = new MultiPhotoPage(photoFileCache);
  multiPhotoPage->setup(photoFilenameVector, adjusted_date_time_vector, project_name, preferences);
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

int Worker::static_worker_num = 0;
std::string Db::dbhost;
std::string Db::user;
std::string Db::password;
std::string Db::database;
