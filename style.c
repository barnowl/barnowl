#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_style_create_internal(owl_style *s, char *name, void (*formatfunc) (owl_fmtext *fm, owl_message *m))
{
  s->type=OWL_STYLE_TYPE_INTERNAL;
  s->name=owl_strdup(name);
  s->perlfuncname=NULL;
  s->formatfunc=formatfunc;
}

void owl_style_create_perl(owl_style *s, char *name, char *perlfuncname)
{
  s->type=OWL_STYLE_TYPE_PERL;
  s->name=owl_strdup(name);
  s->perlfuncname=owl_strdup(perlfuncname);
  s->formatfunc=NULL;
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

/* Use style 's' to format message 'm' into fmtext 'fm'.
 * 'fm' should already be be initialzed
 */
void owl_style_get_formattext(owl_style *s, owl_fmtext *fm, owl_message *m)
{
  if (s->type==OWL_STYLE_TYPE_INTERNAL) {
    (* s->formatfunc)(fm, m);
  } else if (s->type==OWL_STYLE_TYPE_PERL) {
    char *body, *indent;

    /* run the perl function */
    body=owl_config_getmsg(m, s->perlfuncname);
    
    /* indent */
    indent=owl_malloc(strlen(body)+owl_text_num_lines(body)*OWL_TAB+10);
    owl_text_indent(indent, body, OWL_TAB);
    
    /* fmtext_append.  This needs to change */
    owl_fmtext_append_ztext(fm, indent);
    
    owl_free(indent);
    owl_free(body);
  }
}

void owl_style_free(owl_style *s)
{
  if (s->name) owl_free(s->name);
  if (s->type==OWL_STYLE_TYPE_PERL && s->perlfuncname) {
    owl_free(s->perlfuncname);
  }
}
