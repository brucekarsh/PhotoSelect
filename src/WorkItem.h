#ifndef WORKITEM_H__
#define WORKITEM_H__
#include <stdio.h>
#include <stdlib.h>
#include <functional>

class MultiPhotoPage;
class WorkItem {
  public:
    WorkItem(long ticket_number, int index, MultiPhotoPage *multiPhotoPage) :
        ticket_number(ticket_number), index(index), multiPhotoPage(multiPhotoPage) {};
    long ticket_number;
    int index;
    MultiPhotoPage *multiPhotoPage;
    
    // Note thate operator == does not need to check rhs.multiPhotoPage == multiPhotoPage
    // because each multiPhotoPage should have its own unique ticket_number.
    bool operator==(const WorkItem &rhs) const {
      return rhs.ticket_number == ticket_number 
          && rhs.index == index;
    }
    size_t hash() const {
      return std::hash<long>()(ticket_number) ^ std::hash<int>()(index);
      return 3;
    }
};

namespace std {
  template<> class hash<WorkItem> {
    public:
      size_t operator()(const WorkItem &work_item) const {
        return work_item.hash();
      }
  };
}

#endif  // WORKITEM_H__
