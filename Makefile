INCLUDES=src src/sparse_vectors src/sequential
IFLAGS=$(addprefix -I ,$(INCLUDES))
CFLAGS= -Wall -g
CC=clang++ $(CFLAGS) $(IFLAGS)

OBJDIR=objs

SEQ_OBJECTS=$(OBJDIR)/sparse_vectors.o $(OBJDIR)/example.o $(OBJDIR)/sequential.o
PAR_OBJECTS=$(OBJDIR)/sparse_vectors.o $(OBJDIR)/example.o $(OBJDIR)/parallel.o
HOG_OBJECTS=$(OBJDIR)/sparse_vectors.o $(OBJDIR)/example.o $(OBJDIR)/hogwild.o
CHD_OBJECTS=$(OBJDIR)/sparse_vectors.o $(OBJDIR)/example.o $(OBJDIR)/hogwildchild.o

all : $(SEQ_OBJECTS) $(PAR_OBJECTS) $(HOG_OBJECTS) $(CHD_OBJECTS)
	clang++ -o hogwild_seq $(SEQ_OBJECTS)
	clang++ -pthread -o hogwild_naive $(PAR_OBJECTS)
	clang++ -pthread -o hogwild $(HOG_OBJECTS)
	clang++ -pthread -o hogwildchild $(CHD_OBJECTS)

$(OBJDIR)/sequential.o : src/sequential/svm_seq.cpp
	$(CC) -c -o $(OBJDIR)/sequential.o src/sequential/svm_seq.cpp

$(OBJDIR)/sparse_vectors.o : src/sparse_vectors/sparse_svm.h src/sparse_vectors/sparse_vector.h
	$(CC) -c -o $(OBJDIR)/sparse_vectors.o src/sparse_vectors/sparse_svm.cpp

$(OBJDIR)/example.o : src/example.h src/example.cpp
	$(CC) -c -o $(OBJDIR)/example.o src/example.cpp

$(OBJDIR)/parallel.o : src/parallel/naive_parallel.cpp  
	# because you cant compile without the pthead flag
	$(CC) -c -pthread -o $(OBJDIR)/parallel.o src/parallel/naive_parallel.cpp

$(OBJDIR)/hogwild.o : src/parallel/hogwild.cpp
	$(CC) -c -pthread -o $(OBJDIR)/hogwild.o src/parallel/hogwild.cpp

$(OBJDIR)/hogwildchild.o : src/parallel/hogwildchild.cpp
	$(CC) -c -pthread -o $(OBJDIR)/hogwildchild.o src/parallel/hogwildchild.cpp

clean: 
	rm hogwildchild hogwild hogwild_seq hogwild_naive $(OBJDIR)/*.o
