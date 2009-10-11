/* -*- mode: c; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifdef HAVE_LIBZEPHYR
#include <zephyr/zephyr.h>
#endif
#include <EXTERN.h>

#define OWL_PERL
#include "owl.h"

#define SV_IS_CODEREF(sv) (SvROK((sv)) && SvTYPE(SvRV((sv))) == SVt_PVCV)

typedef char utf8;

	/*************************************************************
	 * NOTE
	 *************************************************************
	 * These functions, when they are intended to be user-visible,
	 * are documented in perl/lib/BarnOwl.pm. If you add functions
	 * to this file, add the appropriate documentation there!
	 *
	 * If the function is simple enough, we simply define its
	 * entire functionality here in XS. If, however, it needs
	 * complex argument processing or something, we define a
	 * simple version here that takes arguments in as flat a
	 * manner as possible, to simplify the XS code, put it in
	 * BarnOwl::Internal::, and write a perl wrapper in BarnOwl.pm
	 * that munges the arguments as appropriate and calls the
	 * internal version.
	 */

MODULE = BarnOwl		PACKAGE = BarnOwl

const utf8 *
command(cmd, ...)
	const char *cmd
	PREINIT:
		char *rv = NULL;
		const char **argv;
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

const utf8 *
zephyr_getrealm()
	CODE:
		RETVAL = owl_zephyr_get_realm();
	OUTPUT:
		RETVAL

const utf8 *
zephyr_getsender()
	CODE:
		RETVAL = owl_zephyr_get_sender();
	OUTPUT:
		RETVAL

void
zephyr_zwrite(cmd,msg)
	const char *cmd
	const char *msg
	PREINIT:
		int i;
	CODE:
		i = owl_zwrite_create_and_send_from_line(cmd, msg);

const utf8 *
ztext_stylestrip(ztext)
	const char *ztext
	PREINIT:
		char *rv = NULL;
	CODE:
		rv = owl_function_ztext_stylestrip(ztext);
		RETVAL = rv;
	OUTPUT:
		RETVAL
	CLEANUP:
		if (rv) owl_free(rv);

const utf8 *
zephyr_smartstrip_user(in)
	const char *in
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

const utf8 *
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

void
queue_message(msg)
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

void
admin_message(header, body)
	const char *header
	const char *body
	CODE:
	{
		owl_function_adminmsg(header, body);		
	}

void
start_question(line, callback)
	const char *line
	SV *callback
	PREINIT:
	CODE:
	{
		if(!SV_IS_CODEREF(callback))
			croak("Callback must be a subref");

		owl_function_start_question(line);

		owl_editwin_set_cbdata(owl_global_get_typwin(&g), SvREFCNT_inc(callback));
		owl_editwin_set_callback(owl_global_get_typwin(&g), owl_perlconfig_edit_callback);
	}

void
start_password(line, callback)
	const char *line
	SV *callback
	PREINIT:
	CODE:
	{
		if(!SV_IS_CODEREF(callback))
			croak("Callback must be a subref");

		owl_function_start_password(line);

		owl_editwin_set_cbdata(owl_global_get_typwin(&g), SvREFCNT_inc(callback));
		owl_editwin_set_callback(owl_global_get_typwin(&g), owl_perlconfig_edit_callback);
	}

void
start_edit_win(line, callback)
	const char *line
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

		owl_editwin_set_cbdata(owl_global_get_typwin(&g), SvREFCNT_inc(callback));
		owl_editwin_set_callback(owl_global_get_typwin(&g), owl_perlconfig_edit_callback);
	}


const char * 
get_data_dir ()
	CODE:
		RETVAL = owl_get_datadir();
	OUTPUT:
	RETVAL

const char * 
get_config_dir ()
	CODE:
		RETVAL = owl_global_get_confdir(&g);
	OUTPUT:
	RETVAL	

void
popless_text(text) 
	const char *text
	CODE:
	{
		owl_function_popless_text(text);
	}

void
popless_ztext(text) 
	const char *text
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
	const char *text
	CODE:
	{
		owl_function_error("%s", text);
	}

void
debug(text)
	const char *text
	CODE:
	{
		owl_function_debugmsg("%s", text);
	}

void
message(text)
	const char *text
	CODE:
	{
		owl_function_makemsg("%s", text);
	}

void
create_style(name, object)
     const char *name
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
	const char *filterName
	CODE:
	{
		/* Don't delete the current view, or the 'all' filter */
		if (strcmp(filterName, owl_view_get_filtname(owl_global_get_current_view(&g)))
		    && strcmp(filterName, "all")) {
			owl_global_remove_filter(&g,filterName);
		}
	}

const utf8 *
wordwrap(in, cols)
	const char *in
	int cols
	PREINIT:
		char *rv = NULL;
	CODE:
		rv = owl_text_wordwrap(in, cols);
		RETVAL = rv;	
	OUTPUT:
		RETVAL
	CLEANUP:
		if (rv)
			owl_free(rv);

void
add_dispatch(fd, cb)
	int fd
	SV * cb
	CODE:
	owl_select_add_perl_dispatch(fd, SvREFCNT_inc(cb));

void
remove_dispatch(fd)
	int fd
	CODE:
	owl_select_remove_perl_dispatch(fd);


AV*
all_filters()
	PREINIT:
		AV *filters;
		const owl_list *fl;
		const owl_filter *f;
		int i;
	CODE:
	{
		fl = owl_global_get_filterlist(&g);
		filters = newAV();
		for(i=0;i<owl_list_get_size(fl);i++) {
			f = owl_list_get_element(fl, i);
			av_push(filters, owl_new_sv(owl_filter_get_name(f)));
		}
		RETVAL = filters;
		sv_2mortal((SV*)RETVAL);
	}
	OUTPUT:
		RETVAL

AV*
all_styles()
	PREINIT:
		AV *styles;
		owl_list l;
		const char *name;
		int i;
	CODE:
	{
		owl_list_create(&l);
		owl_global_get_style_names(&g, &l);
		styles = newAV();
		for(i=0;i<owl_list_get_size(&l);i++) {
			name = owl_list_get_element(&l, i);
			av_push(styles, owl_new_sv(name));
		}
		RETVAL = styles;
		sv_2mortal((SV*)RETVAL);
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		owl_list_free_all(&l, owl_free);


AV*
all_variables()
	PREINIT:
		AV *vars;
		owl_list l;
		const char *name;
		int i;
	CODE:
	{
		owl_list_create(&l);
		owl_dict_get_keys(owl_global_get_vardict(&g), &l);
		vars = newAV();
		for(i=0;i<owl_list_get_size(&l);i++) {
			name = owl_list_get_element(&l, i);
			av_push(vars, owl_new_sv(name));
		}
		RETVAL = vars;
		sv_2mortal((SV*)RETVAL);
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		owl_list_free_all(&l, owl_free);

void
redisplay()
	CODE:
	{
		owl_messagelist_invalidate_formats(owl_global_get_msglist(&g));
		owl_function_calculate_topmsg(OWL_DIRECTION_DOWNWARDS);
		owl_mainwin_redisplay(owl_global_get_mainwin(&g));
	}

const char *
get_zephyr_variable(name)
	const char *name;
	CODE:
		RETVAL = owl_zephyr_get_variable(name);
	OUTPUT:
		RETVAL

const utf8 *
skiptokens(str, n)
	const char *str;
	int n;
	CODE:
		RETVAL = skiptokens(str, n);
	OUTPUT:
		RETVAL



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
		cmd.name = name;
		cmd.cmd_perl = SvREFCNT_inc(func);
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
	const char * name
	const char * ival
	const char * summ
	const char * desc
	CODE:
	owl_variable_dict_newvar_string(owl_global_get_vardict(&g),
					name,
					summ,
					desc,
					ival);

void
new_variable_int(name, ival, summ, desc)
	const char * name
	int ival
	const char * summ
	const char * desc
	CODE:
	owl_variable_dict_newvar_int(owl_global_get_vardict(&g),
				     name,
				     summ,
				     desc,
				     ival);

void
new_variable_bool(name, ival, summ, desc)
	const char * name
	int ival
	const char * summ
	const char * desc
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

MODULE = BarnOwl		PACKAGE = BarnOwl::Editwin

int
replace(count, string)
	int count;
	const char *string;
	CODE:
		RETVAL = owl_editwin_replace(owl_global_get_typwin(&g), count, string);
	OUTPUT:
		RETVAL

int
point_move(delta)
	int delta;
	CODE:
		RETVAL = owl_editwin_point_move(owl_global_get_typwin(&g), delta);
	OUTPUT:
		RETVAL

int
replace_region(string)
	const char *string;
	CODE:
		RETVAL = owl_editwin_replace_region(owl_global_get_typwin(&g), string);
	OUTPUT:
		RETVAL

const utf8 *
get_region()
	PREINIT:
		char *region;
	CODE:
		region = owl_editwin_get_region(owl_global_get_typwin(&g));
		RETVAL = region;
	OUTPUT:
		RETVAL
	CLEANUP:
		owl_free(region);

SV *
save_excursion(sub)
	SV *sub;
	PROTOTYPE: &
	PREINIT:
		int count;
		owl_editwin_excursion *x;
	CODE:
	{
		x = owl_editwin_begin_excursion(owl_global_get_typwin(&g));
		PUSHMARK(SP);
		count = call_sv(sub, G_SCALAR|G_EVAL|G_NOARGS);
		SPAGAIN;
		owl_editwin_end_excursion(owl_global_get_typwin(&g), x);

		if(SvTRUE(ERRSV)) {
			croak(NULL);
		}

		if(count == 1)
			RETVAL = SvREFCNT_inc(POPs);
		else
			XSRETURN_UNDEF;

	}
	OUTPUT:
		RETVAL

int
current_column()
	CODE:
		RETVAL = owl_editwin_current_column(owl_global_get_typwin(&g));
	OUTPUT:
		RETVAL

int
point()
	CODE:
		RETVAL = owl_editwin_get_point(owl_global_get_typwin(&g));
	OUTPUT:
		RETVAL

int
mark()
	CODE:
		RETVAL = owl_editwin_get_mark(owl_global_get_typwin(&g));
	OUTPUT:
		RETVAL
