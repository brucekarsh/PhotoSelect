#ifndef STOCK_THUMBNAILS_H__
#define STOCK_THUMBNAILS_H__

#include <gtk/gtk.h>
#include <stdlib.h>
#include <iostream>

class StockThumbnails {
  public:
    StockThumbnails() {
      make_loading_thumbnail();
      make_bad_jpeg_thumbnail();
    }

    ~StockThumbnails() {
      std::cout << "~StockThumbnails" << std::endl;
      g_object_unref(G_OBJECT(loading_thumbnail));
      g_object_unref(G_OBJECT(bad_jpeg_thumbnail));
    }

    GdkPixbuf *get_loading_thumbnail() { return loading_thumbnail; }

    GdkPixbuf *get_bad_jpeg_thumbnail() { return bad_jpeg_thumbnail; }

    void make_loading_thumbnail() {
      unsigned char *newpixels = (unsigned char *)malloc(ICON_WIDTH * ICON_HEIGHT * 3);
      gboolean has_alpha = false;
      int bits_per_sample = 8;
      for (int i = 0; i < ICON_HEIGHT; i++) {
        for (int j = 0; j < ICON_WIDTH; j++) {
	   int p = i * ICON_STRIDE + j * 3;
	   newpixels[p] = 64;
	   newpixels[p+1] = 64;
	   newpixels[p+2] = 64;
        }
      }
      loading_thumbnail = gdk_pixbuf_new_from_data(newpixels, GDK_COLORSPACE_RGB,
          has_alpha, bits_per_sample, ICON_WIDTH, ICON_HEIGHT, ICON_STRIDE,
          StockThumbnails::pixbuf_destroy_cb, NULL);
    }

    void make_bad_jpeg_thumbnail() {
      unsigned char *newpixels = (unsigned char *)malloc(ICON_WIDTH * ICON_HEIGHT * 3);
      gboolean has_alpha = false;
      int bits_per_sample = 8;
      for (int i = 0; i < ICON_HEIGHT; i++) {
        for (int j = 0; j < ICON_WIDTH; j++) {
	   int p = i * ICON_STRIDE + j * 3;
	   newpixels[p] = 160;
	   newpixels[p+1] = 0;
	   newpixels[p+2] = 0;
        }
      }
      bad_jpeg_thumbnail = gdk_pixbuf_new_from_data(newpixels, GDK_COLORSPACE_RGB,
          has_alpha, bits_per_sample, ICON_WIDTH, ICON_HEIGHT, ICON_STRIDE,
          StockThumbnails::pixbuf_destroy_cb, NULL);
    }
    

  private:
    static void pixbuf_destroy_cb(guchar *pixels, gpointer data) {
      extern StockThumbnails *stock_thumbnails;
      if (pixels == gdk_pixbuf_get_pixels(stock_thumbnails->bad_jpeg_thumbnail)) {
        std::cout << "pixbuf_destroy_cb bad_jpeg_thumbnail" << std::endl;
      } else if (pixels == gdk_pixbuf_get_pixels(stock_thumbnails->loading_thumbnail)) {
        std::cout << "pixbuf_destroy_cb loading_thumbnail" << std::endl;
      } else {
        std::cout << "pixbuf_destroy_cb unknown-pixbuf" << std::endl;
      }
      free(pixels);
    }
    static const int ICON_WIDTH = 400;
    static const int ICON_HEIGHT = 300;
    static const int ICON_STRIDE = 400 * 3;
    GdkPixbuf *loading_thumbnail;
    GdkPixbuf *bad_jpeg_thumbnail;
};

#endif  // STOCK_THUMBNAILS_H__
