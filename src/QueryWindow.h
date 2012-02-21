#ifndef QUERYWINDOW_H__
#define QUERYWINDOW_H__
#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>

using namespace std;

class PhotoSelectWindow;

class QueryWindow {
  public:
  PhotoSelectWindow* photoSelectWindow;
  GtkWidget *window;


  QueryWindow(PhotoSelectWindow* photoSelectWindow) {
    this->photoSelectWindow = photoSelectWindow;
  }

  void run() {
    printf("QueryWindow::run()\n");
    
    /* Load UI from file. If error occurs, report it and quit application. */
    GError *error = NULL;
    GtkBuilder* builder = gtk_builder_new();
    if( ! gtk_builder_add_from_file( builder, "/home/bruce/GladeTestProject1.glade", &error ) ) {
        g_warning( "%s", error->message );
        g_free( error );
    }
    window = GTK_WIDGET( gtk_builder_get_object( builder, "QueryWindow" ));
    gtk_builder_connect_signals(builder, NULL);
    g_object_unref( G_OBJECT( builder ) );
    gtk_widget_show(window);
  }

};
#endif  // QUERYWINDOW_H__
