#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include "PhotoSelectPage.h"
#include "ImportWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>

class PhotoSelectPage;
 extern "C" {

  G_MODULE_EXPORT void
  import_response_cb(GtkWidget *widget, gint response_id, gpointer user_data) {
    std::cout << "import_response_activate_cb" << std::endl;
    std::cout << "response_id = " << response_id << std::endl;
    ImportWindow *importWindow = WindowRegistry::getImportWindow(widget);
    importWindow->import_response_cb(response_id);
  }
// TODO get rid of excessive redraws.
}
