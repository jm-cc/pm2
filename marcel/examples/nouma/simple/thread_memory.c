#include "marcel.h"

typedef struct thread_memory_s {
  void *address;
  size_t size;
  int node;
  marcel_t *thread;
} thread_memory_t;

int thread_memory_attach(void *address, size_t size, int node, thread_memory_t* thread_memory) {
}

int thread_memory_move(thread_memory_t* thread_memory, int node) {
}

int thread_memory_detach(thread_memory_t* thread_memory) {
}

int main(int argc, char * argv[]) {
  marcel_init(&argc,argv);
}

