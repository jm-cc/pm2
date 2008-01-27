#include<stdio.h>
#include<stdlib.h>
#include <sys/mman.h>

#include "marcel.h"

#ifdef MARCEL_NUMA
int main(int argc, char *argv[]) {

	marcel_init(&argc,argv);
#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

	void* h;
	int *offset1;
	int *offset2;
	int *offset3;
	int i;

	h = ma_hcreate(1024*1024*sizeof(int));
	if (h == NULL) {
		printf("Error: hcreate\n");
		exit(0);
	}
	ma_hmallinfo(h);

	offset1 = ma_hmalloc(10*sizeof(int),h);
	printf("offset1=%x\n",offset1);
	if (offset1 != NULL) {
		for(i = 0; i < 10; ++i) {
			offset1[i] = 1;
		}
	} else {
		printf("Allocation failed\n");
	}
	
	offset2 = ma_hmalloc(20*sizeof(int),h);
	printf("offset2=%x\n",offset2);
	if (offset1 != NULL) {
		for(i = 0; i < 20; ++i) {
			offset2[i] = 2;
		}
	} else {
		printf("Allocation failed\n");
	}

	offset3 = ma_hmalloc(30*sizeof(int),h);
	printf("offset3=%x\n",offset3);
	if (offset1 != NULL) {
		for(i = 0; i < 30; ++i) {
			offset3[i] = 3;
		}
	} else {
		printf("Allocation failed\n");
	}
	for(i = 0; i < 10; ++i) {
		printf("%d ",offset3[i]+offset1[i]+offset2[i]);
	}
	printf("\n");
	
	offset3 = ma_hrealloc(offset3,10*sizeof(int),h);
	printf("offset3=%x\n",offset3);
	ma_hmallinfo(h);

	ma_hfree(offset3,h);
	ma_hmallinfo(h);

	offset3 = ma_hcalloc(30,sizeof(int),h);
	printf("offset3=%x\n",offset3);
	ma_hmallinfo(h);
	
	offset2 = ma_hrealloc(offset2,60*sizeof(int),h);
	printf("offset2=%x\n",offset2);
	ma_hmallinfo(h);

	ma_hfree(offset2,h);
	ma_hmallinfo(h);

	ma_hfree(offset1,h);
	ma_hmallinfo(h);

	offset2 = ma_hmalloc(5*sizeof(int),h);
	printf("offset2=%x\n",offset2);
	ma_hmallinfo(h);

	ma_hdelete(&h);
	printf("heap=%x\n",h);

	profile_stop();

	return EXIT_SUCCESS;
}
#else /* MARCEL_NUMA */
#  warning Marcel NUMA mode must be selected for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr,
	  "'marcel NUMA' mode required in the flavor\n");

  return 0;
}
#endif /* MARCEL_NUMA */

