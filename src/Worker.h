#ifndef WORKER_H__
#define WORKER_H__

#include <gtk/gtk.h>
#include <boost/thread.hpp>
#include "WorkList.h"
#include "StockThumbnails.h"

extern StockThumbnails *stock_thumbnails;
extern WorkList work_list;

class Worker {
  private:
    boost::shared_ptr<boost::thread> m_thread;
    static int static_worker_num;
    int my_worker_num;

    enum {
      DELIVERY_OK,	  // Delivery succeeded
      DELIVERY_REJECTED,    // Delivery failed. Retry will not work
      DELIVERY_RETRY_LATER  // Delivery temporarily failed. queue is full
    };

  public:
    Worker();
    void operator()();
    void do_work();
    GdkPixbuf *build_pixbuf_from_work_item(const WorkItem &work_item);
    int deliver_pixbuf_to_multiphotopage(GdkPixbuf *pixbuf, const WorkItem& work_item);
};
#endif  // WORKER_H__
