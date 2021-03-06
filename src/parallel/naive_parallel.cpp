#include <string>
#include <assert.h>
#include <stdio.h>
#include <queue>
#include <vector>
#include <cstdlib>
#include <getopt.h>
#include <pthread.h>

#include "../example.h"
#include "../sparse_vectors/sparse_svm.h"
#include "../sparse_vectors/sparse_vector.h"
#include "../cycle_timer.h"

#define MAX_FILEPATH 512
#define BATCH_SIZE 512

pthread_t *threads;
struct thread_args {
  int id;
  double alpha;
  double lambda;
};

int num_workers;
int t_allocation;
int nVectors;
int num_epochs;

struct sparse_example *train_examples; 
std::vector<int> col_hits(0); // d[i] = number of data points that use column i
std::vector<double> *decision_vec;

pthread_mutex_t dec_lock;
pthread_barrier_t epoch_barrier;
pthread_barrier_t start_barrier;  

double alpha = 0.9;
double lambda = 0.9;
double stepsize = 1.;
int fileflag = 0;
int e = 20;

void usage() {
  printf("svm_seq [-p num_workers] [-d] [-e epochs] [-a alpha] [-s stepsize] [-l lambda] -f \"trainfilepath\" -t \"testfilepath\"\n");
}

std::vector<double> *train(double alpha, double lambda, const char *filepath, bool debug); 
void test(const char *filepath);
void *thread_work(void *args);
void thread_init(double lambda, double alpha);

int main(int argc, char* argv[]) {
  int opt;
  num_workers = 12;
  alpha = 0.9;
  lambda = 0.9;
  stepsize = 1.;
  int fileflag = 0;
  e = 20;
  bool debug = false;
  std::string filepath;
  std::string testpath;
  while ((opt = getopt(argc, argv, "p:de:a:s:l:f:t:")) != -1) {
    switch(opt) {
    case 'p':
      num_workers = std::atoi(optarg);
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
  threads = (pthread_t*)malloc(num_workers*sizeof(pthread_t));
  printf("-- Running on %d threads. --\n", num_workers);

  thread_init(lambda, alpha);
  num_epochs = e;
  decision_vec = train(alpha, lambda, filepath.c_str(), debug);
  test(testpath.c_str());

  for(int worker=0; worker<num_workers; worker++){
    void *thread_ret;
    pthread_join(threads[worker], &thread_ret);
  }
  pthread_mutex_destroy(&dec_lock);
  pthread_barrier_destroy(&epoch_barrier);
  pthread_barrier_destroy(&start_barrier);
  delete decision_vec;

  return 0;
}

void thread_init(double lambda, double alpha){
  pthread_mutex_init(&dec_lock,NULL);

  pthread_barrier_init(&epoch_barrier, NULL, num_workers+1);
  pthread_barrier_init(&start_barrier,NULL, num_workers+1);  

  for(int worker=0; worker<num_workers; worker++){
    pthread_t worker_thread;
    struct thread_args *worker_args = (struct thread_args *)malloc(sizeof(struct thread_args));
    worker_args->id = worker;
    worker_args->lambda = lambda;
    worker_args->alpha = alpha;
    
    int ret = pthread_create(&worker_thread, NULL, thread_work, worker_args);
    if(ret != 0){
      printf("Creating threads went wrong\n");
      exit(1);
    }
    threads[worker] = worker_thread;
  }

  return;
}

double apply(SparseVector &z) {
  return SparseVector::dot(*decision_vec,z) > 0. ? 1. : -1.;
}

std::vector<double> *train(double alpha, double lambda, const char *filepath, bool debug) {
  int rows=0, cols=0, nExamples=0;

  if (debug)
    printf("Reading binary\n");
  struct example *example_data = read_binary_examples(filepath,rows,cols,nExamples,col_hits); 
  
  nVectors = 0;
  if (debug)
    printf("Parsing data\n");
  train_examples = parse_examples(nExamples, example_data, nVectors);
  delete example_data;

  // decision vector which we update to find our function
  decision_vec = new std::vector<double>(cols);

  double total_time = 0.;
  t_allocation = nVectors / num_workers;
  pthread_barrier_wait(&start_barrier);

  for (int epoch = 0; epoch < num_epochs; epoch++) {
    double start = CycleTimer::currentSeconds();
    pthread_barrier_wait(&epoch_barrier);
    double epoch_time = CycleTimer::currentSeconds() - start;
    total_time += epoch_time;
    if (debug)
      printf("Epoch %d finished in %fs; total time: %fs\n", epoch, epoch_time, total_time);
  }

  printf("Total running time: %fs\n", total_time);

  if (!debug) {
    int miss = 0;

    pthread_mutex_lock(&dec_lock);
    // Get the error on the training set
    for (int i = 0; i < nVectors; i++) {
      // test our categorization function and compare it to real 'rating'
      if (apply(*train_examples[i].sv) != train_examples[i].value) {
        miss++;
      }
    }

    printf("Error on training data: %d/%d = %f%%\n", miss, nVectors, 100.*((double) miss / (double) nVectors));
    pthread_mutex_unlock(&dec_lock);
  }
  delete train_examples;
  return decision_vec;
}

void test(const char *filepath) {
  int rows=0, cols=0, nExamples=0;

  std::vector<int> d(0); // d[i] = number of data points that use column i
  struct example *example_data = read_binary_examples(filepath,rows,cols,nExamples,d); 
  
  int nVectors = 0;
  struct sparse_example *test_examples = parse_examples(nExamples, example_data, nVectors);
  delete example_data;

  // Get the error after running each epoch
  int miss = 0;
  for (int i = 0; i < nVectors; i++) {
    if (apply(*test_examples[i].sv) != test_examples[i].value) {
      miss++;
    }
  }
  printf("Error on testing data: %d/%d = %f%%\n", miss, nVectors, 100.*((double) miss / (double) nVectors));
  delete test_examples;
}

void *thread_work(void *args){
  struct thread_args *t_args = (struct thread_args *)args;
  double lambda = t_args->lambda;

  double my_stepsize = stepsize;
  pthread_barrier_wait(&start_barrier);  

  SparseVector *train_vectors[BATCH_SIZE];
  double train_labels[BATCH_SIZE];

  for (int epoch = 0; epoch < num_epochs; epoch++) {
    
    double lock_wait = 0;
    double start = CycleTimer::currentSeconds();

    int my_start = t_args->id * t_allocation;
    int my_end = t_args->id == num_workers-1 ? nVectors : my_start + t_allocation;

    for(int i=my_start; i<my_end;i+=BATCH_SIZE){

      SparseVector *train_vec;
      double label;
      
      for(int j=0;j<BATCH_SIZE && i+j < my_end;j++){
        train_vec = train_examples[i+j].sv;
        label = train_examples[i+j].value;
      
        train_vectors[j] = train_vec;
        train_labels[j] = label;
      }

      for(int j=0;j<BATCH_SIZE && i+j < my_end;j++){
        train_vec = train_vectors[j];
        label = train_labels[j]; 

        if (label*SparseVector::dot(*decision_vec,*(train_vec)) < 1.) {

          double lock_start = CycleTimer::currentSeconds();
          pthread_mutex_lock(&dec_lock);
          double lock_end = CycleTimer::currentSeconds() - lock_start;
          lock_wait += lock_end;

          SparseVector::UpdateDecVariable(*decision_vec,col_hits,*(train_vec),label,my_stepsize,lambda);

          pthread_mutex_unlock(&dec_lock);   
        }
      }
    }
    double epoch_time = CycleTimer::currentSeconds() - start;
    // printf
    //if (t_args->id == 0) {
    if (t_args->id == -1) {
      printf("t%d finished epoch %d took time %f, %f waiting on locks\n",t_args->id,epoch,epoch_time, lock_wait);
    }
    pthread_barrier_wait(&epoch_barrier);
    my_stepsize = my_stepsize*alpha;
  }
  return args;
}
