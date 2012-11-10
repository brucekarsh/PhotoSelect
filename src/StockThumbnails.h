#ifndef STOCK_THUMBNAILS_H__
#define STOCK_THUMBNAILS_H__
#include <gtk/gtk.h>
class StockThumbnails {
  public:
    StockThumbnails();
    ~StockThumbnails();
    GdkPixbuf *get_loading_thumbnail();
    GdkPixbuf *get_bad_jpeg_thumbnail();
    void make_loading_thumbnail();
    void make_bad_jpeg_thumbnail();
  private:
    static void pixbuf_destroy_cb(guchar *pixels, gpointer data);
    static const int ICON_WIDTH = 380;
    static const int ICON_HEIGHT = 285;
    static const int ICON_STRIDE = ICON_WIDTH * 3;
    // TODO get ICON_WIDTH, ICON_HEIGHT from the same place as MultiPhotoPage and Worker
    GdkPixbuf *loading_thumbnail;
    GdkPixbuf *bad_jpeg_thumbnail;
};
#endif  // STOCK_THUMBNAILS_H__
