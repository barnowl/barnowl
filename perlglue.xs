static const char fileIdent[] = "$Id$";

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* Yeah, we should just include owl.h, but curses and perl don't play nice. */
extern char *owl_function_command(char *);
extern void owl_free(void *);
extern int owl_zwrite_create_and_send_from_line(char *, char *);
extern char *owl_fmtext_ztext_stylestrip(char *);

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

void
send_zwrite(cmd,msg)
	char *cmd
	char *msg
	PREINIT:
		int i;
	CODE:
		i = owl_zwrite_create_and_send_from_line(cmd, msg);

char *
ztext_stylestrip(ztext)
	char *ztext
	PREINIT:
		char *rv = NULL;
	CODE:
		rv = owl_fmtext_ztext_stylestrip(ztext);
		RETVAL = rv;
	OUTPUT:
		RETVAL
	CLEANUP:
		if (rv) owl_free(rv);

