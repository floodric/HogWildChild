// sparse_vector.h
// From the hogwild source coude
#include <vector>
#include <stdio.h>
#include <cstring>
#include <assert.h>

#ifndef _sparse_vector_h
#define _sparse_vector_h 

class SparseVector {
 public:
    double *values;
    int    *indexes;
    int nValues;

  SparseVector() {
    values  = NULL;
    indexes = NULL;
    nValues = 0;
  }
 
  // Assumes the indexes of the vectors are in ascending order.
  SparseVector(double *_values, int *_indexes, int _nValues) : nValues(_nValues) {
      //printf("new sv: %d big, %f %d\n",_nValues,_values[0],_indexes[0]);  
      values  = new double[_nValues];
      indexes = new int[_nValues];
      std::memcpy(values , _values , sizeof(double)*nValues);
      std::memcpy(indexes, _indexes, sizeof(int)*nValues);
      // This structural assertion is used to prevent deadlocks!
      // For debugging purposes
      for(int i = 1; i < nValues; i++) { assert( indexes[i-1] < indexes[i] ); } //printf("%6d %s",indexes[i], (i % 10 == 0 ? "\n":"")); }
  }

  // Written by Billy Wood
  static inline double dot(std::vector<double> &x, SparseVector &y) {
    double ret = 0.0;
    for (int i = 0; i < y.nValues; i++) {
      //printf("i: %d, %f,%f \n",i,y.values[i],x[y.indexes[i]]);
      ret += x[y.indexes[i]] * y.values[i];
    }
    return ret;
  };

  static inline void UpdateDecVariable(std::vector<double> &x, std::vector<int> &d, SparseVector &y, double v, double stepsize, double lambda) {
    for (int i = 0; i < y.nValues; i++) {
      // x -= stepsize * grad_z(x)
      x[y.indexes[i]] -= stepsize * (((2 * lambda * x[y.indexes[i]]) / d[y.indexes[i]]) - y.values[i] * v);
    }
    return;
  };
};

#endif
