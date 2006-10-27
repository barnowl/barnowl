/* -*- mode: c; indent-tabs-mode: t; c-basic-offset: 8 -*- */
static const char fileIdent[] = "$Id$";

#ifdef HAVE_LIBZEPHYR
#include <zephyr/zephyr.h>
#endif
#include <EXTERN.h>

#define OWL_PERL
#include "owl.h"
SV *owl_perlconfig_curmessage2hashref(void);

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

void
new_command_internal(name, func, summary, usage, description)
	char *name
	SV *func
	char *summary
	char *usage
	char *description
	PREINIT:
		owl_cmd cmd;
	CODE:
		SvREFCNT_inc(func);
		cmd.name = name;
		cmd.cmd_perl = func;
		cmd.summary = summary;
		cmd.usage = usage;
		cmd.description = description;
		cmd.validctx = OWL_CTX_ANY;
		cmd.cmd_aliased_to = NULL;
		cmd.cmd_args_fn = NULL;
		cmd.cmd_v_fn = NULL;
		cmd.cmd_i_fn = NULL;
		cmd.cmd_ctxargs_fn = NULL;
		cmd.cmd_ctxv_fn = NULL;
		cmd.cmd_ctxi_fn = NULL;
		owl_cmddict_add_cmd(owl_global_get_cmddict(&g), &cmd);

void queue_message(msg) 
	SV *msg
	PREINIT:
		char * key;
		char * val;
		owl_message *m;
		HV * hash;
		HE * ent;
		I32 count;
		I32 len;
	CODE:
	{
		if(!SvROK(msg) || SvTYPE(SvRV(msg)) != SVt_PVHV) {
			croak("Usage: owl::queue_message($message)");
		}
		
		hash = (HV*)SvRV(msg);
		m = owl_malloc(sizeof(owl_message));
		owl_message_init(m);
		
		count = hv_iterinit(hash);
		while((ent = hv_iternext(hash))) {
			key = hv_iterkey(ent, &len);
			val = SvPV_nolen(hv_iterval(hash, ent));
			if(!strcmp(key, "type")) {
				owl_message_set_type(m, owl_message_parse_type(val));
			} else if(!strcmp(key, "direction")) {
				owl_message_set_direction(m, owl_message_parse_direction(val));
			} else {
				owl_message_set_attribute(m, key, val);
			}
		}
		owl_message_set_isprivate(m);
		if(owl_message_is_type_admin(m)) {
			if(!owl_message_get_attribute_value(m, "adminheader"))
				owl_message_set_attribute(m, "adminheader", "");
		} 

		owl_global_messagequeue_addmsg(&g, m);
	}
