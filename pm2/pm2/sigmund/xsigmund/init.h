#ifndef INIT_H
#define INIT_H

/* Structure to record codes and their names */
struct code_name {
  unsigned int    code;
  char           *name;
};

/* Table of codes : fut */
extern struct code_name code_table[1000];

extern void init_names();

#endif
