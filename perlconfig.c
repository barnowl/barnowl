#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#define OWL_PERL
#include "owl.h"

extern XS(boot_BarnOwl);
extern XS(boot_DynaLoader);
/* extern XS(boot_DBI); */

static void owl_perl_xs_init(pTHX)
{
  const char *file = __FILE__;
  dXSUB_SYS;
  {
    newXS("BarnOwl::bootstrap", boot_BarnOwl, file);
    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
  }
}


SV *owl_new_sv(const char * str)
{
  SV *ret = newSVpv(str, 0);
  if (is_utf8_string((const U8 *)str, strlen(str))) {
    SvUTF8_on(ret);
  } else {
    char *escape = owl_escape_highbit(str);
    owl_function_error("Internal error! Non-UTF-8 string encountered:\n%s", escape);
    owl_free(escape);
  }
  return ret;
}

AV *owl_new_av(const owl_list *l, SV *(*to_sv)(const void *))
{
  AV *ret;
  int i;
  void *element;

  ret = newAV();

  for (i = 0; i < owl_list_get_size(l); i++) {
    element = owl_list_get_element(l, i);
    av_push(ret, to_sv(element));
  }

  return ret;
}

HV *owl_new_hv(const owl_dict *d, SV *(*to_sv)(const void *))
{
  HV *ret;
  owl_list l;
  const char *key;
  void *element;
  int i;

  ret = newHV();

  /* TODO: add an iterator-like interface to owl_dict */
  owl_dict_get_keys(d, &l);
  for (i = 0; i < owl_list_get_size(&l); i++) {
    key = owl_list_get_element(&l, i);
    element = owl_dict_find_element(d, key);
    (void)hv_store(ret, key, strlen(key), to_sv(element), 0);
  }
  owl_list_cleanup(&l, owl_free);

  return ret;
}

SV *owl_perlconfig_message2hashref(const owl_message *m)
{
  HV *h, *stash;
  SV *hr;
  const char *type;
  char *ptr, *utype, *blessas;
  int i, j;
  const owl_pair *pair;
  const owl_filter *wrap;

  if (!m) return &PL_sv_undef;
  wrap = owl_global_get_filter(&g, "wordwrap");
  if(!wrap) {
      owl_function_error("wrap filter is not defined");
      return &PL_sv_undef;
  }

  h = newHV();

#define MSG2H(h,field) (void)hv_store(h, #field, strlen(#field),        \
                                      owl_new_sv(owl_message_get_##field(m)), 0)

  if (owl_message_is_type_zephyr(m)
      && owl_message_is_direction_in(m)) {
    /* Handle zephyr-specific fields... */
    AV *av_zfields;

    av_zfields = newAV();
    j=owl_zephyr_get_num_fields(owl_message_get_notice(m));
    for (i=0; i<j; i++) {
      ptr=owl_zephyr_get_field_as_utf8(owl_message_get_notice(m), i+1);
      av_push(av_zfields, owl_new_sv(ptr));
      owl_free(ptr);
    }
    (void)hv_store(h, "fields", strlen("fields"), newRV_noinc((SV*)av_zfields), 0);

    (void)hv_store(h, "auth", strlen("auth"), 
                   owl_new_sv(owl_zephyr_get_authstr(owl_message_get_notice(m))),0);
  }

  j=owl_list_get_size(&(m->attributes));
  for(i=0; i<j; i++) {
    pair=owl_list_get_element(&(m->attributes), i);
    (void)hv_store(h, owl_pair_get_key(pair), strlen(owl_pair_get_key(pair)),
                   owl_new_sv(owl_pair_get_value(pair)),0);
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
  (void)hv_store(h, "time", strlen("time"), owl_new_sv(owl_message_get_timestr(m)),0);
  (void)hv_store(h, "unix_time", strlen("unix_time"), newSViv(m->time), 0);
  (void)hv_store(h, "id", strlen("id"), newSViv(owl_message_get_id(m)),0);
  (void)hv_store(h, "deleted", strlen("deleted"), newSViv(owl_message_is_delete(m)),0);
  (void)hv_store(h, "private", strlen("private"), newSViv(owl_message_is_private(m)),0);
  (void)hv_store(h, "should_wordwrap",
                 strlen("should_wordwrap"), newSViv(
                                                    owl_filter_message_match(wrap, m)),0);

  type = owl_message_get_type(m);
  if(!type || !*type) type = "generic";
  utype = owl_strdup(type);
  utype[0] = toupper(type[0]);
  blessas = owl_sprintf("BarnOwl::Message::%s", utype);

  hr = newRV_noinc((SV*)h);
  stash =  gv_stashpv(blessas,0);
  if(!stash) {
    owl_function_error("No such class: %s for message type %s", blessas, owl_message_get_type(m));
    stash = gv_stashpv("BarnOwl::Message", 1);
  }
  hr = sv_bless(hr,stash);
  owl_free(utype);
  owl_free(blessas);
  return hr;
}

SV *owl_perlconfig_curmessage2hashref(void)
{
  int curmsg;
  const owl_view *v;
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

   This has been somewhat addressed, but is still not lossless.
 */
owl_message * owl_perlconfig_hashref2message(SV *msg)
{
  owl_message * m;
  HE * ent;
  I32 count, len;
  const char *key,*val;
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
#ifdef HAVE_LIBZEPHYR
  if (owl_message_is_type_zephyr(m)) {
    ZNotice_t *n = &(m->notice);
    n->z_kind = ACKED;
    n->z_port = 0;
    n->z_auth = ZAUTH_NO;
    n->z_checked_auth = 0;
    n->z_class = zstr(owl_message_get_class(m));
    n->z_class_inst = zstr(owl_message_get_instance(m));
    n->z_opcode = zstr(owl_message_get_opcode(m));
    n->z_sender = zstr(owl_message_get_sender(m));
    n->z_recipient = zstr(owl_message_get_recipient(m));
    n->z_default_format = zstr("[zephyr created from perl]");
    n->z_multinotice = zstr("[zephyr created from perl]");
    n->z_num_other_fields = 0;
    n->z_message = owl_sprintf("%s%c%s", owl_message_get_zsig(m), '\0', owl_message_get_body(m));
    n->z_message_len = strlen(owl_message_get_zsig(m)) + strlen(owl_message_get_body(m)) + 1;
  }
#endif
  return m;
}

/* Calls in a scalar context, passing it a hash reference.
   If return value is non-null, caller must free. */
char *owl_perlconfig_call_with_message(const char *subname, const owl_message *m)
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
  
  count = call_pv(subname, G_SCALAR|G_EVAL);
  
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
char * owl_perlconfig_message_call_method(const owl_message *m, const char *method, int argc, const char ** argv)
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
    XPUSHs(sv_2mortal(owl_new_sv(argv[i])));
  }
  PUTBACK;

  count = call_method(method, G_SCALAR|G_EVAL);

  SPAGAIN;

  if(count != 1) {
    fprintf(stderr, "perl returned wrong count %u\n", count);
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


char *owl_perlconfig_initperl(const char * file, int *Pargc, char ***Pargv, char *** Penv)
{
  int ret;
  PerlInterpreter *p;
  char *err;
  const char *args[4] = {"", "-e", "0;", NULL};
  AV *inc;
  char *path;

  /* create and initialize interpreter */
  PERL_SYS_INIT3(Pargc, Pargv, Penv);
  p=perl_alloc();
  owl_global_set_perlinterp(&g, p);
  perl_construct(p);

  owl_global_set_no_have_config(&g);

  ret=perl_parse(p, owl_perl_xs_init, 2, (char **)args, NULL);
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

  /* Add the system lib path to @INC */
  inc = get_av("INC", 0);
  path = owl_sprintf("%s/lib", owl_get_datadir());
  av_unshift(inc, 1);
  av_store(inc, 0, owl_new_sv(path));
  owl_free(path);

  eval_pv("use BarnOwl;", FALSE);

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
int owl_perlconfig_is_function(const char *fn) {
  if (get_cv(fn, FALSE)) return(1);
  else return(0);
}

/* caller is responsible for freeing returned string */
char *owl_perlconfig_execute(const char *line)
{
  STRLEN len;
  SV *response;
  char *out;

  if (!owl_global_have_config(&g)) return NULL;

  ENTER;
  SAVETMPS;
  /* execute the subroutine */
  response = eval_pv(line, FALSE);

  if (SvTRUE(ERRSV)) {
    owl_function_error("Perl Error: '%s'", SvPV_nolen(ERRSV));
    sv_setsv (ERRSV, &PL_sv_undef);     /* and clear the error */
  }

  out = owl_strdup(SvPV(response, len));
  FREETMPS;
  LEAVE;

  return(out);
}

void owl_perlconfig_getmsg(const owl_message *m, const char *subname)
{
  char *ptr = NULL;
  if (owl_perlconfig_is_function("BarnOwl::Hooks::_receive_msg")) {
    ptr = owl_perlconfig_call_with_message(subname?subname
                                           :"BarnOwl::_receive_msg_legacy_wrap", m);
  }
  if (ptr) owl_free(ptr);
}

/* Called on all new messages; receivemsg is only called on incoming ones */
void owl_perlconfig_newmsg(const owl_message *m, const char *subname)
{
  char *ptr = NULL;
  if (owl_perlconfig_is_function("BarnOwl::Hooks::_new_msg")) {
    ptr = owl_perlconfig_call_with_message(subname?subname
                                           :"BarnOwl::Hooks::_new_msg", m);
  }
  if (ptr) owl_free(ptr);
}

void owl_perlconfig_new_command(const char *name)
{
  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(sv_2mortal(owl_new_sv(name)));
  PUTBACK;

  call_pv("BarnOwl::Hooks::_new_command", G_SCALAR|G_VOID);

  SPAGAIN;

  if(SvTRUE(ERRSV)) {
    owl_function_error("%s", SvPV_nolen(ERRSV));
  }

  FREETMPS;
  LEAVE;
}

char *owl_perlconfig_perlcmd(const owl_cmd *cmd, int argc, const char *const *argv)
{
  int i, count;
  char * ret = NULL;
  SV *rv;
  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  for(i=0;i<argc;i++) {
    XPUSHs(sv_2mortal(owl_new_sv(argv[i])));
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

void owl_perlconfig_cmd_cleanup(owl_cmd *cmd)
{
  SvREFCNT_dec(cmd->cmd_perl);
}

void owl_perlconfig_io_dispatch_destroy(const owl_io_dispatch *d)
{
  SvREFCNT_dec(d->data);
}

void owl_perlconfig_edit_callback(owl_editwin *e)
{
  SV *cb = owl_editwin_get_cbdata(e);
  SV *text;
  dSP;

  if(cb == NULL) {
    owl_function_error("Perl callback is NULL!");
    return;
  }
  text = owl_new_sv(owl_editwin_get_text(e));

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(sv_2mortal(text));
  PUTBACK;
  
  call_sv(cb, G_DISCARD|G_EVAL);

  if(SvTRUE(ERRSV)) {
    owl_function_error("%s", SvPV_nolen(ERRSV));
  }

  FREETMPS;
  LEAVE;
}

void owl_perlconfig_dec_refcnt(void *data)
{
  SV *v = data;
  SvREFCNT_dec(v);
}

void owl_perlconfig_io_dispatch(const owl_io_dispatch *d, void *data)
{
  SV *cb = data;
  dSP;
  if(cb == NULL) {
    owl_function_error("Perl callback is NULL!");
    return;
  }

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  PUTBACK;

  call_sv(cb, G_DISCARD|G_EVAL);

  if(SvTRUE(ERRSV)) {
    owl_function_error("%s", SvPV_nolen(ERRSV));
  }

  FREETMPS;
  LEAVE;
}

void owl_perlconfig_perl_timer(owl_timer *t, void *data)
{
  dSP;
  SV *obj = data;

  if(!SvROK(obj)) {
    return;
  }

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(obj);
  PUTBACK;

  call_method("do_callback", G_DISCARD|G_EVAL);

  SPAGAIN;

  if (SvTRUE(ERRSV)) {
    owl_function_error("Error in callback: '%s'", SvPV_nolen(ERRSV));
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
