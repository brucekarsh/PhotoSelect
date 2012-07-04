#ifndef PHOTOSELECTWINOW_H__
#define PHOTOSELECTWINOW_H__

#include "WindowRegistry.h"
#include "Preferences.h"
#include <list>
#include <stdio.h>
#include "ConversionEngine.h"
#include "PreferencesWindow.h"
#include "ImportWindow.h"
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include <boost/lexical_cast.hpp>

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
    g_signal_connect(drawing_area, "button-press-event",
        G_CALLBACK(drawing_area_button_press_cb), NULL);
    g_signal_connect(drawing_area, "button-release-event",
        G_CALLBACK(drawing_area_button_release_cb), NULL);
    g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(drawing_area_scroll_cb), NULL);
    g_signal_connect(drawing_area, "motion-notify-event",
        G_CALLBACK(drawing_area_motion_notify_cb), NULL);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
        | GDK_SCROLL_MASK | GDK_BUTTON_MOTION_MASK);

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
    g_signal_connect(position_entry, "activate", G_CALLBACK(position_entry_activate_cb), 0);
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

    // Set up the position_entry
    set_position_entry();
  } 

  void
  position_entry_activate() {
    std::string valstr = gtk_entry_get_text(GTK_ENTRY(position_entry));
    int val = atoi(valstr.c_str());
    if (val < 1) {
      val = 1;
    }
    int siz = photoFilenameList.size();
    if (val > siz) {
      val = siz;
    }
    conversionEngine.go_to(val-1);   
    set_position_entry();
  }

  void
  drawing_area_button_press() {
    std::cout << "photoSelectPage->drawing_area_button_press entered" << std::endl; 
    // TODO WRITEME
  }

  void
  drawing_area_button_release() {
    std::cout << "photoSelectPage->drawing_area_button_release entered" << std::endl; 
    // TODO WRITEME
  }

  void
  drawing_area_scroll() {
    std::cout << "photoSelectPage->drawing_area_scroll entered" << std::endl; 
    // TODO WRITEME
  }

  void
  drawing_area_motion_notify() {
    std::cout << "photoSelectPage->drawing_area_motion_notify entered" << std::endl; 
    // TODO WRITEME
  }

  /** Sets to position_entry widget to reflect the positioon of the ConversionEngine.
      The ConversionEngine counts 0..N-1 and the position_entry counts 1..N. */
  void
  set_position_entry() {
    int val = conversionEngine.get_position();
    std::string valstring =  boost::lexical_cast<std::string>(val + 1);
    gtk_entry_set_text(GTK_ENTRY(position_entry), valstring.c_str());
  }

  void redraw_image() {
    double rotated_aspectratio;
    int scaled_image_width, scaled_image_height;
    int rotated_image_width, rotated_image_height;
    ScaledImage *scaled_image;

    ConvertedPhotoFile *convertedPhotoFile = conversionEngine.getConvertedPhotoFile(); 
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(drawing_area));
    cairo_set_source_rgb(cr, 0.2,  0.5, 0.2);
    cairo_paint(cr);
    if (NULL == convertedPhotoFile) {
      return;
    }

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
    // TODO is this fp-safe?

    cairo_surface_t * dest_surface = cairo_get_target(cr);
    if (cairo_surface_status(dest_surface) != CAIRO_STATUS_SUCCESS) {
      std::cout << "Nil dest_surface" << std::endl;
      // TODO handle this case
    }
    gint surface_width = gtk_widget_get_allocated_width(drawing_area);
    gint surface_height = gtk_widget_get_allocated_height(drawing_area);

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
           scaled_image =
               convertedPhotoFile->scale(scaled_image_width,scaled_image_height);
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

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, scaled_image_width);
    cairo_surface_t *source_surface = cairo_image_surface_create_for_data(scaled_image->getPixels(),
        CAIRO_FORMAT_RGB24, scaled_image_width, scaled_image_height, stride);
    cairo_set_source_surface(cr, source_surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(source_surface);

    delete scaled_image;
  }

  void keep() {
    // TODO WRITEME or DELETEME
  }

  void drop() {
    // TODO WRITEME or DELETEME
  }

  void next() {
    rotation = 0;
    conversionEngine.next();   
    set_position_entry();
  }

  void back() {
    rotation = 0;
    conversionEngine.back();   
    set_position_entry();
  }

  void rotate() {
    rotation += 1;
    if (rotation == 4) {
      rotation = 0;
    }
  }

  void gimp() {
    // TODO WRITEME or DELETEME
  }

  static void keep_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> keep();
      photoSelectPage -> redraw_image();
    }
  }

  static void drop_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> drop();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  next_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> next();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  back_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> back();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  rotate_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> rotate();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  gimp_button_clicked_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> gimp();
      photoSelectPage -> redraw_image();
    }
  }

  static void
  drawing_area_draw_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage -> redraw_image();
    }
  }

  static void
  position_entry_activate_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->position_entry_activate();
      photoSelectPage -> redraw_image();
    }
  }
  static void
  drawing_area_button_press_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_button_press();
    }
  }
  static void
  drawing_area_button_release_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_button_release();
      photoSelectPage -> redraw_image();
    }
  }
  static void
  drawing_area_scroll_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_scroll();
      photoSelectPage -> redraw_image();
    }
  }
  static void
  drawing_area_motion_notify_cb(GtkWidget *widget, gpointer data) {
    PhotoSelectPage *photoSelectPage = WindowRegistry::getPhotoSelectPage(widget);
    if (0 != photoSelectPage) {
      photoSelectPage->drawing_area_motion_notify();
      photoSelectPage -> redraw_image();
    }
  }
};
#endif  // PHOTOSELECTWINOW_H__
