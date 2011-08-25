
#include <iostream>
using namespace std;

#include "pm2.h"
	
extern "C" {
int pm2_main(int argc, char *argv[]);
}

int pm2_main(int argc, char *argv[])
{
  pm2_init(argc, argv);

  cout << "Hello World!" << endl;

  if(pm2_self() == 0)
    pm2_halt();

  pm2_exit();

  return 0;
}
