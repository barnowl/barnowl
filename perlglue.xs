static const char fileIdent[] = "$Id$";

#ifdef HAVE_LIBZEPHYR
#include <zephyr/zephyr.h>
#endif
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

/* Yeah, we should just include owl.h, but curses and perl don't play nice. */
extern char *owl_function_command(char *cmd);
extern void owl_free(void *x);
extern SV *owl_perlconfig_curmessage2hashref();
extern int owl_zwrite_create_and_send_from_line(char *, char *);
extern char *owl_function_ztext_stylestrip(char *);
extern void g;
extern int owl_global_get_cols(void*);
extern char *owl_zephyr_get_realm();
extern char *owl_zephyr_get_sender();
 
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

SV *
getcurmsg()
	CODE:
		ST(0) = owl_perlconfig_curmessage2hashref();

int
getnumcols()
	CODE:
		RETVAL = owl_global_get_cols(&g);
	OUTPUT:
		RETVAL

char *
zephyr_getrealm()
	CODE:
		RETVAL = owl_zephyr_get_realm();
	OUTPUT:
		RETVAL

char *
zephyr_getsender()
	CODE:
		RETVAL = owl_zephyr_get_sender();
	OUTPUT:
		RETVAL

void
zephyr_zwrite(cmd,msg)
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
		rv = owl_function_ztext_stylestrip(ztext);
		RETVAL = rv;
	OUTPUT:
		RETVAL
	CLEANUP:
		if (rv) owl_free(rv);

