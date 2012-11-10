#include "Worker.h"

#include <iostream>
#include "WorkList.h"
#include "ConversionEngine.h"
#include "StockThumbnails.h"
#include "ConvertedPhotoFile.h"
#include "MultiPhotoPage.h"

using namespace std;

Worker::Worker() {
  my_worker_num = static_worker_num;
  static_worker_num++;
}

void Worker::operator()() {
  do_work();
}

void Worker::do_work() {
  while(1) {
    WorkItem work_item;
    bool b = false;
    try {
      bool blocking = true;
      bool b = work_list.get_next_work_item(work_item, blocking);
      BOOST_ASSERT(b);
    } catch(int signum) {
	  // The workList has shut down. We just exit the thread.
      break;
    }
    GdkPixbuf *pixbuf = build_pixbuf_from_work_item(work_item);
    gboolean is_delivery_done = false;
    while (!is_delivery_done) {
      int delivery_result = deliver_pixbuf_to_multiphotopage(pixbuf, work_item);
      if (delivery_result == DELIVERY_RETRY_LATER) {
        // Recipient queue is full. Wait a while and try again
        usleep(1000000);
      } else if (delivery_result == DELIVERY_REJECTED) {
        // Recipient no longer exists
        g_object_unref(pixbuf);
        is_delivery_done = true;
      } else {
        // successfully delivered
        is_delivery_done = true;
      }
    }
  }
}

GdkPixbuf *Worker::build_pixbuf_from_work_item(const WorkItem &work_item) {
  int ICON_WIDTH = MultiPhotoPage::ICON_WIDTH;
  int ICON_HEIGHT = MultiPhotoPage::ICON_HEIGHT;
  int ICON_STRIDE = MultiPhotoPage::ICON_STRIDE;
  string photofile_name = (work_item.multiPhotoPage)->get_photofile_name(work_item.index);

  int rotation = work_item.rotation;
  ConvertedPhotoFile *convertedPhotoFile = ConversionEngine::getConvertedPhotoFile(
      photofile_name, ICON_WIDTH, ICON_WIDTH, rotation);
  int width = convertedPhotoFile->width;
  int height = convertedPhotoFile->height;
  if (width == 0 || height == 0) {
    delete convertedPhotoFile;
    return stock_thumbnails->get_bad_jpeg_thumbnail();
  }
  double M;
  (work_item.multiPhotoPage)->calculate_scaling(M, width, height, ICON_WIDTH,
      ICON_HEIGHT);
  unsigned char *pixels = convertedPhotoFile->scale_and_pan_and_rotate(
      ICON_WIDTH, ICON_HEIGHT, M, 0.0, 0.0, rotation);
  delete convertedPhotoFile;
  // Convert pixels to the format favored by GtkIconView
  unsigned char *newpixels = (unsigned char *)malloc(ICON_WIDTH * ICON_HEIGHT * 3);
  unsigned char *p = newpixels;
  for (int i = 0; i < ICON_WIDTH * ICON_HEIGHT; i++) {
    *p++ = pixels[4*i+2];
    *p++ = pixels[4*i+1];
    *p++ = pixels[4*i+0];
  }
  free(pixels);
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(newpixels, GDK_COLORSPACE_RGB,
      FALSE, 8, ICON_WIDTH, ICON_HEIGHT, ICON_STRIDE, MultiPhotoPage::pixbuf_destroy_cb,
      NULL);
  return pixbuf;
}

int Worker::deliver_pixbuf_to_multiphotopage(GdkPixbuf *pixbuf, const WorkItem& work_item) {
  int ret = DELIVERY_OK;
  bool b = ticket_registry.reference_ticket(work_item.ticket_number);
  if (!b) {
    // The class that we made this pixbuf for is gone, so we balk.
    return DELIVERY_REJECTED;
  }
  bool is_delivered = (work_item.multiPhotoPage)->set_thumbnail(work_item.index,
      pixbuf, work_item.rotation, work_item.priority);
  if (is_delivered) {
    ret = DELIVERY_OK;
  } else {
    ret = DELIVERY_RETRY_LATER;
  }
  ticket_registry.unreference_ticket(work_item.ticket_number);
  return ret;
}
