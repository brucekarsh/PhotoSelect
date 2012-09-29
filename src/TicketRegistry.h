#ifndef TICKET_REGISTRY_H__
#define TICKET_REGISTRY_H__
#include <boost/thread.hpp>

//! Keeps track of objects which receive callbacks from Worker.
//! It assigns each one a unique ticket number and maintains a map of
//! their ticket number and reference count. They object must wait for
//! its ticket's reference count to go to zero before it can be destroyed.
class TicketRegistry {
  private:
    std::map<long, int> ticket_map; // Map from ticket to reference count
    boost::mutex class_mutex;
    boost::condition_variable class_condition_variable;
    long next_ticket_number;
  public:
    TicketRegistry() : next_ticket_number(123000000) {}

    //! Assigns a new ticket and adds it with a reference count of 1.
    //! \return the ticket number
    long new_ticket() {
      boost::unique_lock<boost::mutex> member_lock(class_mutex);
      long new_ticket_number = next_ticket_number++;
      BOOST_ASSERT(0 == ticket_map.count(new_ticket_number));
      ticket_map[new_ticket_number] = 1;
      return new_ticket_number;
    }

    //! Increases the reference count of a ticket by one.
    //! \return false if the ticket is not in the map, true otherwise
    bool reference_ticket(long ticket_number) {
      boost::unique_lock<boost::mutex> member_lock(class_mutex);
      if (0 == ticket_map.count(ticket_number)) {
        return false;
      }
      ticket_map[ticket_number]++;
      return true;
    }

    //! Decreases the reference count of a ticket by one.
    //! If the reference count goes to zero, it wakes up threads waiting for the ticket.
    void unreference_ticket(long ticket_number) {
      boost::unique_lock<boost::mutex> member_lock(class_mutex);
      BOOST_ASSERT(0 != ticket_map.count(ticket_number));
      BOOST_ASSERT(0 < ticket_map[ticket_number]);
      ticket_map[ticket_number]--;
      if (0 == ticket_map[ticket_number]) {
        ticket_map.erase(ticket_number);
        class_condition_variable.notify_all();
      }
    }

    int get_reference_count(long ticket_number) {
      boost::unique_lock<boost::mutex> member_lock(class_mutex);
      if (0 == ticket_map.count(ticket_number)) {
        return 0;
      }
      return ticket_map[ticket_number];
    }

    void wait_for_ticket(long ticket_number) {
      boost::unique_lock<boost::mutex> member_lock(class_mutex);
      while (0 != ticket_map.count(ticket_number)) {
        class_condition_variable.wait(member_lock);
      }
    }
};
#endif  // TICKET_REGISTRY_H__
