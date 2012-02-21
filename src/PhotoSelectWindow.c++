#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include "PhotoSelectWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>

GtkWidget *get_toplevel_widget (GtkWidget *widget);

class PhotoSelectWindow;
 extern "C" {
  PhotoSelectWindow* getPhotoSelectWindow(GtkWidget *widget) {
    PhotoSelectWindow *photoSelectWindow = 0;
    map<GtkWindow*, PhotoSelectWindow*>::iterator it =
        PhotoSelectWindow::windowMap.find(GTK_WINDOW(get_toplevel_widget(widget)));
//      PhotoSelectWindow::windowMap.find(GTK_WINDOW(gtk_widget_get_toplevel(widget))); [doesn't work]
    if (PhotoSelectWindow::windowMap.end() == it) {
      printf("Cannot find window in the windowMap\n");
    } else {
      photoSelectWindow = it -> second;
    }
    return photoSelectWindow;
  }

  G_MODULE_EXPORT void
  drawingarea1_expose_event_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    printf("drawingarea1_expose_event_cb\n");
    PhotoSelectWindow *photoSelectWindow = getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Keep_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Keep_clicked_cb\n");
  }

  G_MODULE_EXPORT void
  Drop_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Drop_clicked_cb\n");
  }

  G_MODULE_EXPORT void
  Next_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Next_clicked_cb\n");
    PhotoSelectWindow *photoSelectWindow = getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      photoSelectWindow -> next();
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Back_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Back_clicked_cb\n");
    PhotoSelectWindow *photoSelectWindow = getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      photoSelectWindow -> back();
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Rotate_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectWindow *photoSelectWindow = getPhotoSelectWindow(widget);
    if (0 != photoSelectWindow) {
      printf("Rotate_clicked_cb\n");
      photoSelectWindow -> rotation += 1;
      if (photoSelectWindow -> rotation >= 4) photoSelectWindow -> rotation = 0;
      photoSelectWindow -> redraw_image();
    }
  }

  G_MODULE_EXPORT void
  Gimp_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("Gimp_clicked_cb\n");
  }

  G_MODULE_EXPORT void
  Position_changed_cb(GtkWidget *widget, gpointer data) {
    printf("Position_changed_cb\n");
  }

  G_MODULE_EXPORT void
  Position_editing_done_cb(GtkWidget *widget, gpointer data) {
    printf("editing_done_cb\n");
  }

  G_MODULE_EXPORT void
  view_query_activate_cb(GtkMenuItem *menuItem, gpointer user_data) {
    PhotoSelectWindow *photoSelectWindow = getPhotoSelectWindow(GTK_WIDGET(GTK_MENU_ITEM(menuItem)));
    photoSelectWindow->query();
  }
}

GtkWidget *get_toplevel_widget (GtkWidget *widget)
{
  GtkWidget *parent;

  const int maxLevels = 10000; // Don't try forever
  int i;
  for (i=0; i<maxLevels; i++)
  {
    if (GTK_IS_MENU (widget))
      parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
    else
      parent = widget->parent;
    if (parent == NULL)
      break;
    widget = parent;
  }
  if (i == maxLevels) return 0;
  return (widget);
}

map<GtkWindow*, PhotoSelectWindow*> PhotoSelectWindow::windowMap;

