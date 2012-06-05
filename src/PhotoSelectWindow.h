#ifndef PHOTOSELECTWINOW_H__
#define PHOTOSELECTWINOW_H__

#include "WindowRegistry.h"
#include "Preferences.h"
#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include "QueryWindow.h"
#include "PreferencesWindow.h"
#include "ImportWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>

class PhotoSelectWindow {
  public:
    Preferences *thePreferences;
    int rotation;
    ConversionEngine conversionEngine;
    GtkWidget *window;
    GtkWidget *drawingarea1;
    GtkWidget *ofN;
    std::list<std::string> photoFilenameList;

  PhotoSelectWindow() : rotation(0), window(0), drawingarea1(0), thePreferences((Preferences*)0) {
  }

  void setup(std::list<std::string> photoFilenameList_, Preferences *thePreferences) {
    this -> thePreferences = thePreferences;
    photoFilenameList = photoFilenameList_;
    // Set up a conversion engine.
    conversionEngine.setPhotoFileList(&photoFilenameList);

    /* Load UI from file. If error occurs, report it and quit application. */
    GError *error = NULL;
    GtkBuilder* builder = gtk_builder_new();
    if( ! gtk_builder_add_from_file( builder,
        "/home/bruce/PROJECTS/NEWPHOTOSELECT/src/GladeTestProject1.glade", &error ) ) {
      g_warning( "%s", error->message );
      g_free( error );
    }

    // Get the window, save it in the WindowRegistry so we can find the object (the PhotoSelect Window)
    // when we see the window in a callback.

    window = GTK_WIDGET( gtk_builder_get_object( builder, "PhotoSelectWindow" ));
    WindowRegistry::setPhotoSelectWindow(window, this);

    drawingarea1 = GTK_WIDGET( gtk_builder_get_object(builder, "drawingarea1"));
    ofN = GTK_WIDGET( gtk_builder_get_object(builder, "OfN"));
    char ofstring[20];
    int numberOfPhotos = photoFilenameList.size();
    snprintf(ofstring, 20, "of %d      ", numberOfPhotos);
    ofstring[19]=0;
    gtk_label_set_text(GTK_LABEL(ofN), ofstring);
    gtk_builder_connect_signals( builder, NULL );
    g_object_unref( G_OBJECT( builder ) );
    gtk_widget_show( window );
  } 

  void quit() {
    printf("PhotoSelectWindow::quit() entered\n");
  }

  void redraw_image() {
    double rotated_aspectratio;
    int scaled_image_width, scaled_image_height;
    int rotated_image_width, rotated_image_height;
    ScaledImage *scaled_image;

    ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile(); 
    printf("%d %d\n", convertedPhotoFile->width, convertedPhotoFile->height);

    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(drawingarea1));
    cairo_set_source_rgb(cr, 0.2,  0.5, 0.2);
    cairo_paint(cr);

    // Figure out the width and height of the rotated (but not scaled) image.

    rotated_image_width=0;   // to get rid of a compiler warning
    rotated_image_height=0;  // to get rid of a compiler warning
    switch(rotation) {
        case 0:
        case 2:
            rotated_image_width = convertedPhotoFile->width;
            rotated_image_height = convertedPhotoFile->height;
            break;
        case 1:
        case 3:
            rotated_image_width = convertedPhotoFile->height;
            rotated_image_height = convertedPhotoFile->width;
            break;
    }


    // Here we figure out the size of the scaled and rotated image
    //
    // Try to fit the scaled and rotated image horizontally along the
    // entire widget->allocation.width
    // If it won't fit, then scale it vertically along the entire
    // widget->allocation.height
    // XXX if this fp-safe?

    cairo_surface_t * dest_surface = cairo_get_target(cr);
    printf("dest_surface = 0x%lx\n", (long)dest_surface);
    if (cairo_surface_status(dest_surface) != CAIRO_STATUS_SUCCESS) {
      printf("Nil dest_surface\n");
    } else {
      printf("Non-Nil dest_surface\n");
    }
    printf("cairo_surface_get_type %d\n", cairo_surface_get_type(dest_surface));
    gint surface_width = gtk_widget_get_allocated_width(drawingarea1);
    gint surface_height = gtk_widget_get_allocated_height(drawingarea1);
    printf("surface width %d height %d\n", surface_width, surface_height);

    rotated_aspectratio = ((double)rotated_image_width)/((double)rotated_image_height);
    scaled_image_width = surface_width;
    scaled_image_height = (int)(surface_width / rotated_aspectratio);
    if(scaled_image_height > surface_height)  {
        scaled_image_height = surface_height;
        scaled_image_width = (int)(scaled_image_height * rotated_aspectratio);
    }

    // Allocate a buffer for the scaled and rotated image
    // Scale the image.
    // Get the scaled image pixels.  They are not yet rotated.

    switch(rotation) {
        case 0:
        case 2:
           printf("scaling, %d x %d\n", scaled_image_width, scaled_image_height);
           scaled_image =
               convertedPhotoFile->scale(scaled_image_width,scaled_image_height);
           printf("scaled_image %d x %d\n", scaled_image->getWidth(), scaled_image->getHeight());
            break;
        case 1:
        case 3:
        default:
           scaled_image =
               convertedPhotoFile->scale(scaled_image_height,scaled_image_width);
            break;
    }

    switch(rotation) {
        case 0:
            // No need to rotate.
            break;
        case 1:
            scaled_image->rotate90();
            break;
        case 2:
            scaled_image->rotate180();
            break;
        case 3:
            scaled_image->rotate270();
            break;
    }

    printf("drawing %dx%d 0x%lx\n",scaled_image_width, scaled_image_height, (long)scaled_image->getPixels());
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, scaled_image_width);
    cairo_surface_t *source_surface = cairo_image_surface_create_for_data(scaled_image->getPixels(),
        CAIRO_FORMAT_RGB24, scaled_image_width, scaled_image_height, stride);
    cairo_set_source_surface(cr, source_surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(source_surface);

    delete scaled_image;
  }

  void next() {
    conversionEngine.next();   
  }

  void back() {
    conversionEngine.back();   
  }

  void query() {
    printf("PhotoSelectWindow::query\n");
    QueryWindow queryWindow(this);
    queryWindow.run();
  }

  void preferences() {
    printf("PhotoSelectWindow::preferences\n");
    PreferencesWindow *preferencesWindow = new PreferencesWindow(thePreferences);
    preferencesWindow -> run();
  }

  void import() {
    printf("PhotoSelectWindow::import\n");
    ImportWindow *importWindow = new ImportWindow(thePreferences);
    importWindow -> run();
  }
};
#endif  // PHOTOSELECTWINOW_H__
