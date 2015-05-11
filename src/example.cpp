#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include "example.h"

void write_binary_example(struct example *ex, int nExamples, const char *wFile){
  FILE *f = fopen(wFile, "wb");
  assert(f != NULL);
  // write the number of examples to the first byte
  fwrite(&nExamples,sizeof(int),1,f);
  // then write each struct out
  fwrite(ex,sizeof(struct example),nExamples,f);
  fclose(f);
}

struct example *read_binary_examples(const char *rFile, int &nRows, int &nCols, int &nExamples, std::vector<int> &colHits){
  FILE *f = fopen(rFile, "rb");
  assert(f != NULL);
  // read the number of examples into nExamples, make sure we always read 1 int
  assert(fread(&nExamples, sizeof(int), 1, f) == 1);
  struct example *ret = new example[nExamples];

  // now read nExamples worth of structs into our return array, and assert we got em all
  assert(fread(ret,sizeof(struct example),nExamples,f) == (size_t)nExamples);
  fclose(f);

  nRows=0;nCols=0;
  // we know our max row b/c rows go in order, so just take the max
  for(int i=0; i<nExamples;i++){
    nRows = std::max(ret[i].row,nRows);
    nCols = std::max(ret[i].col,nCols);
    //@TODO make this not resize with every-ish access
    if (nCols > colHits.size()) {
      colHits.resize(nCols+1);
    }
    if (ret[i].col >= 0) {
      colHits[ret[i].col] += 1;
    }
  }
  nRows++;nCols++; // we have to count the last line
  return ret;
} 


