#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* Yeah, we should just include owl.h, but curses and perl don't play nice. */
extern char *owl_function_command(char *cmd);
extern void owl_free(void *x);

MODULE = owl		PACKAGE = owl		

char *
command(cmd)
	char *cmd
	PREINIT:
		char *rv = NULL;
	CODE:
		rv = owl_function_command(cmd);
		RETVAL = rv;	
	OUTPUT:
		RETVAL
	CLEANUP:
		if (rv) owl_free(rv);

