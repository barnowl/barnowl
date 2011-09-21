#define OWL_PERL
#include "owl.h"
#include <stdio.h>

extern XS(boot_BarnOwl);
extern XS(boot_DynaLoader);
/* extern XS(boot_DBI); */

void owl_perl_xs_init(pTHX) /* noproto */
{
  const char *file = __FILE__;
  dXSUB_SYS;
  {
    newXS("BarnOwl::bootstrap", boot_BarnOwl, file);
    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
  }
}


CALLER_OWN SV *owl_new_sv(const char * str)
{
  SV *ret = newSVpv(str, 0);
  if (is_utf8_string((const U8 *)str, strlen(str))) {
    SvUTF8_on(ret);
  } else {
    char *escape = owl_escape_highbit(str);
    owl_function_error("Internal error! Non-UTF-8 string encountered:\n%s", escape);
    g_free(escape);
  }
  return ret;
}

CALLER_OWN AV *owl_new_av(const GPtrArray *l, SV *(*to_sv)(const void *))
{
  AV *ret;
  int i;
  void *element;

  ret = newAV();

  for (i = 0; i < l->len; i++) {
    element = l->pdata[i];
    av_push(ret, to_sv(element));
  }

  return ret;
}

CALLER_OWN HV *owl_new_hv(const owl_dict *d, SV *(*to_sv)(const void *))
{
  HV *ret;
  GPtrArray *keys;
  const char *key;
  void *element;
  int i;

  ret = newHV();

  /* TODO: add an iterator-like interface to owl_dict */
  keys = owl_dict_get_keys(d);
  for (i = 0; i < keys->len; i++) {
    key = keys->pdata[i];
    element = owl_dict_find_element(d, key);
    (void)hv_store(ret, key, strlen(key), to_sv(element), 0);
  }
  owl_ptr_array_free(keys, g_free);

  return ret;
}

CALLER_OWN SV *owl_perlconfig_message2hashref(const owl_message *m)
{
  HV *h, *stash;
  SV *hr;
  const char *type;
  char *ptr, *utype, *blessas;
  const char *f;
  int i;
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

  if (owl_message_is_type_zephyr(m) && owl_message_is_direction_in(m)) {
    /* Handle zephyr-specific fields... */
    AV *av_zfields = newAV();
    if (owl_message_get_notice(m)) {
      for (f = owl_zephyr_first_raw_field(owl_message_get_notice(m)); f != NULL;
           f = owl_zephyr_next_raw_field(owl_message_get_notice(m), f)) {
        ptr = owl_zephyr_field_as_utf8(owl_message_get_notice(m), f);
        av_push(av_zfields, owl_new_sv(ptr));
        g_free(ptr);
      }
      (void)hv_store(h, "auth", strlen("auth"),
                     owl_new_sv(owl_zephyr_get_authstr(owl_message_get_notice(m))), 0);
    } else {
      /* Incoming zephyrs without a ZNotice_t are pseudo-logins. To appease
       * existing styles, put in bogus 'auth' and 'fields' keys. */
      (void)hv_store(h, "auth", strlen("auth"), owl_new_sv("NO"), 0);
    }
    (void)hv_store(h, "fields", strlen("fields"), newRV_noinc((SV*)av_zfields), 0);
  }

  for (i = 0; i < m->attributes->len; i++) {
    pair = m->attributes->pdata[i];
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
  utype = g_strdup(type);
  utype[0] = toupper(type[0]);
  blessas = g_strdup_printf("BarnOwl::Message::%s", utype);

  hr = newRV_noinc((SV*)h);
  stash =  gv_stashpv(blessas,0);
  if(!stash) {
    owl_function_error("No such class: %s for message type %s", blessas, owl_message_get_type(m));
    stash = gv_stashpv("BarnOwl::Message", 1);
  }
  hr = sv_bless(hr,stash);
  g_free(utype);
  g_free(blessas);
  return hr;
}

CALLER_OWN SV *owl_perlconfig_curmessage2hashref(void)
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
 */
CALLER_OWN owl_message *owl_perlconfig_hashref2message(SV *msg)
{
  owl_message * m;
  HE * ent;
  I32 len;
  const char *key,*val;
  HV * hash;
  struct tm tm;

  hash = (HV*)SvRV(msg);

  m = g_new(owl_message, 1);
  owl_message_init(m);

  hv_iterinit(hash);
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
      g_free(m->timestr);
      m->timestr = g_strdup(val);
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
CALLER_OWN char *owl_perlconfig_call_with_message(const char *subname, const owl_message *m)
{
  SV *msgref, *rv;
  char *out = NULL;

  msgref = owl_perlconfig_message2hashref(m);

  OWL_PERL_CALL((call_pv(subname, G_SCALAR|G_EVAL))
                ,
                XPUSHs(sv_2mortal(msgref));
                ,
                "Perl Error: '%s'"
                ,
                false
                ,
                false
                ,
                rv = POPs;
                if (rv && SvPOK(rv))
                  out = g_strdup(SvPV_nolen(rv));
                );
  return out;
}


/* Calls a method on a perl object representing a message.
   If the return value is non-null, the caller must free it.
 */
CALLER_OWN char *owl_perlconfig_message_call_method(const owl_message *m, const char *method, int argc, const char **argv)
{
  SV *msgref, *rv;
  char *out = NULL;
  int i;

  msgref = owl_perlconfig_message2hashref(m);

  OWL_PERL_CALL(call_method(method, G_SCALAR|G_EVAL)
                ,
                XPUSHs(sv_2mortal(msgref));
                OWL_PERL_PUSH_ARGS(i, argc, argv);
                ,
                "Perl Error: '%s'"
                ,
                false
                ,
                false
                ,
                rv = POPs;
                if (rv && SvPOK(rv))
                  out = g_strdup(SvPV_nolen(rv));
                );
  return out;
}

/* caller must free result, if not NULL */
CALLER_OWN char *owl_perlconfig_initperl(const char *file, int *Pargc, char ***Pargv, char ***Penv)
{
  int ret;
  PerlInterpreter *p;
  char *err;
  const char *args[4] = {"", "-e", "0;", NULL};
  const char *dlerr;
  AV *inc;
  char *path;

  /* create and initialize interpreter */
  PERL_SYS_INIT3(Pargc, Pargv, Penv);
  p=perl_alloc();
  owl_global_set_perlinterp(&g, p);
  perl_construct(p);

  PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

  owl_global_set_no_have_config(&g);

  ret=perl_parse(p, owl_perl_xs_init, 2, (char **)args, NULL);
  if (ret || SvTRUE(ERRSV)) {
    err=g_strdup(SvPV_nolen(ERRSV));
    sv_setsv(ERRSV, &PL_sv_undef);     /* and clear the error */
    return(err);
  }

  ret=perl_run(p);
  if (ret || SvTRUE(ERRSV)) {
    err=g_strdup(SvPV_nolen(ERRSV));
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

  sv_setpv(get_sv("BarnOwl::VERSION", TRUE), OWL_VERSION_STRING);

  /* Add the system lib path to @INC */
  inc = get_av("INC", 0);
  path = g_build_filename(owl_get_datadir(), "lib", NULL);
  av_unshift(inc, 1);
  av_store(inc, 0, owl_new_sv(path));
  g_free(path);

  /* Load up perl-Glib. */
  eval_pv("use Glib;", FALSE);

  /* Now, before BarnOwl tries to use them, get the relevant function pointers out. */
  dlerr = owl_closure_init();
  if (dlerr) {
    return g_strdup(dlerr);
  }

  /* And now it's safe to import BarnOwl. */
  eval_pv("use BarnOwl;", FALSE);

  if (SvTRUE(ERRSV)) {
    err=g_strdup(SvPV_nolen(ERRSV));
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
CALLER_OWN char *owl_perlconfig_execute(const char *line)
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

  out = g_strdup(SvPV(response, len));
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
  g_free(ptr);
}

/* Called on all new messages; receivemsg is only called on incoming ones */
void owl_perlconfig_newmsg(const owl_message *m, const char *subname)
{
  char *ptr = NULL;
  if (owl_perlconfig_is_function("BarnOwl::Hooks::_new_msg")) {
    ptr = owl_perlconfig_call_with_message(subname?subname
                                           :"BarnOwl::Hooks::_new_msg", m);
  }
  g_free(ptr);
}

void owl_perlconfig_new_command(const char *name)
{
  OWL_PERL_CALL(call_pv("BarnOwl::Hooks::_new_command", G_VOID|G_EVAL);
                ,
                XPUSHs(sv_2mortal(owl_new_sv(name)));
                ,
                "Perl Error: '%s'"
                ,
                false
                ,
                true
                ,
                );
}

/* caller must free the result */
CALLER_OWN char *owl_perlconfig_perlcmd(const owl_cmd *cmd, int argc, const char *const *argv)
{
  int i;
  SV* rv;
  char *out = NULL;

  OWL_PERL_CALL(call_sv(cmd->cmd_perl, G_SCALAR|G_EVAL)
                ,
                OWL_PERL_PUSH_ARGS(i, argc, argv);
                ,
                "Perl Error: '%s'"
                ,
                false
                ,
                false
                ,
                rv = POPs;
                if (rv && SvPOK(rv))
                  out = g_strdup(SvPV_nolen(rv));
                );
  return out;
}

void owl_perlconfig_cmd_cleanup(owl_cmd *cmd)
{
  SvREFCNT_dec(cmd->cmd_perl);
}

void owl_perlconfig_edit_callback(owl_editwin *e, bool success)
{
  SV *cb = owl_editwin_get_cbdata(e);
  SV *text = owl_new_sv(owl_editwin_get_text(e));

  if (cb == NULL) {
    owl_function_error("Perl callback is NULL!");
    return;
  }

  OWL_PERL_CALL(call_sv(cb, G_DISCARD|G_EVAL)
                ,
                XPUSHs(sv_2mortal(text));
                XPUSHs(sv_2mortal(newSViv(success)));
                ,
                "Perl Error: '%s'"
                ,
                false
                ,
                true
                ,
                );
}

void owl_perlconfig_dec_refcnt(void *data)
{
  SV *v = data;
  SvREFCNT_dec(v);
}
