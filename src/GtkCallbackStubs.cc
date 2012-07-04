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
    printf("import_response_activate_cb\n");
    printf("response_id = %d\n", response_id);
    ImportWindow *importWindow = WindowRegistry::getImportWindow(widget);
    importWindow->import_response_cb(response_id);
  }
// TODO get rid of excessive redraws.
}
