// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/Container.h"

#include <stdexcept>
#include <string.h>

#include "SGP/Debug.h"
#include "SGP/MemMan.h"
#include "SGP/Types.h"

struct ListHeader {
  uint32_t uiTotal_items;
  uint32_t uiSiz_of_elem;
  uint32_t uiMax_size;
  uint32_t uiHead;
  uint32_t uiTail;
  uint8_t data[];
};

struct QueueHeader {
  uint32_t uiTotal_items;
  uint32_t uiSiz_of_elem;
  uint32_t uiMax_size;
  uint32_t uiHead;
  uint32_t uiTail;
  uint8_t data[];
};

// Parameter List : num_items - estimated number of items in queue
//									siz_each - size of each
// item
// Return Value	NULL if unsuccesful
//							 pointer to allocated
// memory
HQUEUE CreateQueue(uint32_t const uiNum_items, uint32_t const uiSiz_each) {
  if (uiNum_items == 0 || uiSiz_each == 0) {
    throw std::logic_error("Requested queue items and size have to be >0");
  }

  uint32_t const uiAmount = uiNum_items * uiSiz_each;
  HQUEUE const q = MALLOCE(QueueHeader, data, uiAmount);
  q->uiMax_size = uiAmount + sizeof(QueueHeader);
  q->uiTotal_items = 0;
  q->uiSiz_of_elem = uiSiz_each;
  q->uiTail = sizeof(QueueHeader);
  q->uiHead = sizeof(QueueHeader);
  return q;
}

// Parameter List : num_items - estimated number of items in ordered list
//									siz_each - size of each
// item
// Return Value	NULL if unsuccesful
//							 pointer to allocated
// memory
HLIST CreateList(uint32_t const uiNum_items, uint32_t const uiSiz_each) {
  if (uiNum_items == 0 || uiSiz_each == 0) {
    throw std::logic_error("Requested queue items and size have to be >0");
  }

  uint32_t const uiAmount = uiNum_items * uiSiz_each;
  HLIST const l = MALLOCE(ListHeader, data, uiAmount);
  l->uiMax_size = uiAmount + sizeof(ListHeader);
  l->uiTotal_items = 0;
  l->uiSiz_of_elem = uiSiz_each;
  l->uiTail = sizeof(ListHeader);
  l->uiHead = sizeof(ListHeader);
  return l;
}

BOOLEAN DeleteQueue(HQUEUE hQueue) {
  if (hQueue == NULL) {
    DebugMsg(TOPIC_QUEUE_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the queue");
    return FALSE;
  }
  MemFree(hQueue);
  return TRUE;
}

BOOLEAN DeleteList(HLIST hList) {
  if (hList == NULL) {
    DebugMsg(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the list");
    return FALSE;
  }
  MemFree(hList);
  return TRUE;
}

// PeekList - gets the specified item in the list without actually deleting it.
//
// Parameter List : hList - pointer to list container
//									data - data where list
// element is stored
void PeekList(HLIST const l, void *const data, uint32_t const pos) {
  if (pos >= l->uiTotal_items) {
    throw std::logic_error("Tried to peek at non-existent element in list");
  }

  uint32_t uiOffsetSrc = l->uiHead + pos * l->uiSiz_of_elem;
  if (uiOffsetSrc >= l->uiMax_size)
    uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - l->uiMax_size);

  uint8_t const *const pbyte = (uint8_t *)l + uiOffsetSrc;
  memmove(data, pbyte, l->uiSiz_of_elem);
}

// Parameter List : pvoid_queue - pointer to queue container
//									data - data removed from
// queue
void RemfromQueue(HQUEUE const q, void *const data) {
  if (!q) throw std::logic_error("This is not a valid pointer to the queue");
  if (!data) throw std::logic_error("Memory fo Data to be removed from queue is NULL");
  if (q->uiTotal_items == 0) throw std::logic_error("There is nothing in the queue to remove");

  // remove the element pointed to by uiHead

  uint8_t *const pbyte = (uint8_t *)q + q->uiHead;
  memmove(data, pbyte, q->uiSiz_of_elem);
  q->uiTotal_items--;
  q->uiHead += q->uiSiz_of_elem;

  // if after removing an element head = tail then set them both
  // to the beginning of the container as it is empty

  if (q->uiHead == q->uiTail) q->uiHead = q->uiTail = sizeof(QueueHeader);

  // if only the head is at the end of the container then make it point
  // to the beginning of the container

  if (q->uiHead == q->uiMax_size) q->uiHead = sizeof(QueueHeader);
}

// Parameter List : pvoid_queue - pointer to queue container
//									pdata - pointer to data
// to add to queue
//
// Return Value	pointer to queue with data added
HQUEUE AddtoQueue(HQUEUE q, void const *const pdata) {
  if (!q) throw std::logic_error("This is not a valid pointer to the queue");
  if (!pdata) throw std::logic_error("Data to be added onto queue is NULL");

  BOOLEAN fresize = FALSE;
  uint32_t const uiSize_of_each = q->uiSiz_of_elem;
  uint32_t const uiMax_size = q->uiMax_size;
  uint32_t const uiHead = q->uiHead;
  uint32_t uiTail = q->uiTail;
  if (uiTail + uiSize_of_each > uiMax_size) {
    uiTail = q->uiTail = sizeof(QueueHeader);
    fresize = TRUE;
  }
  if (uiHead == uiTail && (uiHead >= sizeof(QueueHeader) + uiSize_of_each || fresize)) {
    uint32_t const uiNew_size = 2 * uiMax_size - sizeof(QueueHeader);
    q->uiMax_size = uiNew_size;
    q = (HQUEUE)MemRealloc(q, uiNew_size);
    // copy memory from beginning of container to end of container
    // so that all the data is in one continuous block

    if (uiHead > sizeof(QueueHeader)) {
      uint8_t *const presize = (uint8_t *)q + sizeof(QueueHeader);
      uint8_t *const pmaxsize = (uint8_t *)q + uiMax_size;
      memmove(pmaxsize, presize, uiHead - sizeof(QueueHeader));
    }
    q->uiTail = uiMax_size + uiHead - sizeof(QueueHeader);
  }
  uint8_t *const pbyte = (uint8_t *)q + q->uiTail;
  memmove(pbyte, pdata, uiSize_of_each);
  q->uiTotal_items++;
  q->uiTail += uiSize_of_each;
  return q;
}

static void do_copy(void *const pmem_void, uint32_t const uiSourceOfst, uint32_t const uiDestOfst,
                    uint32_t const uiSize) {
  uint8_t *pOffsetSrc = (uint8_t *)pmem_void + uiSourceOfst;
  uint8_t *pOffsetDst = (uint8_t *)pmem_void + uiDestOfst;
  memmove(pOffsetDst, pOffsetSrc, uiSize);
}

static void do_copy_data(void *const pmem_void, void *const data, uint32_t const uiSrcOfst,
                         uint32_t const uiSize) {
  uint8_t *pOffsetSrc = (uint8_t *)pmem_void + uiSrcOfst;
  memmove(data, pOffsetSrc, uiSize);
}

// Parameter List : pointer to queue
//
// Return Value	uint32_t queue size
uint32_t QueueSize(HQUEUE hQueue) {
  if (hQueue == NULL) {
    DebugMsg(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Queue pointer is NULL");
    return 0;
  }
  return hQueue->uiTotal_items;
}

// Parameter List : pointer to queue
//
// Return Value	uint32_t list size
uint32_t ListSize(HLIST hList) {
  if (hList == NULL) {
    DebugMsg(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "List pointer is NULL");
    return 0;
  }
  return hList->uiTotal_items;
}

// Parameter List : HCONTAINER - handle to list container
//									data - data to add to
// queue 									position - position
// after which data is to added
HLIST AddtoList(HLIST l, void const *pdata, uint32_t const uiPos) {
  uint32_t uiOffsetDst;
  uint32_t uiFinalLoc = 0;

  // check for invalid handle = 0
  if (!l) {
    DebugMsg(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "This is not a valid handle to the list");
    return NULL;
  }

  // check for data = NULL
  if (pdata == NULL) {
    DebugMsg(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Data to be pushed onto list is NULL");
    return NULL;
  }

  if (uiPos > l->uiTotal_items) {
    DebugMsg(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "There are not enough elements in the list");
    return NULL;
  }

  uint32_t const uiSize_of_each = l->uiSiz_of_elem;
  uint32_t const uiMax_size = l->uiMax_size;
  uint32_t const uiHead = l->uiHead;
  uint32_t const uiTail = l->uiTail;
  uint32_t uiOffsetSrc = l->uiHead + uiPos * uiSize_of_each;
  if (uiOffsetSrc >= uiMax_size) uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - uiMax_size);
  bool const fTail_check = uiTail == uiOffsetSrc;
  // copy appropriate blocks
  if (uiTail + uiSize_of_each <= uiMax_size &&
      (uiTail > uiHead || (uiTail == uiHead && uiHead == sizeof(ListHeader)))) {
    uiOffsetSrc = l->uiHead + uiPos * uiSize_of_each;
    uiOffsetDst = uiOffsetSrc + uiSize_of_each;
    if (!fTail_check) {
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc);
      l->uiTail += uiSize_of_each;
    }
    uiFinalLoc = uiOffsetSrc;
  }

  if ((uiTail + uiSize_of_each <= uiMax_size && uiTail < uiHead) ||
      (uiTail + uiSize_of_each > uiMax_size && uiHead >= sizeof(ListHeader) + uiSize_of_each)) {
    uiOffsetSrc = l->uiHead + uiPos * uiSize_of_each;

    if (uiOffsetSrc >= uiMax_size) {
      uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - uiMax_size);
      uiOffsetDst = uiOffsetSrc + uiSize_of_each;
      do_copy(l, uiOffsetDst, uiOffsetSrc, uiTail - uiOffsetSrc);
    } else {
      uiOffsetSrc = sizeof(ListHeader);
      uiOffsetDst = uiOffsetSrc + uiSize_of_each;
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc);

      uiOffsetSrc = uiMax_size - uiSize_of_each;
      uiOffsetDst = sizeof(ListHeader);
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiSize_of_each);
      uiOffsetSrc = l->uiHead + uiPos * uiSize_of_each;
      uiOffsetDst = uiOffsetSrc + uiSize_of_each;
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiMax_size - uiSize_of_each - uiOffsetSrc);
    }
    l->uiTail += uiSize_of_each;
    uiFinalLoc = uiOffsetSrc;
  }

  if ((uiTail + uiSize_of_each <= uiMax_size && uiTail == uiHead &&
       uiHead >= sizeof(ListHeader) + uiSize_of_each) ||
      (uiTail + uiSize_of_each > uiMax_size && uiHead == sizeof(ListHeader))) {
    // need to resize the container
    uint32_t const uiNew_size = uiMax_size + (uiMax_size - sizeof(ListHeader));
    l->uiMax_size = uiNew_size;
    l = (HLIST)MemRealloc(l, uiNew_size);
    do_copy(l, sizeof(ListHeader), uiMax_size, uiHead - sizeof(ListHeader));
    l->uiTail = uiMax_size + (uiHead - sizeof(ListHeader));

    // now make place for the actual element
    uiOffsetSrc = l->uiHead + uiPos * uiSize_of_each;
    uiOffsetDst = uiOffsetSrc + uiSize_of_each;
    do_copy(l, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc);
    l->uiTail += uiSize_of_each;
    uiFinalLoc = uiOffsetSrc;
  }

  // finally insert data at position uiFinalLoc
  if (uiFinalLoc == 0) {
    DebugMsg(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "This should never happen! report this problem!");
    return NULL;
  }

  uint8_t *const pbyte = (uint8_t *)l + uiFinalLoc;
  memmove(pbyte, pdata, uiSize_of_each);
  l->uiTotal_items++;
  if (fTail_check) l->uiTail += uiSize_of_each;
  return l;
}

// Parameter List : HLIST - handle to list container
//									data - data to remove
// from list 									position - position
// after which data is to added
void RemfromList(HLIST const l, void *const data, uint32_t const pos) {
  if (pos >= l->uiTotal_items) {
    throw std::logic_error("Tried to remove non-existent element from list");
  }

  uint32_t const uiSize_of_each = l->uiSiz_of_elem;
  uint32_t const uiMax_size = l->uiMax_size;
  uint32_t const uiHead = l->uiHead;
  uint32_t const uiTail = l->uiTail;

  // copy appropriate blocks
  if (uiTail > uiHead || (uiTail == uiHead && uiHead == sizeof(ListHeader))) {
    uint32_t const uiOffsetSrc = l->uiHead + (pos * l->uiSiz_of_elem);
    uint32_t const uiOffsetDst = uiOffsetSrc + l->uiSiz_of_elem;
    do_copy_data(l, data, uiOffsetSrc, uiSize_of_each);
    do_copy(l, uiOffsetDst, uiOffsetSrc, uiTail - uiOffsetSrc);
    l->uiTail -= uiSize_of_each;
    l->uiTotal_items--;
  }

  if (uiTail < uiHead || (uiTail == uiHead && uiHead <= sizeof(ListHeader) + uiSize_of_each)) {
    uint32_t uiOffsetSrc = l->uiHead + (pos * l->uiSiz_of_elem);
    if (uiOffsetSrc >= uiMax_size) {
      uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - uiMax_size);
      uint32_t uiOffsetDst = uiOffsetSrc + uiSize_of_each;
      do_copy_data(l, data, uiOffsetSrc, uiSize_of_each);
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc);
    } else {
      uint32_t uiOffsetSrc = sizeof(ListHeader);
      uint32_t uiOffsetDst = uiOffsetSrc + uiSize_of_each;
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc);

      uiOffsetSrc = uiMax_size - uiSize_of_each;
      uiOffsetDst = sizeof(ListHeader);
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiSize_of_each);
      uiOffsetSrc = l->uiHead + (pos * l->uiSiz_of_elem);
      uiOffsetDst = uiOffsetSrc + uiSize_of_each;
      do_copy_data(l, data, uiOffsetSrc, uiSize_of_each);
      do_copy(l, uiOffsetSrc, uiOffsetDst, uiMax_size - uiSize_of_each - uiOffsetSrc);
    }
    l->uiTail -= uiSize_of_each;
    l->uiTotal_items--;
  }
}
