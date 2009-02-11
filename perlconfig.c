#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#define OWL_PERL
#include "owl.h"

static const char fileIdent[] = "$Id$";

extern char *owl_perlwrap_codebuff;

extern XS(boot_BarnOwl);
extern XS(boot_DynaLoader);
/* extern XS(boot_DBI); */

static void owl_perl_xs_init(pTHX)
{
  char *file = __FILE__;
  dXSUB_SYS;
  {
    newXS("BarnOwl::bootstrap", boot_BarnOwl, file);
    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
  }
}

SV *owl_perlconfig_message2hashref(owl_message *m)
{
  HV *h, *stash;
  SV *hr;
  char *ptr, *blessas, *type;
  int i, j;
  owl_pair *pair;
  owl_filter *wrap;

  if (!m) return &PL_sv_undef;
  wrap = owl_global_get_filter(&g, "wordwrap");
  if(!wrap) {
      owl_function_error("wrap filter is not defined");
      return &PL_sv_undef;
  }

  h = newHV();

#define MSG2H(h,field) (void)hv_store(h, #field, strlen(#field),        \
                                      newSVpv(owl_message_get_##field(m),0), 0)

  if (owl_message_is_type_zephyr(m)
      && owl_message_is_direction_in(m)) {
    /* Handle zephyr-specific fields... */
    AV *av_zfields;

    av_zfields = newAV();
    j=owl_zephyr_get_num_fields(owl_message_get_notice(m));
    for (i=0; i<j; i++) {
      ptr=owl_zephyr_get_field_as_utf8(owl_message_get_notice(m), i+1);
      av_push(av_zfields, newSVpvn(ptr, strlen(ptr)));
      owl_free(ptr);
    }
    (void)hv_store(h, "fields", strlen("fields"), newRV_noinc((SV*)av_zfields), 0);

    (void)hv_store(h, "auth", strlen("auth"), 
                   newSVpv(owl_zephyr_get_authstr(owl_message_get_notice(m)),0),0);
  }

  j=owl_list_get_size(&(m->attributes));
  for(i=0; i<j; i++) {
    pair=owl_list_get_element(&(m->attributes), i);
    (void)hv_store(h, owl_pair_get_key(pair), strlen(owl_pair_get_key(pair)),
                   newSVpv(owl_pair_get_value(pair),0),0);
  }
  
  MSG2H(h, type);
  MSG2H(h, direction);
  MSG2H(h, class);
  MSG2H(h, instance);
  MSG2H(h, sender);
  MSG2H(h, realm);
  MSG2H(h, recipient);
  MSG2H(h, opcode);
  MSG2H(h, hostname);
  MSG2H(h, body);
  MSG2H(h, login);
  MSG2H(h, zsig);
  MSG2H(h, zwriteline);
  if (owl_message_get_header(m)) {
    MSG2H(h, header); 
  }
  (void)hv_store(h, "time", strlen("time"), newSVpv(owl_message_get_timestr(m),0),0);
  (void)hv_store(h, "id", strlen("id"), newSViv(owl_message_get_id(m)),0);
  (void)hv_store(h, "deleted", strlen("deleted"), newSViv(owl_message_is_delete(m)),0);
  (void)hv_store(h, "private", strlen("private"), newSViv(owl_message_is_private(m)),0);
  (void)hv_store(h, "should_wordwrap",
                 strlen("should_wordwrap"), newSViv(
                                                    owl_filter_message_match(wrap, m)),0);

  type = owl_message_get_type(m);
  if(!type || !*type) type = "generic";
  type = owl_strdup(type);
  type[0] = toupper(type[0]);
  blessas = owl_sprintf("BarnOwl::Message::%s", type);

  hr = newRV_noinc((SV*)h);
  stash =  gv_stashpv(blessas,0);
  if(!stash) {
    owl_function_error("No such class: %s for message type %s", blessas, owl_message_get_type(m));
    stash = gv_stashpv("BarnOwl::Message", 1);
  }
  hr = sv_bless(hr,stash);
  owl_free(type);
  owl_free(blessas);
  return hr;
}

SV *owl_perlconfig_curmessage2hashref(void) /*noproto*/
{
  int curmsg;
  owl_view *v;
  v=owl_global_get_current_view(&g);
  if (owl_view_get_size(v) < 1) {
    return &PL_sv_undef;
  }
  curmsg=owl_global_get_curmsg(&g);
  return owl_perlconfig_message2hashref(owl_view_get_element(v, curmsg));
}

/* XXX TODO: Messages should round-trip properly between
   message2hashref and hashref2message. Currently we lose
   zephyr-specific properties stored in the ZNotice_t
 */
owl_message * owl_perlconfig_hashref2message(SV *msg)
{
  owl_message * m;
  HE * ent;
  I32 count, len;
  char *key,*val;
  HV * hash;
  struct tm tm;

  hash = (HV*)SvRV(msg);

  m = owl_malloc(sizeof(owl_message));
  owl_message_init(m);

  count = hv_iterinit(hash);
  while((ent = hv_iternext(hash))) {
    key = hv_iterkey(ent, &len);
    val = SvPV_nolen(hv_iterval(hash, ent));
    if(!strcmp(key, "type")) {
      owl_message_set_type(m, val);
    } else if(!strcmp(key, "direction")) {
      owl_message_set_direction(m, owl_message_parse_direction(val));
    } else if(!strcmp(key, "private")) {
      SV * v = hv_iterval(hash, ent);
      if(SvTRUE(v)) {
        owl_message_set_isprivate(m);
      }
    } else if (!strcmp(key, "hostname")) {
      owl_message_set_hostname(m, val);
    } else if (!strcmp(key, "zwriteline")) {
      owl_message_set_zwriteline(m, val);
    } else if (!strcmp(key, "time")) {
      m->timestr = owl_strdup(val);
      strptime(val, "%a %b %d %T %Y", &tm);
      m->time = mktime(&tm);
    } else {
      owl_message_set_attribute(m, key, val);
    }
  }
  if(owl_message_is_type_admin(m)) {
    if(!owl_message_get_attribute_value(m, "adminheader"))
      owl_message_set_attribute(m, "adminheader", "");
  }
  return m;
}

/* Calls in a scalar context, passing it a hash reference.
   If return value is non-null, caller must free. */
char *owl_perlconfig_call_with_message(char *subname, owl_message *m)
{
  dSP ;
  int count;
  SV *msgref, *srv;
  char *out;
  
  ENTER ;
  SAVETMPS;
  
  PUSHMARK(SP) ;
  msgref = owl_perlconfig_message2hashref(m);
  XPUSHs(sv_2mortal(msgref));
  PUTBACK ;
  
  count = call_pv(subname, G_SCALAR|G_EVAL|G_KEEPERR);
  
  SPAGAIN ;

  if (SvTRUE(ERRSV)) {
    owl_function_error("Perl Error: '%s'", SvPV_nolen(ERRSV));
    /* and clear the error */
    sv_setsv (ERRSV, &PL_sv_undef);
  }

  if (count != 1) {
    fprintf(stderr, "bad perl!  no biscuit!  returned wrong count!\n");
    abort();
  }

  srv = POPs;

  if (srv) {
    out = owl_strdup(SvPV_nolen(srv));
  } else {
    out = NULL;
  }
  
  PUTBACK ;
  FREETMPS ;
  LEAVE ;

  return out;
}


/* Calls a method on a perl object representing a message.
   If the return value is non-null, the caller must free it.
 */
char * owl_perlconfig_message_call_method(owl_message *m, char *method, int argc, char ** argv)
{
  dSP;
  unsigned int count, i;
  SV *msgref, *srv;
  char *out;

  msgref = owl_perlconfig_message2hashref(m);

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(sv_2mortal(msgref));
  for(i=0;i<argc;i++) {
    XPUSHs(sv_2mortal(newSVpv(argv[i], 0)));
  }
  PUTBACK;

  count = call_method(method, G_SCALAR|G_KEEPERR|G_EVAL);

  SPAGAIN;

  if(count != 1) {
    fprintf(stderr, "perl returned wrong count %d\n", count);
    abort();
  }

  if (SvTRUE(ERRSV)) {
    owl_function_error("Error: '%s'", SvPV_nolen(ERRSV));
    /* and clear the error */
    sv_setsv (ERRSV, &PL_sv_undef);
  }

  srv = POPs;

  if (srv) {
    out = owl_strdup(SvPV_nolen(srv));
  } else {
    out = NULL;
  }

  PUTBACK;
  FREETMPS;
  LEAVE;

  return out;
}


char *owl_perlconfig_initperl(char * file, int *Pargc, char ***Pargv, char *** Penv)
{
  int ret;
  PerlInterpreter *p;
  char *err;
  char *args[4] = {"", "-e", "0;", NULL};

  /* create and initialize interpreter */
  PERL_SYS_INIT3(Pargc, Pargv, Penv);
  p=perl_alloc();
  owl_global_set_perlinterp(&g, (void*)p);
  perl_construct(p);

  owl_global_set_no_have_config(&g);

  ret=perl_parse(p, owl_perl_xs_init, 2, args, NULL);
  if (ret || SvTRUE(ERRSV)) {
    err=owl_strdup(SvPV_nolen(ERRSV));
    sv_setsv(ERRSV, &PL_sv_undef);     /* and clear the error */
    return(err);
  }

  ret=perl_run(p);
  if (ret || SvTRUE(ERRSV)) {
    err=owl_strdup(SvPV_nolen(ERRSV));
    sv_setsv(ERRSV, &PL_sv_undef);     /* and clear the error */
    return(err);
  }

  owl_global_set_have_config(&g);

  /* create legacy variables */
  get_sv("BarnOwl::id", TRUE);
  get_sv("BarnOwl::class", TRUE);
  get_sv("BarnOwl::instance", TRUE);
  get_sv("BarnOwl::recipient", TRUE);
  get_sv("BarnOwl::sender", TRUE);
  get_sv("BarnOwl::realm", TRUE);
  get_sv("BarnOwl::opcode", TRUE);
  get_sv("BarnOwl::zsig", TRUE);
  get_sv("BarnOwl::msg", TRUE);
  get_sv("BarnOwl::time", TRUE);
  get_sv("BarnOwl::host", TRUE);
  get_av("BarnOwl::fields", TRUE);

  if(file) {
    SV * cfg = get_sv("BarnOwl::configfile", TRUE);
    sv_setpv(cfg, file);
  }

  eval_pv(owl_perlwrap_codebuff, FALSE);

  if (SvTRUE(ERRSV)) {
    err=owl_strdup(SvPV_nolen(ERRSV));
    sv_setsv (ERRSV, &PL_sv_undef);     /* and clear the error */
    return(err);
  }

  /* check if we have the formatting function */
  if (owl_perlconfig_is_function("BarnOwl::format_msg")) {
    owl_global_set_config_format(&g, 1);
  }

  return(NULL);
}

/* returns whether or not a function exists */
int owl_perlconfig_is_function(char *fn) {
  if (get_cv(fn, FALSE)) return(1);
  else return(0);
}

/* caller is responsible for freeing returned string */
char *owl_perlconfig_execute(char *line)
{
  STRLEN len;
  SV *response;
  char *out, *preout;

  if (!owl_global_have_config(&g)) return NULL;

  ENTER;
  SAVETMPS;
  /* execute the subroutine */
  response = eval_pv(line, FALSE);

  if (SvTRUE(ERRSV)) {
    owl_function_error("Perl Error: '%s'", SvPV_nolen(ERRSV));
    sv_setsv (ERRSV, &PL_sv_undef);     /* and clear the error */
  }

  preout=SvPV(response, len);
  if (len == 0 || preout[len - 1] != '\n')
    out = owl_sprintf("%s\n", preout);
  else
    out = owl_strdup(preout);
  FREETMPS;
  LEAVE;

  return(out);
}

void owl_perlconfig_getmsg(owl_message *m, char *subname)
{
  char *ptr = NULL;
  if (owl_perlconfig_is_function("BarnOwl::Hooks::_receive_msg")) {
    ptr = owl_perlconfig_call_with_message(subname?subname
                                           :"BarnOwl::_receive_msg_legacy_wrap", m);
  }
  if (ptr) owl_free(ptr);
}

/* Called on all new messages; receivemsg is only called on incoming ones */
void owl_perlconfig_newmsg(owl_message *m, char *subname)
{
  char *ptr = NULL;
  if (owl_perlconfig_is_function("BarnOwl::Hooks::_new_msg")) {
    ptr = owl_perlconfig_call_with_message(subname?subname
                                           :"BarnOwl::Hooks::_new_msg", m);
  }
  if (ptr) owl_free(ptr);
}

char *owl_perlconfig_perlcmd(owl_cmd *cmd, int argc, char **argv)
{
  int i, count;
  char * ret = NULL;
  SV *rv;
  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  for(i=0;i<argc;i++) {
    SV *tmp = newSVpv(argv[i], 0);
    SvUTF8_on(tmp);
    XPUSHs(sv_2mortal(tmp));
  }
  PUTBACK;

  count = call_sv(cmd->cmd_perl, G_SCALAR|G_EVAL);

  SPAGAIN;

  if(SvTRUE(ERRSV)) {
    owl_function_error("%s", SvPV_nolen(ERRSV));
    (void)POPs;
  } else {
    if(count != 1)
      croak("Perl command %s returned more than one value!", cmd->name);
    rv = POPs;
    if(SvTRUE(rv)) {
      ret = owl_strdup(SvPV_nolen(rv));
    }
  }

  FREETMPS;
  LEAVE;

  return ret;
}

void owl_perlconfig_cmd_free(owl_cmd *cmd)
{
  SvREFCNT_dec(cmd->cmd_perl);
}

void owl_perlconfig_dispatch_free(owl_dispatch *d)
{
  SvREFCNT_dec(d->data);
  owl_free(d);
}

void owl_perlconfig_edit_callback(owl_editwin *e)
{
  SV *cb = (SV*)(e->cbdata);
  SV *text;
  dSP;

  if(cb == NULL) {
    owl_function_error("Perl callback is NULL!");
  }
  text = newSVpv(owl_editwin_get_text(e), 0);
  SvUTF8_on(text);

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(sv_2mortal(text));
  PUTBACK;
  
  call_sv(cb, G_DISCARD|G_KEEPERR|G_EVAL);

  if(SvTRUE(ERRSV)) {
    owl_function_error("%s", SvPV_nolen(ERRSV));
  }

  FREETMPS;
  LEAVE;

  SvREFCNT_dec(cb);
  e->cbdata = NULL;
}

void owl_perlconfig_mainloop()
{
  dSP;
  if (!owl_perlconfig_is_function("BarnOwl::Hooks::_mainloop_hook"))
    return;
  PUSHMARK(SP) ;
  call_pv("BarnOwl::Hooks::_mainloop_hook", G_DISCARD|G_EVAL);
  if(SvTRUE(ERRSV)) {
    owl_function_error("%s", SvPV_nolen(ERRSV));
  }
  return;
}

void owl_perlconfig_dispatch(owl_dispatch *d)
{
  SV *cb = d->data;
  dSP;
  if(cb == NULL) {
    owl_function_error("Perl callback is NULL!");
    return;
  }

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  PUTBACK;
  
  call_sv(cb, G_DISCARD|G_KEEPERR|G_EVAL);

  if(SvTRUE(ERRSV)) {
    owl_function_error("%s", SvPV_nolen(ERRSV));
  }

  FREETMPS;
  LEAVE;
}

void owl_perlconfig_perl_timer(owl_timer *t, void *data)
{
  SV *obj = data;

  if(!SvROK(obj)) {
    return;
  }

  dSP;
  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(obj);
  PUTBACK;

  call_method("do_callback", G_DISCARD|G_KEEPERR|G_EVAL);

  SPAGAIN;

  if (SvTRUE(ERRSV)) {
    owl_function_error("Error in calback: '%s'", SvPV_nolen(ERRSV));
    sv_setsv (ERRSV, &PL_sv_undef);
  }

  PUTBACK;
  FREETMPS;
  LEAVE;
}

void owl_perlconfig_perl_timer_destroy(owl_timer *t)
{
  if(SvOK((SV*)t->data)) {
    SvREFCNT_dec((SV*)t->data);
  }
}
