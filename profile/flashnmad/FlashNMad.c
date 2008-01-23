#include <stdio.h>
#include <stdlib.h>
#include <parsing.h>
#include <FlashEngineInit.h>

nmad_state_t nmad_status;

static void usage(char *prog)
{
	printf("%s log-path swf-path\n", prog);
}

int main(int argc, char **argv)
{
	char *logpath;	
	char *swfpath;	

	if (argc < 3) {
		usage(argv[0]);
		exit(-1);
	}

	/* first parse the log to determine the machine set up */
	logpath = argv[1];
	preprocess_log(&nmad_status, logpath);

	/* now do the actual parsing */
	process_log(&nmad_status, logpath);

	/* render the flash output into a file */
	swfpath = argv[2];
	flash_engine_generate_output(&nmad_status, swfpath); 
	
	
	return 0;
}
