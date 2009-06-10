#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <numaif.h>
#include <sys/mman.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PAGES 4
void *buffer;

static void
print_pagenodes(void **pageaddrs)
{
  int pagenodes[PAGES];
  int i;
  int err;

  err = move_pages(0, PAGES, pageaddrs, NULL, pagenodes, 0);
  if (err < 0) {
    perror("move_pages (print_pagenodes)");
    exit(-1);
  }

  for(i=0; i<PAGES; i++) {
    if (pagenodes[i] == -ENOENT)
      printf("  page #%d isn't allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, pagenodes[i]);
  }
}

static void
set_bind(void **pageaddrs, void *buffer, unsigned long length, int node)
{
  unsigned long nodemask;
  unsigned long maxnode;
  int err;

  int i, nodes[PAGES], status[PAGES];

  printf("binding on numa node #%d\n", node);

  for(i=0; i<PAGES ; i++)
    nodes[i] = node;

  err = move_pages(0, PAGES, pageaddrs, nodes, status, MPOL_MF_MOVE);
  if (err < 0) {
    perror("move_pages (set_bind)");
    exit(-1);
  }

  for(i=0; i<PAGES; i++) {
    if (status[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, status[i]);
  }

//  maxnode = numa_max_node()+2;
//  nodemask = (1<<node);
//  printf("binding on numa node #%d (mask %d, maxnode %d)\n", node, nodemask, maxnode);
//#if 0
//  err = mbind(buffer, length, MPOL_BIND, &nodemask, maxnode, MPOL_MF_MOVE);
//  if (err < 0) {
//    perror("mbind");
//    exit(-1);
//  }
//#else
//  err = set_mempolicy(MPOL_BIND, &nodemask, maxnode);
//  if (err < 0) {
//    perror("set_mempolicy");
//    exit(-1);
//  }
//#endif
}

static void*
alloc_anonymous_shared(unsigned long length)
{
  //  void * buffer;

  buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }

  return buffer;
}

#define PATH_MAX 1024

static void*
alloc_backend_unlinked_shared(unsigned long length)
{
  //  void * buffer;
  char path[PATH_MAX];
  int fd;
  int err;

  snprintf(path, PATH_MAX, "/tmp/test-migrate-on-touch-%d-%d", getuid(), getpid());
  fd = open(path, O_CREAT|O_EXCL|O_RDWR, S_IRWXU);
  if (fd < 0) {
    perror("open");
    exit(-1);
  }

  err = lseek(fd, length, SEEK_SET);
  if (err < 0) {
    perror("lseek");
    exit(-1);
  }

  err = write(fd, path, 1);
  if (err < 0) {
    perror("write");
    exit(-1);
  }

  err = unlink(path);
  if (err < 0) {
    perror("unlink");
    exit(-1);
  }

  buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }

  return buffer;
}

int
main(int argc, char *argv[])
{
  //  void * buffer;
  unsigned long pagesize = getpagesize();
  void **pageaddrs;
  int i;
  int err;

//#if 0
//  buffer = alloc_anonymous_shared(PAGES*pagesize);
//#else
  buffer = alloc_backend_unlinked_shared(PAGES*pagesize);
//#endif

//  buffer = malloc(PAGES * pagesize * sizeof(unsigned long));

  pageaddrs = malloc(PAGES * sizeof(void *));
  for(i=0; i<PAGES; i++)
    pageaddrs[i] = buffer + i*pagesize;

  //set_bind(pageaddrs, buffer, (PAGES-2)*pagesize, 0);

  printf("allocated but not touched yet:\n");
  print_pagenodes(pageaddrs);

  for(i=0; i<PAGES-2; i++)
    *(int*)(buffer + i*pagesize) = 0x12345678;

  printf("after touching %d pages with 0x12345678:\n", PAGES-2);
  print_pagenodes(pageaddrs);

  err = madvise(buffer, PAGES*pagesize, MADV_DONTNEED);
  if (err < 0) {
    perror("madvise");
    exit(-1);
  }

  printf("after madvise dontneed:\n");
  print_pagenodes(pageaddrs);

  set_bind(pageaddrs, buffer, PAGES*pagesize, numa_max_node());

  printf("reading again to bring pages back to memory:\n");
  for(i=0; i<PAGES; i++)
    printf("  page #%d contains 0x%x\n", i, *(int*)(buffer + i*pagesize));

  printf("after bringing back to memory:\n");
  print_pagenodes(pageaddrs);

  return 0;
}
