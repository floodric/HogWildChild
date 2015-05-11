#ifndef _EXAMPLES_H__
#define _EXAMPLES_H__
#include <vector>

struct example {
  int row; // row represents vector number
  int col; // col represents position in vector
  double rating; // represents output for this col in vector (or output for vector if col == -1)
};

// will write into a binary file named wFile, and will write nExamples of data from ex
void write_binary_examples(struct example *ex, int nExamples, const char *wFile);

// reads examples from binary file named rFile, and will update values of nRows, nCols, and nExamples
struct example *read_binary_examples(const char *rFile, int &nRows, int &nCols, int &nExamples, std::vector<int> &colHits);

#endif
