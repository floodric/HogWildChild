#include "sparse_vector.h"

struct sparse_example {
  SparseVector *sv;
  double value;
sparse_example(SparseVector *_sv, double v) : sv(_sv), value (v) { } 
sparse_example() : sv(NULL), value(0.0) {}
};

struct sparse_example *parse_examples(int nExamples, struct example *e, int &nVectors);
