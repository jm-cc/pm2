
#ifndef DIALOG_IS_DEF
#define DIALOG_IS_DEF

typedef void (*dialog_func_t)(gpointer);

void dialog_prompt(char *question,
		   char *choice1, dialog_func_t func1, gpointer data1,
		   char *choice2, dialog_func_t func2, gpointer data2,
		   char *choice3, dialog_func_t func3, gpointer data3);

#endif
