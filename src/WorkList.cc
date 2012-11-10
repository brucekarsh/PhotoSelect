#include "WorkList.h"

#include "WorkItem.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <stdlib.h>
#include <unordered_map>        // requires -std=gnu++0x

using namespace std;

WorkList::WorkList() : is_shutdown(false) {}

void WorkList::add_work(const WorkItem &work_item) {
  boost::lock_guard<boost::mutex> member_lock(class_mutex);
  long priority = work_item.priority;
  // TODO: assert work_item.ticket_number != 0;

  // If this work item is already present, remove it.
  delete_work_item(work_item);

  // Now add it
  priority_by_work_item_map[work_item] = priority;
  work_item_by_priority_map.insert(work_item_by_priority_entry_t(priority, work_item));
}

void WorkList::delete_work_by_ticket_number(long ticket_number) {
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

bool WorkList::is_work_item_available() {
  boost::lock_guard<boost::mutex> member_lock(class_mutex);
  return !priority_by_work_item_map.empty();
}

void WorkList::shutdown_work_list() {
  is_shutdown = true;
}

//! Tries to get a WorkItem. Optionally blocks if none available.
//! Throws zero if the WorkList is shut down.
bool WorkList::get_next_work_item(WorkItem &work_item, bool is_blocking) throw(int) {
  while (1) {
    bool have_item = gnwi(work_item);
    if (have_item) {
      return have_item;
    }
    if (is_shutdown) {
      throw (0);
    }
    if (!is_blocking) {
      return false;
    }
    usleep(10000); // TODO: Use a CV and wait for work to appear
  }
}

//! Tries to get a WorkItem. If it can, it removes it an returns it.
//! \return true if it got a WorkItem, false otherwise
bool WorkList::gnwi(WorkItem &work_item) {
  boost::lock_guard<boost::mutex> member_lock(class_mutex);
  bool have_item = false;
  work_item_by_priority_reverse_iterator_t rit = work_item_by_priority_map.rbegin();
  if (rit != work_item_by_priority_map.rend()) {
    work_item_by_priority_iterator_t it = rit.base();
    it--;
    work_item = it->second;
    work_item_by_priority_map.erase(it);
    priority_by_work_item_map.erase(work_item);
    have_item = true;
  }
  return have_item;
}

void WorkList::complete_work_item(const WorkItem &work_item) {
  boost::lock_guard<boost::mutex> member_lock(class_mutex);
}

void WorkList::delete_work_item(const WorkItem &work_item) {
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
    pair<work_item_by_priority_iterator_t, work_item_by_priority_iterator_t>
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
