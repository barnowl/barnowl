#include "owl.h"

static const char fileIdent[] = "$Id$";

/* In all of these functions, 'fm' is expected to already be
 * initialized.
 */

void owl_style_basic_format_body(owl_fmtext *fm, owl_message *m) {
  char *indent, *body;
  owl_filter *f;
  int wrap = 0;

  /* get the body */
  body=owl_strdup(owl_message_get_body(m));

  f = owl_global_get_filter(&g, "wordwrap");
  if(f && owl_filter_message_match(f, m)) 
    wrap = 1;

  if(wrap) {
    int cols, i, width, word;
    char *tab, *tok, *ws = " \t\n\r";
    cols = owl_global_get_cols(&g) - OWL_MSGTAB - 1;

    tab = owl_malloc(OWL_MSGTAB+1);
    for(i = 0; i < OWL_MSGTAB; i++) {
      tab[i] = ' ';
    }
    tab[OWL_MSGTAB] = 0;

    tok = strtok(body, ws);
    tab[OWL_MSGTAB-1] = 0;
    owl_fmtext_append_normal(fm, tab);
    tab[OWL_MSGTAB-1] = ' ';
    width = 0;

    while(tok) {
      word = strlen(tok);
      if(word + width + 1 < cols) {
        owl_fmtext_append_normal(fm, " ");
        owl_fmtext_append_normal(fm, tok);
        width += word + 1;
      } else {
        owl_fmtext_append_normal(fm, "\n");
        owl_fmtext_append_normal(fm, tab);
        owl_fmtext_append_normal(fm, tok);
        width = word;
      }
      tok = strtok(NULL, ws);
    }
    owl_fmtext_append_normal(fm, "\n");

    owl_free(tab);
  } else {
    /* do the indenting into indent */
    indent=owl_malloc(strlen(body)+owl_text_num_lines(body)*OWL_MSGTAB+10);
    owl_text_indent(indent, body, OWL_MSGTAB);
    owl_fmtext_append_ztext(fm, indent);
    if(body[strlen(body)-1] != '\n')
      owl_fmtext_append_ztext(fm, "\n");
    owl_free(indent);
  }

  owl_free(body);
}
 
void owl_stylefunc_basic(owl_fmtext *fm, owl_message *m)
{
#ifdef HAVE_LIBZEPHYR
  char *ptr, *zsigbuff, frombuff[LINE];
  ZNotice_t *n;
#endif

  if (owl_message_is_type_zephyr(m) && owl_message_is_direction_in(m)) {
#ifdef HAVE_LIBZEPHYR
    n=owl_message_get_notice(m);
  
    /* edit the from addr for printing */
    strcpy(frombuff, owl_message_get_sender(m));
    ptr=strchr(frombuff, '@');
    if (ptr && !strncmp(ptr+1, owl_zephyr_get_realm(), strlen(owl_zephyr_get_realm()))) {
      *ptr='\0';
    }
    
    /* set the message for printing */
    owl_fmtext_append_normal(fm, OWL_TABSTR);
    
    if (owl_message_is_ping(m)) {
      owl_fmtext_append_bold(fm, "PING");
      owl_fmtext_append_normal(fm, " from ");
      owl_fmtext_append_bold(fm, frombuff);
      owl_fmtext_append_normal(fm, "\n");
    } else if (owl_message_is_loginout(m)) {
      char *host, *tty;

      host=owl_message_get_attribute_value(m, "loginhost");
      tty=owl_message_get_attribute_value(m, "logintty");
      
      if (owl_message_is_login(m)) {
	owl_fmtext_append_bold(fm, "LOGIN");
      } else if (owl_message_is_logout(m)) {
	owl_fmtext_append_bold(fm, "LOGOUT");
      }
      if (owl_message_is_pseudo(m)) {
	owl_fmtext_append_bold(fm, " (PSEUDO)");
      }
      owl_fmtext_append_normal(fm, " for ");
      ptr=short_zuser(owl_message_get_instance(m));
      owl_fmtext_append_bold(fm, ptr);
      owl_free(ptr);
      owl_fmtext_append_normal(fm, " at ");
      owl_fmtext_append_normal(fm, host ? host : "");
      owl_fmtext_append_normal(fm, " ");
      owl_fmtext_append_normal(fm, tty ? tty : "");
      owl_fmtext_append_normal(fm, "\n");
    } else {
      owl_fmtext_append_normal(fm, "From: ");
      if (strcasecmp(owl_message_get_class(m), "message")) {
	owl_fmtext_append_normal(fm, "Class ");
	owl_fmtext_append_normal(fm, owl_message_get_class(m));
	owl_fmtext_append_normal(fm, " / Instance ");
	owl_fmtext_append_normal(fm, owl_message_get_instance(m));
	owl_fmtext_append_normal(fm, " / ");
      }
      owl_fmtext_append_normal(fm, frombuff);
      if (strcasecmp(owl_message_get_realm(m), owl_zephyr_get_realm())) {
	owl_fmtext_append_normal(fm, " {");
	owl_fmtext_append_normal(fm, owl_message_get_realm(m));
	owl_fmtext_append_normal(fm, "} ");
      }
      
      /* stick on the zsig */
      zsigbuff=owl_malloc(strlen(owl_message_get_zsig(m))+30);
      owl_message_pretty_zsig(m, zsigbuff);
      owl_fmtext_append_normal(fm, "    (");
      owl_fmtext_append_ztext(fm, zsigbuff);
      owl_fmtext_append_normal(fm, ")");
      owl_fmtext_append_normal(fm, "\n");
      owl_free(zsigbuff);
      
      /* then the indented message */
      owl_style_basic_format_body(fm, m);
      
      /* make personal messages bold for smaat users */
      if (owl_global_is_userclue(&g, OWL_USERCLUE_CLASSES)) {
	if (owl_message_is_personal(m)) {
	  owl_fmtext_addattr(fm, OWL_FMTEXT_ATTR_BOLD);
	}
      }
    }
    
#endif
  } else if (owl_message_is_type_zephyr(m) && owl_message_is_direction_out(m)) {
    char *zsigbuff, *foo;
    owl_fmtext_append_normal(fm, OWL_TABSTR);
    owl_fmtext_append_normal(fm, "To: ");
    foo=short_zuser(owl_message_get_recipient(m));
    owl_fmtext_append_normal(fm, foo);
    owl_free(foo);
    owl_fmtext_append_normal(fm, "  (Zsig: ");
    
    zsigbuff=owl_malloc(strlen(owl_message_get_zsig(m))+30);
    owl_message_pretty_zsig(m, zsigbuff);
    owl_fmtext_append_ztext(fm, zsigbuff);
    owl_free(zsigbuff);
    
    owl_fmtext_append_normal(fm, ")");
    owl_fmtext_append_normal(fm, "\n");
    owl_style_basic_format_body(fm, m);
  } else if (owl_message_is_type_aim(m)) {
    if (owl_message_is_loginout(m)) {
      owl_fmtext_append_normal(fm, OWL_TABSTR);
      if (owl_message_is_login(m)) {
	owl_fmtext_append_bold(fm, "AIM LOGIN");
      } else {
	owl_fmtext_append_bold(fm, "AIM LOGOUT");
      }
      owl_fmtext_append_normal(fm, " for ");
      owl_fmtext_append_normal(fm, owl_message_get_sender(m));
      owl_fmtext_append_normal(fm, "\n");
    } else if (owl_message_is_direction_in(m)) {
      owl_fmtext_append_bold(fm, OWL_TABSTR);
      owl_fmtext_append_bold(fm, "AIM from ");
      owl_fmtext_append_bold(fm, owl_message_get_sender(m));
      owl_fmtext_append_bold(fm, "\n");
      owl_style_basic_format_body(fm, m);
    } else if (owl_message_is_direction_out(m)) {
      owl_fmtext_append_normal(fm, OWL_TABSTR);
      owl_fmtext_append_normal(fm, "AIM sent to ");
      owl_fmtext_append_normal(fm, owl_message_get_recipient(m));
      owl_fmtext_append_normal(fm, "\n");
      owl_style_basic_format_body(fm, m);
    }
  } else if (owl_message_is_type_admin(m)) {
    char *text, *header;
    
    text=owl_message_get_body(m);
    header=owl_message_get_attribute_value(m, "adminheader");

    owl_fmtext_append_normal(fm, OWL_TABSTR);
    owl_fmtext_append_bold(fm, "OWL ADMIN ");
    owl_fmtext_append_ztext(fm, header);
    owl_fmtext_append_normal(fm, "\n");
    owl_style_basic_format_body(fm, m);
  } else {
    char *header;

    header=owl_sprintf("%s from: %s to: %s",
		       owl_message_get_type(m),
		       owl_message_get_sender(m),
		       owl_message_get_recipient(m));

    owl_fmtext_append_normal(fm, OWL_TABSTR);
    owl_fmtext_append_normal(fm, header);
    owl_fmtext_append_normal(fm, "\n");
    owl_style_basic_format_body(fm, m);
    
    owl_free(header);
  }
}
