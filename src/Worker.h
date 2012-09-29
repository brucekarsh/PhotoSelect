#ifndef WORKER_H__
#define WORKER_H__

#include <iostream>
#include <set>
#include <boost/thread.hpp>
#include "WorkList.h"
#include "ConversionEngine.h"
extern WorkList work_list;

class Worker {
  private:
    boost::shared_ptr<boost::thread> m_thread;

  public:
    void operator()() {
      do_work();
    }

    void do_work() {
      while(1) {
        try {
          bool blocking = true;
          WorkItem work_item;
          bool b = work_list.get_next_work_item(work_item, blocking);
          if (b) {
            process_work_item(work_item);
          } else {
            std::cout << "No workitem" << std::endl;
          }
        } catch(int signum) {
	  // The workList has shut down. We just exit the thread.
          break;
        }
      }
    }

    void process_work_item(const WorkItem &work_item) {
      int ICON_WIDTH = MultiPhotoPage::ICON_WIDTH;
      int ICON_HEIGHT = MultiPhotoPage::ICON_HEIGHT;
      int ICON_STRIDE = MultiPhotoPage::ICON_STRIDE;
      std::string photofile_name =
          (work_item.multiPhotoPage)->get_photofile_name(work_item.index);

      int rotation = work_item.rotation;
      ConvertedPhotoFile *convertedPhotoFile = ConversionEngine::getConvertedPhotoFile(
          photofile_name, ICON_WIDTH, ICON_WIDTH, rotation);
      int width = convertedPhotoFile->width;
      int height = convertedPhotoFile->height;
      double M;
      (work_item.multiPhotoPage)->calculate_scaling(M, width, height, ICON_WIDTH,
          ICON_HEIGHT);
      unsigned char *pixels = convertedPhotoFile->scale_and_pan_and_rotate(
          ICON_WIDTH, ICON_HEIGHT, M, 0.0, 0.0, rotation);
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
      bool b = ticket_registry.reference_ticket(work_item.ticket_number);
      if (b) {
        (work_item.multiPhotoPage)->set_thumbnail(work_item.index, pixbuf, rotation);
        ticket_registry.unreference_ticket(work_item.ticket_number);
      } else {
        // The class that we made this pixbuf for is gone, so we balk.
        g_object_unref(pixbuf);
      }
    }
};
#endif  // WORKER_H__
