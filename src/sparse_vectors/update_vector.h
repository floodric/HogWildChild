#include <vector>
#include <stdio.h>
#include <cstring>
#include "sparse_vector.h"

#ifndef _update_vector_h
#define _update_vector_h 

class UpdateVector {
 private:
  struct ll {
    struct ll *next;
    struct ll *prev;
    int count;
    double value;
    int index;

    ll(int idx, double v) : next(NULL), prev(NULL), count(1), value(v), index(idx) {}
  };


 public:
  ll *data;

  UpdateVector() {
    data = NULL;
  }

  static inline struct ll *insert(UpdateVector &u, double v, int index, struct ll *pos) {
    // assert(pos->prev->index < index <= pos->index)
    // e.g., This is the correct place to insert it
    if (pos == NULL) {
      u.data = new ll(index, v);
      return u.data;
    } else if (pos->index == index) {
      pos->value += v;
      pos->count++;
      return pos;
    } else {
      struct ll *newLL = new ll(index,v);
      if (pos->prev == NULL) {
        u.data = newLL;
      } else {
        pos->prev->next = newLL;
        newLL->prev = pos->prev;
      }
      pos->prev = newLL;
      newLL->next = pos;
      return newLL;
    }
  }

  static inline struct ll *remove(UpdateVector &u, struct ll *pos) {
    struct ll *tmp = pos->next;
    if (pos->prev == NULL) {
      u.data = pos->next;
    } else {
      pos->prev->next = pos->next;
    }

    if (pos->next != NULL) {
      pos->next->prev = pos->prev;
    }
    delete pos;
    return tmp;
  }

  static inline void FlushUpdates(std::vector<double> &x, UpdateVector &u) {
    struct ll *tmp;
    while (u.data != NULL) {
      x[u.data->index] += u.data->value;
      tmp = u.data;
      u.data = u.data->next;

      delete tmp;
    }
  }

  static inline void UpdateDecVariable(std::vector<double> &x, std::vector<int> &d, SparseVector &y, 
                                       double v, double stepsize, double lambda,
                                       UpdateVector &u, int threshold) {
    struct ll *uData = u.data;
    double xVal, change;
    int idx;
    for (int i = 0; i < y.nValues; i++) {
      idx = y.indexes[i];

      while (uData != NULL && uData->index < idx) {
        uData = uData->next;
      }

      // If we have a pending update, apply it
      // Otherwise just use x[idx]
      xVal = (uData != NULL && uData->index == idx) ? x[idx] + uData->value : x[idx];

      // x -= stepsize * grad_z(x)
      change = 0. - stepsize * (((2 * lambda * xVal) / d[idx]) - y.values[i] * v);

      if (d[idx] > threshold) {
        uData = insert(u, change, idx, uData);

        // If we've seen enough changes to this variable, flush them
        if (uData->count > ((d[idx] + threshold - 1) / threshold)) {
          x[idx] += uData->value;
          uData = remove(u,uData);
        }
      } else {
        x[idx] += change;
      }
    }
    return;
  };
};

#endif
