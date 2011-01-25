#define OWL_PERL
#include "owl.h"

/* Assumes owenership of one existing ref on `obj`*/
void owl_style_create_perl(owl_style *s, const char *name, SV *obj)
{
  s->name=owl_strdup(name);
  s->perlobj = obj;
}

int owl_style_matches_name(const owl_style *s, const char *name)
{
  if (!strcmp(s->name, name)) return(1);
  return(0);
}

const char *owl_style_get_name(const owl_style *s)
{
  return(s->name);
}

const char *owl_style_get_description(const owl_style *s)
{
  SV *sv = NULL;
  OWL_PERL_CALL_METHOD(s->perlobj,
                       "description",
                       ;,
                       "Error in style_get_description: %s",
                       0,
                       sv = SvREFCNT_inc(POPs);
                       );
  if(sv) {
    return SvPV_nolen(sv_2mortal(sv));
  } else {
    return "[error getting description]";
  }
}

/* Use style 's' to format message 'm' into fmtext 'fm'.
 * 'fm' should already be be initialzed
 */
void owl_style_get_formattext(const owl_style *s, owl_fmtext *fm, const owl_message *m)
{
  const char *body;
  char *indent;
  int curlen;
  owl_fmtext with_tabs;

  SV *sv = NULL;
  
  /* Call the perl object */
  OWL_PERL_CALL_METHOD(s->perlobj,
                       "format_message",
                       XPUSHs(sv_2mortal(owl_perlconfig_message2hashref(m)));,
                       "Error in format_message: %s",
                       0,
                       sv = SvREFCNT_inc(POPs);
                       );

  if(sv) {
    body = SvPV_nolen(sv);
  } else {
    body = "<unformatted message>";
  }

  /* indent and ensure ends with a newline */
  indent = owl_text_indent(body, OWL_TAB);
  curlen = strlen(indent);
  if (curlen==0 || indent[curlen-1] != '\n') {
    indent[curlen] = '\n';
    indent[curlen+1] = '\0';
  }

  owl_fmtext_init_null(&with_tabs);
  /* fmtext_append.  This needs to change */
  owl_fmtext_append_ztext(&with_tabs, indent);
  /* Expand tabs, taking the indent into account. Otherwise, tabs from the
   * style display incorrectly due to our own indent. */
  owl_fmtext_expand_tabs(&with_tabs, fm, OWL_TAB_WIDTH - OWL_TAB);
  owl_fmtext_cleanup(&with_tabs);

  owl_free(indent);
  if(sv)
    SvREFCNT_dec(sv);
}

int owl_style_validate(const owl_style *s) {
  if (!s || !s->perlobj || !SvOK(s->perlobj)) {
    return -1;
  }
  return 0;
}

void owl_style_cleanup(owl_style *s)
{
  if (s->name) owl_free(s->name);
  SvREFCNT_dec(s->perlobj);
}

void owl_style_delete(owl_style *s)
{
  owl_style_cleanup(s);
  owl_free(s);
}
