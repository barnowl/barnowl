#define OWL_PERL
#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_style_create_perl(owl_style *s, char *name, SV *obj)
{
  s->name=owl_strdup(name);
  s->perlobj = SvREFCNT_inc(obj);
}

int owl_style_matches_name(owl_style *s, char *name)
{
  if (!strcmp(s->name, name)) return(1);
  return(0);
}

char *owl_style_get_name(owl_style *s)
{
  return(s->name);
}

char *owl_style_get_description(owl_style *s)
{
  SV *sv = NULL;
  OWL_PERL_CALL_METHOD(s->perlobj,
                       "description",
                       /* no args */,
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
void owl_style_get_formattext(owl_style *s, owl_fmtext *fm, owl_message *m)
{
  char *body, *indent;
  int curlen;

  SV *sv = NULL;
  
  /* Call the perl object */
  OWL_PERL_CALL_METHOD(s->perlobj,
                       "format_message",
                       XPUSHs(owl_perlconfig_message2hashref(m));,
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
  indent=owl_malloc(strlen(body)+(owl_text_num_lines(body))*OWL_TAB+10);
  owl_text_indent(indent, body, OWL_TAB);
  curlen = strlen(indent);
  if (curlen==0 || indent[curlen-1] != '\n') {
    indent[curlen] = '\n';
    indent[curlen+1] = '\0';
  }

  /* fmtext_append.  This needs to change */
  owl_fmtext_append_ztext(fm, indent);

  owl_free(indent);
  if(sv)
    SvREFCNT_dec(body);
}

int owl_style_validate(owl_style *s) {
  if (!s || !s->perlobj || !SvOK(s->perlobj)) {
    return -1;
  }
  return 0;
}

void owl_style_free(owl_style *s)
{
  if (s->name) owl_free(s->name);
  SvREFCNT_dec(s->perlobj);
}
