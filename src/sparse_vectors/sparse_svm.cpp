#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <assert.h>
#include "sparse_svm.h"
#include "../example.h"

// taken from hogwild
struct sparse_example *parse_examples(int nExamples, struct example *e, int &nVectors){
//  printf("sizeof(nVectors)=%lu\n",sizeof(nVectors));
  nVectors = 1;
  int last_row = e[0].row; // This prevents the first vector from counting
//  printf("sizeof(e[0].row)=%lu\n",sizeof(e[0].row));
//  printf("sizeof(last_row)=%lu\n",sizeof(last_row));
  for(int i = 0; i < nExamples; i++) {
    if(last_row != e[i].row) nVectors++;
    last_row = e[i].row;
  }

  // Assume sorted by (row,col,rating)
  // if col == -1, then this is the value
  struct sparse_example *ret = new struct sparse_example[nVectors];
  int i = 0, curVec = 0;
  const int local_max_size = 65536;
  double dCache[local_max_size];
  int    iCache[local_max_size];
  double rating = 0.0;
  while(i < nExamples) {
    assert(curVec < nVectors);
    // find the current span (num in row)
    int span = 0, delay = 0;
    last_row = e[i].row;
    while(i+span < nExamples && e[i+span].row == last_row) span++;
    assert(span < local_max_size);

    //printf("row %d, span %d\n",curVec,span);
    // Copy out. (entire row)
    for(int k = 0; (k < span); k++) {
      if(e[i+k].col < 0) {
        assert(delay == 0);
        rating = e[i+k].rating;
        delay  = 1;
      } else {
        dCache[k-delay] = e[i+k].rating;
        iCache[k-delay] = e[i+k].col;
        //std::cerr << "  -- " << e[i+k].col << std::endl;
      }
    }
    assert(delay == 1 && (span - 1) >= 0);
    ret[curVec].sv    = new SparseVector(dCache, iCache, span - 1);
    ret[curVec].value = rating;
    // Advance
    i += span;
    curVec++;
  }
  printf("\n");
  assert(i == nExamples);
  return ret;
}
