
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <pthread.h>
#include <marcel.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

void *f(void *foo) {
	return NULL;
}

int main(int argc, char *argv[]) {
	pthread_t t;
	marcel_init(&argc, argv);
	pthread_create(&t,NULL,f,NULL);
	marcel_end();
	return 0;
}
