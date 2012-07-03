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

class PhotoSelectPage {
  public:
    Preferences *thePreferences;
    int rotation;
    ConversionEngine conversionEngine;
    std::list<std::string> photoFilenameList;
    sql::Connection *connection;

    GtkWidget *page_vbox;
    GtkWidget *drawing_area;
    GtkWidget *button_hbox;
    GtkWidget *keep_button;
    GtkWidget *drop_button;
    GtkWidget *next_button;
    GtkWidget *back_button;
    GtkWidget *rotate_button;
    GtkWidget *gimp_button;
    GtkWidget *button_separator;
    GtkWidget *of_label;
    GtkWidget *position_entry;
    GtkWidget *tab_label;

  PhotoSelectPage(sql::Connection *connection_) :
      rotation(0), drawing_area(0), thePreferences((Preferences*)0), connection(connection_) {
  }

  GtkWidget *
  get_notebook_page() {
    return page_vbox;
  }

  GtkWidget *
  get_tab_label() {
    return tab_label;
  }

  void
  build_page() {
    // Make a label for the notebook tab
    tab_label = gtk_label_new("tab");
    // make a vbox to hold the page (page_vbox)
    page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(page_vbox);

    // add a drawing area (drawing_area) to page_vbox
    drawing_area = gtk_drawing_area_new();
    gtk_widget_show(drawing_area);
    gtk_box_pack_start(GTK_BOX(page_vbox), drawing_area, TRUE, TRUE, 0);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(drawing_area_draw_cb), 0);

    // make an hbox (button_hbox) to hold the buttons, etc and add it to page_vbox
    button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(button_hbox);
    gtk_box_pack_start(GTK_BOX(page_vbox), button_hbox, FALSE, FALSE, 0);

    // add Keep, Drop, Next, Back, Rotate and Gimp buttons to button_hbox
    keep_button = gtk_button_new_with_label("Keep");
    gtk_widget_show(keep_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), keep_button, FALSE, FALSE, 0);
    g_signal_connect(keep_button, "clicked", G_CALLBACK(keep_button_clicked_cb), 0);
    
    drop_button = gtk_button_new_with_label("Drop");
    gtk_widget_show(drop_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), drop_button, FALSE, FALSE, 0);
    g_signal_connect(drop_button, "clicked", G_CALLBACK(drop_button_clicked_cb), 0);
    
    next_button = gtk_button_new_with_label("Next");
    gtk_widget_show(next_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), next_button, FALSE, FALSE, 0);
    g_signal_connect(next_button, "clicked", G_CALLBACK(next_button_clicked_cb), 0);
    
    back_button = gtk_button_new_with_label("Back");
    gtk_widget_show(back_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), back_button, FALSE, FALSE, 0);
    g_signal_connect(back_button, "clicked", G_CALLBACK(back_button_clicked_cb), 0);
    
    rotate_button = gtk_button_new_with_label("Rotate");
    gtk_widget_show(rotate_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), rotate_button, FALSE, FALSE, 0);
    g_signal_connect(rotate_button, "clicked", G_CALLBACK(rotate_button_clicked_cb), 0);
    
    gimp_button = gtk_button_new_with_label("Gimp");
    gtk_widget_show(gimp_button);
    gtk_box_pack_start(GTK_BOX(button_hbox), gimp_button, FALSE, FALSE, 0);
    g_signal_connect(gimp_button, "clicked", G_CALLBACK(gimp_button_clicked_cb), 0);
    
    // add a separator (button_separator) to button_hbox
    button_separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_show(button_separator);
    gtk_box_pack_end(GTK_BOX(button_hbox), button_separator, FALSE, FALSE, 0);

    // add a label (of_label) to button_hbox
    of_label = gtk_label_new("of -1");
    gtk_widget_show(of_label);
    gtk_box_pack_end(GTK_BOX(button_hbox), of_label, FALSE, FALSE, 0);
    
    // add a text entry (position_entry) to button_hbox   
    position_entry = gtk_entry_new();
    gtk_widget_show(position_entry);
    gtk_box_pack_end(GTK_BOX(button_hbox), position_entry, FALSE, FALSE, 0);
  }

  void setup(std::list<std::string> photoFilenameList_, Preferences *thePreferences) {
    this -> thePreferences = thePreferences;
    photoFilenameList = photoFilenameList_;

    // Set up a conversion engine.
    conversionEngine.setPhotoFileList(&photoFilenameList);

    // Build the page
    build_page();

    // Add it to the registry so we can find this object when we get a callback
    WindowRegistry::setPhotoSelectPage(page_vbox, this);

    // Set up the of_label
    char ofstring[20];
    int numberOfPhotos = photoFilenameList.size();
    snprintf(ofstring, 20, "of %d      ", numberOfPhotos);
    ofstring[19]=0;
    gtk_label_set_text(GTK_LABEL(of_label), ofstring);
  } 

  void quit() {
    printf("PhotoSelectPage::quit() entered\n");
  }

  void redraw_image() {
    double rotated_aspectratio;
    int scaled_image_width, scaled_image_height;
    int rotated_image_width, rotated_image_height;
    ScaledImage *scaled_image;

    ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile(); 
    printf("%d %d\n", convertedPhotoFile->width, convertedPhotoFile->height);

    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(drawing_area));
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
    gint surface_width = gtk_widget_get_allocated_width(drawing_area);
    gint surface_height = gtk_widget_get_allocated_height(drawing_area);
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

  void keep() {
    printf("PhotoSelectPage::keep\n");
  }

  void drop() {
    printf("PhotoSelectPage::drop\n");
  }

  void next() {
    printf("PhotoSelectPage::next\n");
    rotation = 0;
    conversionEngine.next();   
  }

  void back() {
    printf("PhotoSelectPage::back\n");
    rotation = 0;
    conversionEngine.back();   
  }

  void rotate() {
    printf("PhotoSelectPage::rotate\n");
    rotation += 1;
    if (rotation == 4) {
      rotation = 0;
    }
  }

  void gimp() {
    printf("PhotoSelectPage::gimp\n");
  }

#ifdef NEVER
  void query() {
    printf("PhotoSelectPage::query\n");
    QueryWindow *queryWindow = new QueryWindow(connection);
    // TODO make sure that queryWindow eventually gets destroyed.
    queryWindow->run();
  }

  void preferences() {
    printf("PhotoSelectPage::preferences\n");
    PreferencesWindow *preferencesWindow = new PreferencesWindow(thePreferences);
    preferencesWindow -> run();
  }

  void import() {
    printf("PhotoSelectPage::import\n");
    ImportWindow *importWindow = new ImportWindow(thePreferences, connection);
    importWindow -> run();
  }
#endif // NEVER

  static void keep_button_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("keep_button_clicked_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> keep();
      photoSelectPage -> redraw_image();
    }
  }

  static void drop_button_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("drop_button_clicked_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> drop();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  next_button_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("next_button_clicked_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> next();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  back_button_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("back_button_clicked_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> back();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  rotate_button_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("rotate_button_clicked_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> rotate();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  gimp_button_clicked_cb(GtkWidget *widget, gpointer data) {
    printf("gimp_button_clicked_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> gimp();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  drawing_area_draw_cb(GtkWidget *widget, gpointer data) {
    printf("drawing_area_draw_cb\n");
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> redraw_image();
    }
  }

};
#endif  // PHOTOSELECTWINOW_H__
