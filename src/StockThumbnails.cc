#include "StockThumbnails.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

StockThumbnails::StockThumbnails() {
  make_loading_thumbnail();
  make_bad_jpeg_thumbnail();
}

StockThumbnails::~StockThumbnails() {
  cout << "~StockThumbnails" << endl;
  g_object_unref(G_OBJECT(loading_thumbnail));
  g_object_unref(G_OBJECT(bad_jpeg_thumbnail));
}

GdkPixbuf *StockThumbnails::get_loading_thumbnail() { return loading_thumbnail; }

GdkPixbuf *StockThumbnails::get_bad_jpeg_thumbnail() { return bad_jpeg_thumbnail; }

void StockThumbnails::make_loading_thumbnail() {
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

void StockThumbnails::make_bad_jpeg_thumbnail() {
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

/* static */ void StockThumbnails::pixbuf_destroy_cb(guchar *pixels, gpointer data) {
  extern StockThumbnails *stock_thumbnails;
  if (pixels == gdk_pixbuf_get_pixels(stock_thumbnails->bad_jpeg_thumbnail)) {
    cout << "pixbuf_destroy_cb bad_jpeg_thumbnail" << endl;
  } else if (pixels == gdk_pixbuf_get_pixels(stock_thumbnails->loading_thumbnail)) {
    cout << "pixbuf_destroy_cb loading_thumbnail" << endl;
  } else {
    cout << "pixbuf_destroy_cb unknown-pixbuf" << endl;
  }
  free(pixels);
}
