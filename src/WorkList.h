#ifndef WORKLIST_H__
#define WORKLIST_H__
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <map>
#include <unordered_map>	// requires -std=gnu++0x
#include <algorithm>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include "WorkItem.h"

class WorkList {
  public:

    WorkList() : is_shutdown(false) {}

    void add_work(const WorkItem &work_item, long priority) {
      boost::lock_guard<boost::mutex> member_lock(class_mutex);
      // TODO: assert work_item.ticket_number != 0;

      // If this work item is already present, remove it.
      delete_work_item(work_item);

      // Now add it
      priority_by_work_item_map[work_item] = priority;
      work_item_by_priority_map.insert(work_item_by_priority_entry_t(priority, work_item));
    }

    void delete_work_by_ticket_number(long ticket_number) {
      boost::lock_guard<boost::mutex> member_lock(class_mutex);

      // Go through work_item_by_priority_map and delete matching entries
      for (work_item_by_priority_iterator_t it = work_item_by_priority_map.begin();
          it != work_item_by_priority_map.end(); ) {

        work_item_by_priority_iterator_t next_it = it; // get the next iterator before invalidate
        next_it++;

        if (it->second.ticket_number == ticket_number) {
	    work_item_by_priority_map.erase(it);
        }
        it = next_it;
      }

      // Go through priority_by_work_item_map and delete matching entries
      for (priority_by_work_item_iterator_t it = priority_by_work_item_map.begin();
          it != priority_by_work_item_map.end(); ) {

        priority_by_work_item_iterator_t next_it = it; // get the next iterator before invalidate
        next_it++;

        if (it->first.ticket_number == ticket_number) {
	    priority_by_work_item_map.erase(it);
        }
        it = next_it;
      }
    }

    bool is_work_item_available() {
      boost::lock_guard<boost::mutex> member_lock(class_mutex);
      return !priority_by_work_item_map.empty();
    }

    void shutdown_work_list() {
      is_shutdown = true;
    }

    //! Throws zero if the WorkList is shut down.
    bool get_next_work_item(WorkItem &work_item, bool is_blocking) throw(int) {
      boost::lock_guard<boost::mutex> member_lock(class_mutex);
      bool have_item = false;
      do {
        if (is_shutdown) {
          throw(0);
        }
        work_item_by_priority_iterator_t it = work_item_by_priority_map.begin();
        if (it != work_item_by_priority_map.end()) {
          work_item = it->second;
          work_item_by_priority_map.erase(it);
          priority_by_work_item_map.erase(work_item);
          have_item = true;
        } else {
          usleep(10000); // TODO: Use a CV and wait for work to appear
        }
      } while (is_blocking && !have_item);
      return have_item;
    }

    void complete_work_item(const WorkItem &work_item) {
      boost::lock_guard<boost::mutex> member_lock(class_mutex);
    }
  private:

    void delete_work_item(const WorkItem &work_item) {
      // Find it in the priority_by_work_item_map
      priority_by_work_item_iterator_t it = priority_by_work_item_map.find(work_item);

      // Make sure that it's there
      if (it != priority_by_work_item_map.end()) {

        // It's there.  Remember its priority so we can find more quickly it in the
        // work_item_by_priority_map. Then erase it.
        long priority = it->second;
        priority_by_work_item_map.erase(it);

        // Now look through the entries with its priority in the work_item_by_priority_map and
        // erase it there too.
        std::pair<work_item_by_priority_iterator_t, work_item_by_priority_iterator_t>
            equal_range = work_item_by_priority_map.equal_range(priority);
        for (work_item_by_priority_iterator_t p = equal_range.first; p != equal_range.second; ) {
          if ((*p).second == work_item) {
            work_item_by_priority_map.erase(p);
            break;
          } else {
            ++p;
          }
        }
      }
    }

    boost::mutex class_mutex;

    typedef std::unordered_map<WorkItem, long>           priority_by_work_item_map_t;
    typedef std::unordered_map<WorkItem, long>::iterator priority_by_work_item_iterator_t;
    typedef std::pair<WorkItem, long>                    priority_by_work_item_entry_t;
    priority_by_work_item_map_t priority_by_work_item_map;

    typedef std::multimap<long, WorkItem>           work_item_by_priority_map_t;
    typedef std::multimap<long, WorkItem>::iterator work_item_by_priority_iterator_t;
    typedef std::pair<long, WorkItem>          work_item_by_priority_entry_t;
    work_item_by_priority_map_t work_item_by_priority_map;
    bool is_shutdown;
};

#endif  // WORKLIST_H__
