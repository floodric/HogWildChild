#include <string>
#include <stdio.h>
#include <vector>
#include <cstdlib>
#include <getopt.h>

#include "../example.h"
#include "../sparse_vectors/sparse_svm.h"
#include "../sparse_vectors/sparse_vector.h"
#include "../cycle_timer.h"

#define MAX_FILEPATH 512

void usage() {
  printf("svm_seq [-d] [-e epochs] [-a alpha] [-s stepsize] [-l lambda] -f \"trainfilepath\" -t \"testfilepath\"\n");
}

std::vector<double> *train(int num_epochs, double alpha, double stepsize, double lambda, const char *filepath, bool debug); 
void test(std::vector<double> *x, const char *filepath);

int main(int argc, char* argv[]) {
  int opt;
  double alpha = 0.9;
  double lambda = 0.9;
  double stepsize = 1.;
  int fileflag = 0;
  int e = 20;
  bool debug = false;
  std::string filepath;
  std::string testpath;
  while ((opt = getopt(argc, argv, "de:a:s:l:f:t:")) != -1) {
    switch(opt) {
    case 'f':
      filepath.assign(optarg);
      fileflag |= 1;
      break;
    case 't':
      testpath.assign(optarg);
      fileflag |= 2;
      break;
    case 's':
      stepsize = std::atof(optarg);
      break;
    case 'l':
      lambda = std::atof(optarg);
      break;
    case 'a':
      alpha = std::atof(optarg);
      break;
    case 'e':
      e = std::atoi(optarg);
      break;
    case 'd':
      debug = true;
      break;
    default:
      usage();
      exit(0);
    }
  }
  if (fileflag != 3) {
    usage();
    exit(0);
  }

  //printf("Calling train\n");
  std::vector<double> *x = train(e, alpha, stepsize, lambda, filepath.c_str(), debug);
  test(x, testpath.c_str());
  delete x;

  return 0;
}

double apply(std::vector<double> &x, SparseVector &z) {
  return SparseVector::dot(x,z) > 0. ? 1. : -1.;
}

std::vector<double> *train(int num_epochs, double alpha, double stepsize, double lambda, const char *filepath, bool debug) {
  int rows=0, cols=0, nExamples=0;

  std::vector<int> d(0); // d[i] = number of data points that use column i
  //printf("Reading binary\n");
  struct example *example_data = read_binary_examples(filepath,rows,cols,nExamples,d); 
  //printf("Reading complete\n");
  
  int nVectors = 0;
  //printf("Parsing data\n");
  struct sparse_example *exs = parse_examples(nExamples, example_data, nVectors);
  //printf("Parsed %d vectors\n", nVectors);
  delete example_data;

  
  std::vector<double> *x = new std::vector<double>(cols);

  double total_time = 0.;
  for (int epoch = 0; epoch < num_epochs; epoch++) {
    //printf("Starting Epoch %d\n", epoch);
    double start = CycleTimer::currentSeconds();
    for (int i = 0; i < nVectors; i++) {
      SparseVector *z = exs[i].sv;
      double v = (double)exs[i].value;
      //printf("value %f\tsv: %d %p %p\n",v,z->nValues, (void*)z->values, (void*)z->indexes);
      if (v*SparseVector::dot(*x,*z) < 1.) {
        SparseVector::UpdateDecVariable(*x,d,*z,v,stepsize,lambda);
      }
    }
    double epoch_time = CycleTimer::currentSeconds() - start;
    total_time += epoch_time;
    //printf("Epoch %d finished in %fs; total time: %fs\n", epoch, epoch_time, total_time);

    stepsize = stepsize*alpha;
  }
  printf("Total running time: %fs\n", total_time);

  if (!debug) {
    int miss = 0;
    // Get the error on the training set
    for (int i = 0; i < nVectors; i++) {
      if (apply(*x,*exs[i].sv) != exs[i].value) {
        miss++;
      }
    }
    printf("Error on training data: %d/%d = %f%%\n", miss, nVectors, 100.*((double) miss / (double) nVectors));
  }
  delete exs;
  return x;
}

void test(std::vector<double> *x, const char *filepath) {
  int rows=0, cols=0, nExamples=0;

  std::vector<int> d(0); // d[i] = number of data points that use column i
  //printf("Reading binary\n");
  struct example *example_data = read_binary_examples(filepath,rows,cols,nExamples,d); 
  //printf("Reading complete\n");
  
  int nVectors = 0;
  //printf("Parsing data\n");
  struct sparse_example *exs = parse_examples(nExamples, example_data, nVectors);
  //printf("Parsed %d vectors\n", nVectors);
  delete example_data;

  // Get the error after running each epoch
  int miss = 0;
  for (int i = 0; i < nVectors; i++) {
    if (apply(*x,*exs[i].sv) != exs[i].value) {
      miss++;
    }
  }
  printf("Error on test data: %d/%d = %f%%\n", miss, nVectors, 100.*((double) miss / (double) nVectors));
  delete exs;
}
