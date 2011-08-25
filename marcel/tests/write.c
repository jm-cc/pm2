/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2011 "the PM2 team" (see AUTHORS file)
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


#include "marcel.h"


static int do_test(void)
{
	int fd;
	char file[] = "writetestXXXXXX";
	char w_data[] = "coucoucoucou";
	char r_data[32];
	ssize_t b_written, b_read;

	fd = mkstemp(file);
	if (-1 == fd) {
		perror("open");
		return MARCEL_TEST_FAILED;
	}

	/** write data into the file **/
	b_written = marcel_write(fd, w_data, strlen(w_data));
	if (-1 == b_written) {
		perror("write");
		marcel_close(fd);
		return MARCEL_TEST_FAILED;
	}
	if ((size_t)b_written != strlen(w_data)) {
		marcel_fprintf(stderr, "Can't write message in the file\n");
		marcel_close(fd);
		return MARCEL_TEST_FAILED;
	}
	
	/** force flush data to the disk and reset file cursor **/
	if (-1 == marcel_fsync(fd)) {
		perror("fsync");
		marcel_close(fd);
		return MARCEL_TEST_FAILED;
	}

	if (-1 == marcel_lseek(fd, 0, SEEK_SET)) {
		perror("lseek");
		marcel_close(fd);
		return MARCEL_TEST_FAILED;
	}

	/** read data previously written and check them **/
	memset(r_data, 0, sizeof(r_data));
	b_read = marcel_read(fd, r_data, b_written);
	if (-1 == b_read) {
		perror("read");
		marcel_close(fd);
		return MARCEL_TEST_FAILED;
	}
	if (strcmp(r_data, w_data)) {
		marcel_fprintf(stderr, "R/W failed | read: %s -  write: %s\n", r_data, w_data);
		marcel_close(fd);
		return MARCEL_TEST_FAILED;
	}

	marcel_close(fd);
	unlink(file);
	return MARCEL_TEST_SUCCEEDED;
}

int marcel_main(int argc, char *argv[])
{
	int result;

	marcel_init(&argc, argv);
	result = do_test();
	marcel_end();

	return result;
}
