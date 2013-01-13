/* -*- mode: c; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
			/* Ensure this is NULL-terminated. */
			argv = g_new0(const char *, items + 1);
			argv[0] = cmd;
			for(i = 1; i < items; i++) {
				argv[i] = SvPV_nolen(ST(i));
			}
			rv = owl_function_command_argv(argv, items);
			g_free(argv);
		}
		RETVAL = rv;
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		g_free(rv);

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

int
getnumlines()
	CODE:
		RETVAL = owl_global_get_lines(&g);
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
		g_free(rv);

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
		g_free(rv);

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
		g_free(rv);

SV *
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

		RETVAL = owl_perlconfig_message2hashref(m);
	}
	OUTPUT:
		RETVAL

void
admin_message(header, body)
	const char *header
	const char *body
	CODE:
	{
		owl_function_adminmsg(header, body);		
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
		owl_fmtext_cleanup(&fm);
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
		s = g_new(owl_style, 1);
		owl_style_create_perl(s, name, newSVsv(object));
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
		g_free(rv);

AV*
all_filters()
	PREINIT:
		GPtrArray *fl;
	CODE:
	{
		fl = owl_dict_get_keys(&g.filters);
		RETVAL = owl_new_av(fl, (SV*(*)(const void*))owl_new_sv);
		sv_2mortal((SV*)RETVAL);
		owl_ptr_array_free(fl, g_free);
	}
	OUTPUT:
		RETVAL

AV*
all_styles()
	PREINIT:
		GPtrArray *l;
	CODE:
	{
		l = owl_global_get_style_names(&g);
		RETVAL = owl_new_av(l, (SV*(*)(const void*))owl_new_sv);
		sv_2mortal((SV*)RETVAL);
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		owl_ptr_array_free(l, g_free);


AV*
all_variables()
	PREINIT:
		GPtrArray *l;
	CODE:
	{
		l = owl_dict_get_keys(owl_global_get_vardict(&g));
		RETVAL = owl_new_av(l, (SV*(*)(const void*))owl_new_sv);
		sv_2mortal((SV*)RETVAL);
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		owl_ptr_array_free(l, g_free);


AV*
all_keymaps()
	PREINIT:
		GPtrArray *l;
		const owl_keyhandler *kh;
	CODE:
	{
		kh = owl_global_get_keyhandler(&g);
		l = owl_keyhandler_get_keymap_names(kh);
		RETVAL = owl_new_av(l, (SV*(*)(const void*))owl_new_sv);
		sv_2mortal((SV*)RETVAL);
	}
	OUTPUT:
		RETVAL
	CLEANUP:
		owl_ptr_array_free(l, g_free);

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


MODULE = BarnOwl		PACKAGE = BarnOwl::Zephyr

int
have_zephyr()
	CODE:
		RETVAL = owl_global_is_havezephyr(&g);
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
		cmd.cmd_perl = newSVsv(func);
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

void
start_edit(edit_type, line, callback)
	const char *edit_type
	const char *line
	SV *callback
	PREINIT:
		owl_editwin *e;
	CODE:
	{
		if (!SV_IS_CODEREF(callback))
			croak("Callback must be a subref");

		if (!strcmp(edit_type, "question"))
			e = owl_function_start_question(line);
		else if (!strcmp(edit_type, "password"))
			e = owl_function_start_password(line);
		else if (!strcmp(edit_type, "edit_win"))
			e = owl_function_start_edit_win(line);
		else
			croak("edit_type must be one of 'password', 'question', 'edit_win', not '%s'", edit_type);

		owl_editwin_set_cbdata(e, newSVsv(callback), owl_perlconfig_dec_refcnt);
		owl_editwin_set_callback(e, owl_perlconfig_edit_callback);
	}

int
zephyr_zwrite(cmd,msg)
	const char *cmd
	const char *msg
	CODE:
		RETVAL = owl_zwrite_create_and_send_from_line(cmd, msg);
	OUTPUT:
		RETVAL

MODULE = BarnOwl		PACKAGE = BarnOwl::Editwin

int
replace(count, string)
	int count;
	const char *string;
	PREINIT:
		owl_editwin *e;
	CODE:
		e = owl_global_current_typwin(&g);
		if (e) {
			RETVAL = owl_editwin_replace(e, count, string);
		} else {
			RETVAL = 0;
		}
	OUTPUT:
		RETVAL

int
point_move(delta)
	int delta;
	PREINIT:
		owl_editwin *e;
	CODE:
		e = owl_global_current_typwin(&g);
		if (e) {
			RETVAL = owl_editwin_point_move(e, delta);
		} else {
			RETVAL = 0;
		}
	OUTPUT:
		RETVAL

int
replace_region(string)
	const char *string;
	PREINIT:
		owl_editwin *e;
	CODE:
		e = owl_global_current_typwin(&g);
		if (e) {
			RETVAL = owl_editwin_replace_region(e, string);
		} else {
			RETVAL = 0;
		}
	OUTPUT:
		RETVAL

const utf8 *
get_region()
	PREINIT:
		char *region;
		owl_editwin *e;
	CODE:
		e = owl_global_current_typwin(&g);
		if (e) {
			region = owl_editwin_get_region(owl_global_current_typwin(&g));
		} else {
			region = NULL;
		}
		RETVAL = region;
	OUTPUT:
		RETVAL
	CLEANUP:
		g_free(region);

SV *
save_excursion(sub)
	SV *sub;
	PROTOTYPE: &
	PREINIT:
		int count;
		owl_editwin *e;
		owl_editwin_excursion *x;
	CODE:
	{
		e = owl_global_current_typwin(&g);
		if(!e)
			croak("The edit window is not currently active!");

		x = owl_editwin_begin_excursion(owl_global_current_typwin(&g));
		PUSHMARK(SP);
		count = call_sv(sub, G_SCALAR|G_EVAL|G_NOARGS);
		SPAGAIN;
		owl_editwin_end_excursion(owl_global_current_typwin(&g), x);

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
	PREINIT:
		owl_editwin *e;
	CODE:
		e = owl_global_current_typwin(&g);
		if (e) {
			RETVAL = owl_editwin_current_column(e);
		} else {
			RETVAL = 0;
		}
	OUTPUT:
		RETVAL

int
point()
	PREINIT:
		owl_editwin *e;
	CODE:
		e = owl_global_current_typwin(&g);
		if (e) {
			RETVAL = owl_editwin_get_point(e);
		} else {
			RETVAL = 0;
		}
	OUTPUT:
		RETVAL

int
mark()
	PREINIT:
		owl_editwin *e;
	CODE:
		e = owl_global_current_typwin(&g);
		if (e) {
			RETVAL = owl_editwin_get_mark(e);
		} else {
			RETVAL = 0;
		}
	OUTPUT:
		RETVAL
