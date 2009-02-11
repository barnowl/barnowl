/* -*- mode: c; indent-tabs-mode: t; c-basic-offset: 8 -*- */
static const char fileIdent[] = "$Id$";

#ifdef HAVE_LIBZEPHYR
#include <zephyr/zephyr.h>
#endif
#include <EXTERN.h>

#define OWL_PERL
#include "owl.h"
SV *owl_perlconfig_curmessage2hashref(void);

#define SV_IS_CODEREF(sv) (SvROK((sv)) && SvTYPE(SvRV((sv))) == SVt_PVCV)

	/*************************************************************
	 * NOTE
	 *************************************************************
	 * These functions, when they are intended to be user-visible,
	 * are documented in perlwrap.pm. If you add functions to this
	 * file, add the appropriate documentation there!
	 *
	 * If the function is simple enough, we simply define its
	 * entire functionality here in XS. If, however, it needs
	 * complex argument processing or something, we define a
	 * simple version here that takes arguments in as flat a
	 * manner as possible, to simplify the XS code, put it in
	 * BarnOwl::Intenal::, and write a perl wrapper in perlwrap.pm
	 * that munges the arguments as appropriate and calls the
	 * internal version.
	 */

MODULE = BarnOwl		PACKAGE = BarnOwl

char *
command(cmd, ...)
	char *cmd
	PREINIT:
		char *rv = NULL;
		char **argv;
		int i;
	CODE:
	{
		if (items == 1) {
			rv = owl_function_command(cmd);
		} else {
			argv = owl_malloc((items + 1) * sizeof *argv);
			argv[0] = cmd;
			for(i = 1; i < items; i++) {
				argv[i] = SvPV_nolen(ST(i));
			}
			rv = owl_function_command_argv(argv, items);
			owl_free(argv);
		}
		RETVAL = rv;
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		if (rv) owl_free(rv);

SV *
getcurmsg()
	CODE:
		RETVAL = owl_perlconfig_curmessage2hashref();
	OUTPUT:
		RETVAL

int
getnumcols()
	CODE:
		RETVAL = owl_global_get_cols(&g);
	OUTPUT:
		RETVAL
		
time_t
getidletime()
	CODE:
		RETVAL = owl_global_get_idletime(&g);
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

char *
zephyr_smartstrip_user(in)
	char *in
	PREINIT:
		char *rv = NULL;
	CODE:
	{
		rv = owl_zephyr_smartstripped_user(in);
		RETVAL = rv;
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		owl_free(rv);

char *
zephyr_getsubs()
	PREINIT:
		char *rv = NULL;
	CODE:
		rv = owl_zephyr_getsubs();
		RETVAL = rv;
    OUTPUT:
		RETVAL
    CLEANUP:
		if (rv) owl_free(rv);

void queue_message(msg) 
	SV *msg
	PREINIT:
		owl_message *m;
	CODE:
	{
		if(!SvROK(msg) || SvTYPE(SvRV(msg)) != SVt_PVHV) {
			croak("Usage: BarnOwl::queue_message($message)");
		}

		m = owl_perlconfig_hashref2message(msg);

		owl_global_messagequeue_addmsg(&g, m);
	}

void admin_message(header, body) 
	char *header
	char *body
	CODE:
	{
		owl_function_adminmsg(header, body);		
	}

void start_question(line, callback)
	char *line
	SV *callback
	PREINIT:
	CODE:
	{
		if(!SV_IS_CODEREF(callback))
			croak("Callback must be a subref");

		owl_function_start_question(line);

		SvREFCNT_inc(callback);
		owl_editwin_set_cbdata(owl_global_get_typwin(&g), callback);
		owl_editwin_set_callback(owl_global_get_typwin(&g), owl_perlconfig_edit_callback);
	}

void start_password(line, callback)
	char *line
	SV *callback
	PREINIT:
	CODE:
	{
		if(!SV_IS_CODEREF(callback))
			croak("Callback must be a subref");

		owl_function_start_password(line);

		SvREFCNT_inc(callback);
		owl_editwin_set_cbdata(owl_global_get_typwin(&g), callback);
		owl_editwin_set_callback(owl_global_get_typwin(&g), owl_perlconfig_edit_callback);
	}

void start_edit_win(line, callback)
	char *line
	SV *callback
	PREINIT:
		owl_editwin * e;
		char buff[1024];
	CODE:
	{
		if(!SV_IS_CODEREF(callback))
			croak("Callback must be a subref");

		e = owl_global_get_typwin(&g);
		owl_editwin_new_style(e, OWL_EDITWIN_STYLE_MULTILINE,
				      owl_global_get_msg_history(&g));
		owl_editwin_clear(e);
		owl_editwin_set_dotsend(e);
		snprintf(buff, 1023, "----> %s\n", line);
		owl_editwin_set_locktext(e, buff);

		owl_global_set_typwin_active(&g);

		SvREFCNT_inc(callback);
		owl_editwin_set_cbdata(owl_global_get_typwin(&g), callback);
		owl_editwin_set_callback(owl_global_get_typwin(&g), owl_perlconfig_edit_callback);
	}


char * 
get_data_dir ()
	CODE:
		RETVAL = (char *) owl_get_datadir();
	OUTPUT:
	RETVAL

char * 
get_config_dir ()
	CODE:
		RETVAL = (char *) owl_global_get_confdir(&g);
	OUTPUT:
	RETVAL	

void
popless_text(text) 
	char *text
	CODE:
	{
		owl_function_popless_text(text);
	}

void
popless_ztext(text) 
	char *text
	CODE:
	{
		owl_fmtext fm;
		owl_fmtext_init_null(&fm);
		owl_fmtext_append_ztext(&fm, text);
		owl_function_popless_fmtext(&fm);
		owl_fmtext_free(&fm);
	}

void
error(text) 
	char *text
	CODE:
	{
		owl_function_error("%s", text);
	}

void
create_style(name, object)
     char *name
     SV  *object
     PREINIT:
		owl_style *s;
     CODE:
	{
		s = owl_malloc(sizeof(owl_style));
		owl_style_create_perl(s, name, object);
		owl_global_add_style(&g, s);
	}

int
getnumcolors()
	CODE:
		RETVAL = owl_function_get_color_count();
	OUTPUT:
		RETVAL

void
_remove_filter(filterName)
	char *filterName
	CODE:
	{
		/* Don't delete the current view, or the 'all' filter */
		if (strcmp(filterName, owl_view_get_filtname(owl_global_get_current_view(&g)))
		    && strcmp(filterName, "all")) {
			owl_global_remove_filter(&g,filterName);
		}
	}

char *
wordwrap(in, cols)
	char *in
	int cols
	PREINIT:
		char *rv = NULL;
	CODE:
		rv = owl_text_wordwrap(in, cols);
		RETVAL = rv;	
	OUTPUT:
		RETVAL
	CLEANUP:
		if (rv) owl_free(rv);

void
add_dispatch(fd, cb)
	int fd
	SV * cb
	CODE:
        SvREFCNT_inc(cb);
	owl_select_add_perl_dispatch(fd, cb);

void
remove_dispatch(fd)
	int fd
	CODE:
	owl_select_remove_perl_dispatch(fd);

MODULE = BarnOwl		PACKAGE = BarnOwl::Internal


void
new_command(name, func, summary, usage, description)
	char *name
	SV *func
	char *summary
	char *usage
	char *description
	PREINIT:
		owl_cmd cmd;
	CODE:
	{
		if(!SV_IS_CODEREF(func)) {
			croak("Command function must be a coderef!");
		}
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
	   }

void
new_variable_string(name, ival, summ, desc)
	char * name
	char * ival
	char * summ
	char * desc
	CODE:
	owl_variable_dict_newvar_string(owl_global_get_vardict(&g),
					name,
					summ,
					desc,
					ival);

void
new_variable_int(name, ival, summ, desc)
	char * name
	int ival
	char * summ
	char * desc
	CODE:
	owl_variable_dict_newvar_int(owl_global_get_vardict(&g),
				     name,
				     summ,
				     desc,
				     ival);

void
new_variable_bool(name, ival, summ, desc)
	char * name
	int ival
	char * summ
	char * desc
	CODE:
	owl_variable_dict_newvar_bool(owl_global_get_vardict(&g),
				      name,
				      summ,
				      desc,
				      ival);

IV
add_timer(after, interval, cb)
	int after
	int interval
	SV *cb
	PREINIT:
		SV *ref;
		owl_timer *t;
	CODE:
		ref = sv_rvweaken(newSVsv(cb));
		t = owl_select_add_timer(after,
					 interval,
					 owl_perlconfig_perl_timer,
					 owl_perlconfig_perl_timer_destroy,
					 ref);
	owl_function_debugmsg("Created timer %p", t);
	RETVAL = (IV)t;
	OUTPUT:
		RETVAL

void
remove_timer(timer)
	IV timer
	PREINIT:
		owl_timer *t;
	CODE:
		t = (owl_timer*)timer;
		owl_function_debugmsg("Freeing timer %p", t);
				owl_select_remove_timer(t);
