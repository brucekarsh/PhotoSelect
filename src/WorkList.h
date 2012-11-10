#ifndef WORKLIST_H__
#define WORKLIST_H__
#include <boost/thread.hpp>
#include <map>
#include <unordered_map>	// requires -std=gnu++0x
#include "WorkItem.h"

//! Maintains two lists of WorkItems, one priority -> WorkItem and one is WorkItem -> priority
class WorkList {
  public:
    WorkList();
    void add_work(const WorkItem &work_item);
    void delete_work_by_ticket_number(long ticket_number);
    bool is_work_item_available();
    void shutdown_work_list();

    //! Tries to get a WorkItem. Optionally blocks if none available.
    //! Throws zero if the WorkList is shut down.
    bool get_next_work_item(WorkItem &work_item, bool is_blocking) throw(int);

    //! Tries to get a WorkItem. If it can, it removes it an returns it.
    //! \return true if it got a WorkItem, false otherwise
    bool gnwi(WorkItem &work_item);
    void complete_work_item(const WorkItem &work_item);

  private:

    void delete_work_item(const WorkItem &work_item);
    boost::mutex class_mutex;

    typedef std::unordered_map<WorkItem, long>           priority_by_work_item_map_t;
    typedef std::unordered_map<WorkItem, long>::iterator priority_by_work_item_iterator_t;
    typedef std::pair<WorkItem, long>                    priority_by_work_item_entry_t;
    priority_by_work_item_map_t priority_by_work_item_map;

    typedef std::multimap<long, WorkItem>           work_item_by_priority_map_t;
    typedef std::multimap<long, WorkItem>::iterator work_item_by_priority_iterator_t;
    typedef std::multimap<long, WorkItem>::reverse_iterator
        work_item_by_priority_reverse_iterator_t;
    typedef std::pair<long, WorkItem>          work_item_by_priority_entry_t;
    work_item_by_priority_map_t work_item_by_priority_map;
    bool is_shutdown;
};
#endif  // WORKLIST_H__
