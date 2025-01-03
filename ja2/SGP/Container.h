// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

//***********************************************
//
// Filename : Container.h
//
// Purpose : prototypes for the container file
//
// Modification History : 25 Nov 96 Creation
//
//***********************************************

#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdexcept>

#include "SGP/Types.h"

typedef struct QueueHeader *HQUEUE;
typedef struct ListHeader *HLIST;

// Queue Functions
// CreateQueue(estimated number of items in queue, size of each item
// AddtoQueue(handle to container returned from CreateQueue, data to be passed
// in (must be void *)) : returns handle to queue RemfromQueue(handle to
// container returned from CreateQueue, variable where data is stored (must be
// void *)) : returns BOOLEAN QueueSize(handle to the queue) returns the queue
// size DeleteQueue(handle to container) Delete the queue container : returns
// BOOLEAN
HQUEUE CreateQueue(uint32_t num_of_elem, uint32_t siz_of_each);
HQUEUE AddtoQueue(HQUEUE hQueue, void const *data);
void RemfromQueue(HQUEUE hQueue, void *data);
uint32_t QueueSize(HQUEUE hQueue);
BOOLEAN DeleteQueue(HQUEUE hQueue);

namespace SGP {
template <typename T>
class Queue {
 public:
  Queue(uint32_t const n_elements) : queue_(CreateQueue(n_elements, (uint32_t)sizeof(T))) {}

  ~Queue() { DeleteQueue(queue_); }

  bool IsEmpty() const { return QueueSize(queue_) == 0; }

  void Add(T const &data) { queue_ = AddtoQueue(queue_, &data); }

  T Remove() {
    T data;
    RemfromQueue(queue_, &data);
    return data;
  }

 private:
  HQUEUE queue_;
};
}  // namespace SGP

// List Functions
// CreateList(estimated number of items in queue, size of each item
// AddtoList(handle to container returned from CreateQueue, data to be passed in
// (must be void *)
//          position where data is to be added (0...sizeof(list))
// : returns handle to new list
// RemfromList(handle to container returned from CreateList, variable where data
// is stored (must be void *)
//          position where data is to be deleted (0...sizeof(list)-1)
// PeekList(handle to the list, variable where peeked data is stored). Item is
// not deleted.
//          position where data is to be peeked (0...sizeof(list)-1)
// ListSize(handle to the list) returns the list size
// DeleteList(handle to the list) Delete the list container

extern HLIST CreateList(uint32_t num_of_elem, uint32_t siz_of_each);
extern HLIST AddtoList(HLIST hList, void const *data, uint32_t position);
void RemfromList(HLIST, void *data, uint32_t pos);
void PeekList(HLIST, void *data, uint32_t pos);
extern uint32_t ListSize(HLIST hList);
extern BOOLEAN DeleteList(HLIST hList);

namespace SGP {
template <typename T>
class List {
 public:
  List(uint32_t const n_elements) : list_(CreateList(n_elements, sizeof(T))) {}

  ~List() { DeleteList(list_); }

  uint32_t Size() const { return ListSize(list_); }

  void Add(T const &data, uint32_t const pos) {
    HLIST l = AddtoList(list_, &data, pos);
    /* XXX cannot distinguish between invalid pos and failed memory
     * allocation here */
    if (!l) throw std::runtime_error("Failed to add element to list");
    list_ = l;
  }

  T Remove(uint32_t const pos) {
    T data;
    RemfromList(list_, &data, pos);
    return data;
  }

  T Peek(uint32_t const pos) {
    T data;
    PeekList(list_, &data, pos);
    return data;
  }

 private:
  HLIST list_;
};
}  // namespace SGP

#endif
