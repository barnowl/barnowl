#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_message_init(owl_message *m)
{
  time_t t;

  m->id=owl_global_get_nextmsgid(&g);
  m->type=OWL_MESSAGE_TYPE_GENERIC;
  owl_message_set_direction_none(m);
  m->delete=0;
  strcpy(m->hostname, "");
  m->zwriteline=strdup("");

  owl_list_create(&(m->attributes));
  
  /* save the time */
  t=time(NULL);
  m->time=owl_strdup(ctime(&t));
  m->time[strlen(m->time)-1]='\0';
}

void owl_message_set_attribute(owl_message *m, char *attrname, char *attrvalue)
{
  /* add the named attribute to the message.  If an attribute with the
     name already exists, replace the old value with the new value */

  int i, j;
  owl_pair *p;

  /* look for an existing pair with this key, and nuke the entry if
     found */
  j=owl_list_get_size(&(m->attributes));
  for (i=0; i<j; i++) {
    p=owl_list_get_element(&(m->attributes), i);
    if (!strcmp(owl_pair_get_key(p), attrname)) {
      owl_free(owl_pair_get_key(p));
      owl_free(owl_pair_get_value(p));
      owl_free(p);
      owl_list_remove_element(&(m->attributes), i);
      break;
    }
  }

  p=owl_malloc(sizeof(owl_pair));
  owl_pair_create(p, owl_strdup(attrname), owl_strdup(attrvalue));
  owl_list_append_element(&(m->attributes), p);
}

char *owl_message_get_attribute_value(owl_message *m, char *attrname)
{
  /* return the value associated with the named attribute, or NULL if
     the attribute does not exist */
  int i, j;
  owl_pair *p;

  j=owl_list_get_size(&(m->attributes));
  for (i=0; i<j; i++) {
    p=owl_list_get_element(&(m->attributes), i);
    if (!strcmp(owl_pair_get_key(p), attrname)) {
      return(owl_pair_get_value(p));
    }
  }
  owl_function_debugmsg("No attribute %s found", attrname);
  return(NULL);
}


owl_fmtext *owl_message_get_fmtext(owl_message *m)
{
  return(&(m->fmtext));
}

void owl_message_set_class(owl_message *m, char *class)
{
  owl_message_set_attribute(m, "class", class);
}

char *owl_message_get_class(owl_message *m)
{
  char *class;

  class=owl_message_get_attribute_value(m, "class");
  if (!class) return("");
  return(class);
}

void owl_message_set_instance(owl_message *m, char *inst)
{
  owl_message_set_attribute(m, "instance", inst);
}

char *owl_message_get_instance(owl_message *m)
{
  char *instance;

  instance=owl_message_get_attribute_value(m, "instance");
  if (!instance) return("");
  return(instance);
}

void owl_message_set_sender(owl_message *m, char *sender)
{
  owl_message_set_attribute(m, "sender", sender);
}

char *owl_message_get_sender(owl_message *m)
{
  char *sender;

  sender=owl_message_get_attribute_value(m, "sender");
  if (!sender) return("");
  return(sender);
}

void owl_message_set_zsig(owl_message *m, char *zsig)
{
  owl_message_set_attribute(m, "zsig", zsig);
}

char *owl_message_get_zsig(owl_message *m)
{
  char *zsig;

  zsig=owl_message_get_attribute_value(m, "zsig");
  if (!zsig) return("");
  return(zsig);
}

void owl_message_set_recipient(owl_message *m, char *recip)
{
  owl_message_set_attribute(m, "recipient", recip);
}

char *owl_message_get_recipient(owl_message *m)
{
  /* this is stupid for outgoing messages, we need to fix it. */

  char *recip;

  recip=owl_message_get_attribute_value(m, "recipient");
  if (!recip) return("");
  return(recip);
}

void owl_message_set_realm(owl_message *m, char *realm)
{
  owl_message_set_attribute(m, "realm", realm);
}

char *owl_message_get_realm(owl_message *m)
{
  char *realm;
  
  realm=owl_message_get_attribute_value(m, "realm");
  if (!realm) return("");
  return(realm);
}

void owl_message_set_body(owl_message *m, char *body)
{
  owl_message_set_attribute(m, "body", body);
}

char *owl_message_get_body(owl_message *m)
{
  char *body;

  body=owl_message_get_attribute_value(m, "body");
  if (!body) return("");
  return(body);
}


void owl_message_set_opcode(owl_message *m, char *opcode)
{
  owl_message_set_attribute(m, "opcode", opcode);
}

char *owl_message_get_opcode(owl_message *m)
{
  char *opcode;

  opcode=owl_message_get_attribute_value(m, "opcode");
  if (!opcode) return("");
  return(opcode);
}

char *owl_message_get_timestr(owl_message *m)
{
  return(m->time);
}

void owl_message_set_type_admin(owl_message *m)
{
  m->type=OWL_MESSAGE_TYPE_ADMIN;
}

void owl_message_set_type_zephyr(owl_message *m)
{
  m->type=OWL_MESSAGE_TYPE_ZEPHYR;
}

void owl_message_set_type_aim(owl_message *m)
{
  m->type=OWL_MESSAGE_TYPE_AIM;
}
						
int owl_message_is_type_admin(owl_message *m)
{
  if (m->type==OWL_MESSAGE_TYPE_ADMIN) return(1);
  return(0);
}

int owl_message_is_type_zephyr(owl_message *m)
{
  if (m->type==OWL_MESSAGE_TYPE_ZEPHYR) return(1);
  return(0);
}

int owl_message_is_type_aim(owl_message *m)
{
  if (m->type==OWL_MESSAGE_TYPE_AIM) return(1);
  return(0);
}

int owl_message_is_type_generic(owl_message *m)
{
  if (m->type==OWL_MESSAGE_TYPE_GENERIC) return(1);
  return(0);
}

char *owl_message_get_text(owl_message *m)
{
  return(owl_fmtext_get_text(&(m->fmtext)));
}

void owl_message_set_direction_in(owl_message *m)
{
  m->direction=OWL_MESSAGE_DIRECTION_IN;
}

void owl_message_set_direction_out(owl_message *m)
{
  m->direction=OWL_MESSAGE_DIRECTION_OUT;
}

void owl_message_set_direction_none(owl_message *m)
{
  m->direction=OWL_MESSAGE_DIRECTION_NONE;
}

int owl_message_is_direction_in(owl_message *m)
{
  if (m->direction==OWL_MESSAGE_DIRECTION_IN) return(1);
  return(0);
}

int owl_message_is_direction_out(owl_message *m)
{
  if (m->direction==OWL_MESSAGE_DIRECTION_OUT) return(1);
  return(0);
}

int owl_message_is_direction_none(owl_message *m)
{
  if (m->direction==OWL_MESSAGE_DIRECTION_NONE) return(1);
  return(0);
}

int owl_message_get_numlines(owl_message *m)
{
  if (m == NULL) return(0);
  return(owl_fmtext_num_lines(&(m->fmtext)));
}

void owl_message_mark_delete(owl_message *m)
{
  if (m == NULL) return;
  m->delete=1;
}

void owl_message_unmark_delete(owl_message *m)
{
  if (m == NULL) return;
  m->delete=0;
}

char *owl_message_get_zwriteline(owl_message *m)
{
  return(m->zwriteline);
}

void owl_message_set_zwriteline(owl_message *m, char *line)
{
  m->zwriteline=strdup(line);
}

int owl_message_is_delete(owl_message *m)
{
  if (m == NULL) return(0);
  if (m->delete==1) return(1);
  return(0);
}

ZNotice_t *owl_message_get_notice(owl_message *m)
{
  return(&(m->notice));
}

char *owl_message_get_hostname(owl_message *m)
{
  return(m->hostname);
}


void owl_message_curs_waddstr(owl_message *m, WINDOW *win, int aline, int bline, int acol, int bcol, int color)
{
  owl_fmtext a, b;

  owl_fmtext_init_null(&a);
  owl_fmtext_init_null(&b);
  
  owl_fmtext_truncate_lines(&(m->fmtext), aline, bline-aline+1, &a);
  owl_fmtext_truncate_cols(&a, acol, bcol, &b);
  if (color!=OWL_COLOR_DEFAULT) {
    owl_fmtext_colorize(&b, color);
  }

  if (owl_global_is_search_active(&g)) {
    owl_fmtext_search_and_highlight(&b, owl_global_get_search_string(&g));
  }
      
  owl_fmtext_curs_waddstr(&b, win);

  owl_fmtext_free(&a);
  owl_fmtext_free(&b);
}

int owl_message_is_personal(owl_message *m)
{
  if (owl_message_is_type_zephyr(m)) {
    if (strcasecmp(owl_message_get_class(m), "message")) return(0);
    if (strcasecmp(owl_message_get_instance(m), "personal")) return(0);
    if (!strcasecmp(owl_message_get_recipient(m), ZGetSender()) ||
	!strcasecmp(owl_message_get_sender(m), ZGetSender())) {
      return(1);
    }
  }
  return(0);
}

/* true if the message is only intended for one recipient (me) */
int owl_message_is_to_me(owl_message *m)
{
  if (owl_message_is_type_zephyr(m)) {
    if (!strcasecmp(owl_message_get_recipient(m), ZGetSender())) {
      return(1);
    } else {
      return(0);
    }
  } else if (owl_message_is_type_aim(m)) {
    /* right now we don't support chat rooms */
    return(1);
  } else if (owl_message_is_type_admin(m)) {
    return(1);
  }
  return(0);
}


int owl_message_is_from_me(owl_message *m)
{
  if (owl_message_is_type_zephyr(m)) {
    if (!strcasecmp(owl_message_get_sender(m), ZGetSender())) {
      return(1);
    } else {
      return(0);
    }
  } else if (owl_message_is_type_aim(m)) {
    if (!strcasecmp(owl_message_get_sender(m), owl_global_get_aim_screenname(&g))) {
      return(1);
    } else {
      return(0);
    }
  } else if (owl_message_is_type_admin(m)) {
    return(0);
  }
  return(0);
}

int owl_message_is_mail(owl_message *m)
{
  if (owl_message_is_type_zephyr(m)) {
    if (!strcasecmp(owl_message_get_class(m), "mail") && owl_message_is_to_me(m)) {
      return(1);
    } else {
      return(0);
    }
  }
  return(0);
}

int owl_message_is_ping(owl_message *m)
{
  if (owl_message_is_type_zephyr(m)) {
    if (!strcasecmp(owl_message_get_opcode(m), "ping")) {
      return(1);
    } else {
      return(0);
    }
  }
  return(0);
}

int owl_message_is_login(owl_message *m)
{
  if (owl_message_is_type_zephyr(m)) {
    if (!strcasecmp(owl_message_get_class(m), "login")) {
      return(1);
    } else {
      return(0);
    }
  } else if (owl_message_is_type_aim(m)) {
    /* deal with this once we can use buddy lists */
    return(0);
  }
       
  return(0);
}

int owl_message_is_burningears(owl_message *m)
{
  /* we should add a global to cache the short zsender */
  char sender[LINE], *ptr;

  /* if the message is from us or to us, it doesn't count */
  if (owl_message_is_from_me(m) || owl_message_is_to_me(m)) return(0);

  if (owl_message_is_type_zephyr(m)) {
    strcpy(sender, ZGetSender());
    ptr=strchr(sender, '@');
    if (ptr) *ptr='\0';
  } else if (owl_message_is_type_aim(m)) {
    strcpy(sender, owl_global_get_aim_screenname(&g));
  } else {
    return(0);
  }

  if (stristr(owl_message_get_body(m), sender)) {
    return(1);
  }
  return(0);
}

/* caller must free return value. */
char *owl_message_get_cc(owl_message *m)
{
  char *cur, *out, *end;

  cur = owl_message_get_body(m);
  while (*cur && *cur==' ') cur++;
  if (strncasecmp(cur, "cc:", 3)) return(NULL);
  cur+=3;
  while (*cur && *cur==' ') cur++;
  out = owl_strdup(cur);
  end = strchr(out, '\n');
  if (end) end[0] = '\0';
  return(out);
}

int owl_message_get_id(owl_message *m)
{
  return(m->id);
}
					
int owl_message_search(owl_message *m, char *string)
{
  /* return 1 if the message contains "string", 0 otherwise.  This is
   * case insensitive because the functions it uses are */

  return (owl_fmtext_search(&(m->fmtext), string));
}

void owl_message_create(owl_message *m, char *header, char *text)
{
  char *indent;

  owl_message_init(m);
  owl_message_set_body(m, text);

  indent=owl_malloc(strlen(text)+owl_text_num_lines(text)*OWL_MSGTAB+10);
  owl_text_indent(indent, text, OWL_MSGTAB);
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);
  owl_fmtext_append_ztext(&(m->fmtext), header);
  owl_fmtext_append_normal(&(m->fmtext), "\n");
  owl_fmtext_append_ztext(&(m->fmtext), indent);
  if (text[strlen(text)-1]!='\n') {
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  }

  owl_free(indent);
}

void owl_message_create_aim(owl_message *m, char *sender, char *recipient, char *text)
{
  char *indent;

  owl_message_init(m);
  owl_message_set_body(m, text);
  owl_message_set_sender(m, sender);
  owl_message_set_recipient(m, recipient);
  owl_message_set_type_aim(m);

  indent=owl_malloc(strlen(text)+owl_text_num_lines(text)*OWL_MSGTAB+10);
  owl_text_indent(indent, text, OWL_MSGTAB);
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);
  owl_fmtext_append_normal(&(m->fmtext), "AIM: ");
  owl_fmtext_append_normal(&(m->fmtext), sender);
  owl_fmtext_append_normal(&(m->fmtext), " -> ");
  owl_fmtext_append_normal(&(m->fmtext), recipient);
  owl_fmtext_append_normal(&(m->fmtext), "\n");
  owl_fmtext_append_ztext(&(m->fmtext), indent);
  if (text[strlen(text)-1]!='\n') {
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  }
  
  owl_free(indent);
}

void owl_message_create_admin(owl_message *m, char *header, char *text)
{
  char *indent;

  owl_message_init(m);
  owl_message_set_type_admin(m);

  owl_message_set_body(m, text);

  /* do something to make it clear the notice shouldn't be used for now */

  indent=owl_malloc(strlen(text)+owl_text_num_lines(text)*OWL_MSGTAB+10);
  owl_text_indent(indent, text, OWL_MSGTAB);
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);
  owl_fmtext_append_bold(&(m->fmtext), "OWL ADMIN ");
  owl_fmtext_append_ztext(&(m->fmtext), header);
  owl_fmtext_append_normal(&(m->fmtext), "\n");
  owl_fmtext_append_ztext(&(m->fmtext), indent);
  if (text[strlen(text)-1]!='\n') {
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  }

  owl_free(indent);
}

void owl_message_create_from_znotice(owl_message *m, ZNotice_t *n)
{
  struct hostent *hent;
  int k;
  char *ptr, *tmp, *tmp2;

  owl_message_init(m);
  
  owl_message_set_type_zephyr(m);
  owl_message_set_direction_in(m);
  
  /* first save the full notice */
  memcpy(&(m->notice), n, sizeof(ZNotice_t));

  /* a little gross, we'll reaplace \r's with ' ' for now */
  owl_zephyr_hackaway_cr(&(m->notice));
  
  m->delete=0;

  /* set other info */
  owl_message_set_sender(m, n->z_sender);
  owl_message_set_class(m, n->z_class);
  owl_message_set_instance(m, n->z_class_inst);
  owl_message_set_recipient(m, n->z_recipient);
  if (n->z_opcode) {
    owl_message_set_opcode(m, n->z_opcode);
  } else {
    owl_message_set_opcode(m, "");
  }
  owl_message_set_zsig(m, n->z_message);

  if ((ptr=strchr(n->z_recipient, '@'))!=NULL) {
    owl_message_set_realm(m, ptr+1);
  } else {
    owl_message_set_realm(m, ZGetRealm());
  }

  m->zwriteline=strdup("");

  /* set the body */
  ptr=owl_zephyr_get_message(n, &k);
  tmp=owl_malloc(k+10);
  memcpy(tmp, ptr, k);
  tmp[k]='\0';
  if (owl_global_is_newlinestrip(&g)) {
    tmp2=owl_util_stripnewlines(tmp);
    owl_message_set_body(m, tmp2);
    owl_free(tmp2);
  } else {
    owl_message_set_body(m, tmp);
  }
  owl_free(tmp);

#ifdef OWL_ENABLE_ZCRYPT
  /* if zcrypt is enabled try to decrypt the message */
  if (owl_global_is_zcrypt(&g) && !strcasecmp(n->z_opcode, "crypt")) {
    char *out;
    int ret;

    out=owl_malloc(strlen(owl_message_get_body(m))*16+20);
    ret=zcrypt_decrypt(out, owl_message_get_body(m), owl_message_get_class(m), owl_message_get_instance(m));
    if (ret==0) {
      owl_message_set_body(m, out);
    } else {
      owl_free(out);
    }
  }
#endif  

  /* save the hostname */
  owl_function_debugmsg("About to do gethostbyaddr");
  hent=gethostbyaddr((char *) &(n->z_uid.zuid_addr), sizeof(n->z_uid.zuid_addr), AF_INET);
  if (hent && hent->h_name) {
    strcpy(m->hostname, hent->h_name);
  } else {
    strcpy(m->hostname, inet_ntoa(n->z_sender_addr));
  }

  /* save the time */
  m->time=owl_strdup(ctime((time_t *) &n->z_time.tv_sec));
  m->time[strlen(m->time)-1]='\0';

  /* create the formatted message */
  if (owl_global_is_config_format(&g)) {
    _owl_message_make_text_from_config(m);
  } else if (owl_global_is_userclue(&g, OWL_USERCLUE_CLASSES)) {
    _owl_message_make_text_from_notice_standard(m);
  } else {
    _owl_message_make_text_from_notice_simple(m);
  }

}

void owl_message_create_from_zwriteline(owl_message *m, char *line, char *body, char *zsig)
{
  owl_zwrite z;
  int ret;
  
  owl_message_init(m);

  /* create a zwrite for the purpose of filling in other message fields */
  owl_zwrite_create_from_line(&z, line);

  /* set things */
  owl_message_set_direction_out(m);
  owl_message_set_type_zephyr(m);
  owl_message_set_sender(m, ZGetSender());
  owl_message_set_class(m, owl_zwrite_get_class(&z));
  owl_message_set_instance(m, owl_zwrite_get_instance(&z));
  owl_message_set_recipient(m,
			    long_zuser(owl_zwrite_get_recip_n(&z, 0))); /* only gets the first user, must fix */
  owl_message_set_opcode(m, owl_zwrite_get_opcode(&z));
  owl_message_set_realm(m, owl_zwrite_get_realm(&z)); /* also a hack, but not here */
  m->zwriteline=owl_strdup(line);
  owl_message_set_body(m, body);
  owl_message_set_zsig(m, zsig);
  
  /* save the hostname */
  ret=gethostname(m->hostname, MAXHOSTNAMELEN);
  if (ret) {
    strcpy(m->hostname, "localhost");
  }

  /* create the formatted message */
  if (owl_global_is_config_format(&g)) {
    _owl_message_make_text_from_config(m);
  } else if (owl_global_is_userclue(&g, OWL_USERCLUE_CLASSES)) {
    _owl_message_make_text_from_zwriteline_standard(m);
  } else {
    _owl_message_make_text_from_zwriteline_simple(m);
  }

  owl_zwrite_free(&z);
}

void _owl_message_make_text_from_config(owl_message *m)
{
  char *body, *indent;

  owl_fmtext_init_null(&(m->fmtext));

  /* get body from the config */
  body=owl_config_getmsg(m, 1);
  
  /* indent */
  indent=owl_malloc(strlen(body)+owl_text_num_lines(body)*OWL_TAB+10);
  owl_text_indent(indent, body, OWL_TAB);

  /* fmtext_append.  This needs to change */
  owl_fmtext_append_ztext(&(m->fmtext), indent);

  owl_free(indent);
  owl_free(body);
}

void _owl_message_make_text_from_zwriteline_standard(owl_message *m)
{
  char *indent, *text, *zsigbuff, *foo;

  text=owl_message_get_body(m);

  indent=owl_malloc(strlen(text)+owl_text_num_lines(text)*OWL_MSGTAB+10);
  owl_text_indent(indent, text, OWL_MSGTAB);
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);
  owl_fmtext_append_normal(&(m->fmtext), "Zephyr sent to ");
  foo=short_zuser(owl_message_get_recipient(m));
  owl_fmtext_append_normal(&(m->fmtext), foo);
  owl_free(foo);
  owl_fmtext_append_normal(&(m->fmtext), "  (Zsig: ");

  zsigbuff=owl_malloc(strlen(owl_message_get_zsig(m))+30);
  owl_message_pretty_zsig(m, zsigbuff);
  owl_fmtext_append_ztext(&(m->fmtext), zsigbuff);
  owl_free(zsigbuff);
  
  owl_fmtext_append_normal(&(m->fmtext), ")");
  owl_fmtext_append_normal(&(m->fmtext), "\n");
  owl_fmtext_append_ztext(&(m->fmtext), indent);
  if (text[strlen(text)-1]!='\n') {
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  }

  owl_free(indent);
}

void _owl_message_make_text_from_zwriteline_simple(owl_message *m)
{
  char *indent, *text, *zsigbuff, *foo;

  text=owl_message_get_body(m);

  indent=owl_malloc(strlen(text)+owl_text_num_lines(text)*OWL_MSGTAB+10);
  owl_text_indent(indent, text, OWL_MSGTAB);
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);
  owl_fmtext_append_normal(&(m->fmtext), "To: ");
  foo=short_zuser(owl_message_get_recipient(m));
  owl_fmtext_append_normal(&(m->fmtext), foo);
  owl_free(foo);
  owl_fmtext_append_normal(&(m->fmtext), "  (Zsig: ");

  zsigbuff=owl_malloc(strlen(owl_message_get_zsig(m))+30);
  owl_message_pretty_zsig(m, zsigbuff);
  owl_fmtext_append_ztext(&(m->fmtext), zsigbuff);
  owl_free(zsigbuff);
  
  owl_fmtext_append_normal(&(m->fmtext), ")");
  owl_fmtext_append_normal(&(m->fmtext), "\n");
  owl_fmtext_append_ztext(&(m->fmtext), indent);
  if (text[strlen(text)-1]!='\n') {
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  }

  owl_free(indent);
}

void _owl_message_make_text_from_notice_standard(owl_message *m)
{
  char *body, *indent, *ptr, *zsigbuff, frombuff[LINE];
  ZNotice_t *n;

  n=&(m->notice);
  
  /* get the body */
  body=owl_malloc(strlen(owl_message_get_body(m))+30);
  strcpy(body, owl_message_get_body(m));

  /* add a newline if we need to */
  if (body[0]!='\0' && body[strlen(body)-1]!='\n') {
    strcat(body, "\n");
  }

  /* do the indenting into indent */
  indent=owl_malloc(strlen(body)+owl_text_num_lines(body)*OWL_MSGTAB+10);
  owl_text_indent(indent, body, OWL_MSGTAB);

  /* edit the from addr for printing */
  strcpy(frombuff, owl_message_get_sender(m));
  ptr=strchr(frombuff, '@');
  if (ptr && !strncmp(ptr+1, ZGetRealm(), strlen(ZGetRealm()))) {
    *ptr='\0';
  }

  /* set the message for printing */
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);

  if (!strcasecmp(owl_message_get_opcode(m), "ping") && owl_message_is_to_me(m)) {
    owl_fmtext_append_bold(&(m->fmtext), "PING");
    owl_fmtext_append_normal(&(m->fmtext), " from ");
    owl_fmtext_append_bold(&(m->fmtext), frombuff);
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  } else if (!strcasecmp(n->z_class, "login")) {
    char *ptr, host[LINE], tty[LINE];
    int len;

    ptr=owl_zephyr_get_field(n, 1, &len);
    strncpy(host, ptr, len);
    host[len]='\0';
    ptr=owl_zephyr_get_field(n, 3, &len);
    strncpy(tty, ptr, len);
    tty[len]='\0';
    
    if (!strcasecmp(n->z_opcode, "user_login")) {
      owl_fmtext_append_bold(&(m->fmtext), "LOGIN");
    } else if (!strcasecmp(n->z_opcode, "user_logout")) {
      owl_fmtext_append_bold(&(m->fmtext), "LOGOUT");
    }
    owl_fmtext_append_normal(&(m->fmtext), " for ");
    ptr=short_zuser(n->z_class_inst);
    owl_fmtext_append_bold(&(m->fmtext), ptr);
    owl_free(ptr);
    owl_fmtext_append_normal(&(m->fmtext), " at ");
    owl_fmtext_append_normal(&(m->fmtext), host);
    owl_fmtext_append_normal(&(m->fmtext), " ");
    owl_fmtext_append_normal(&(m->fmtext), tty);
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  } else {
    owl_fmtext_append_normal(&(m->fmtext), owl_message_get_class(m));
    owl_fmtext_append_normal(&(m->fmtext), " / ");
    owl_fmtext_append_normal(&(m->fmtext), owl_message_get_instance(m));
    owl_fmtext_append_normal(&(m->fmtext), " / ");
    owl_fmtext_append_bold(&(m->fmtext), frombuff);
    if (strcasecmp(owl_message_get_realm(m), ZGetRealm())) {
      owl_fmtext_append_normal(&(m->fmtext), " {");
      owl_fmtext_append_normal(&(m->fmtext), owl_message_get_realm(m));
      owl_fmtext_append_normal(&(m->fmtext), "} ");
    }
    if (n->z_opcode[0]!='\0') {
      owl_fmtext_append_normal(&(m->fmtext), " [");
      owl_fmtext_append_normal(&(m->fmtext), owl_message_get_opcode(m));
      owl_fmtext_append_normal(&(m->fmtext), "] ");
    }

    /* stick on the zsig */
    zsigbuff=owl_malloc(strlen(owl_message_get_zsig(m))+30);
    owl_message_pretty_zsig(m, zsigbuff);
    owl_fmtext_append_normal(&(m->fmtext), "    (");
    owl_fmtext_append_ztext(&(m->fmtext), zsigbuff);
    owl_fmtext_append_normal(&(m->fmtext), ")");
    owl_fmtext_append_normal(&(m->fmtext), "\n");
    owl_free(zsigbuff);

    /* then the indented message */
    owl_fmtext_append_ztext(&(m->fmtext), indent);

    /* make personal messages bold for smaat users */
    if (owl_global_is_userclue(&g, OWL_USERCLUE_CLASSES)) {
      if (owl_message_is_personal(m)) {
	owl_fmtext_addattr((&m->fmtext), OWL_FMTEXT_ATTR_BOLD);
      }
    }
  }

  owl_free(body);
  owl_free(indent);
}

void _owl_message_make_text_from_notice_simple(owl_message *m)
{
  char *body, *indent, *ptr, *zsigbuff, frombuff[LINE];
  ZNotice_t *n;

  n=&(m->notice);

  /* get the body */
  body=owl_strdup(owl_message_get_body(m));
  body=realloc(body, strlen(body)+30);

  /* add a newline if we need to */
  if (body[0]!='\0' && body[strlen(body)-1]!='\n') {
    strcat(body, "\n");
  }

  /* do the indenting into indent */
  indent=owl_malloc(strlen(body)+owl_text_num_lines(body)*OWL_MSGTAB+10);
  owl_text_indent(indent, body, OWL_MSGTAB);

  /* edit the from addr for printing */
  strcpy(frombuff, owl_message_get_sender(m));
  ptr=strchr(frombuff, '@');
  if (ptr && !strncmp(ptr+1, ZGetRealm(), strlen(ZGetRealm()))) {
    *ptr='\0';
  }

  /* set the message for printing */
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);

  if (!strcasecmp(owl_message_get_opcode(m), "ping")) {
    owl_fmtext_append_bold(&(m->fmtext), "PING");
    owl_fmtext_append_normal(&(m->fmtext), " from ");
    owl_fmtext_append_bold(&(m->fmtext), frombuff);
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  } else if (!strcasecmp(owl_message_get_class(m), "login")) {
    char *ptr, host[LINE], tty[LINE];
    int len;

    ptr=owl_zephyr_get_field(n, 1, &len);
    strncpy(host, ptr, len);
    host[len]='\0';
    ptr=owl_zephyr_get_field(n, 3, &len);
    strncpy(tty, ptr, len);
    tty[len]='\0';
    
    if (!strcasecmp(owl_message_get_opcode(m), "user_login")) {
      owl_fmtext_append_bold(&(m->fmtext), "LOGIN");
    } else if (!strcasecmp(owl_message_get_opcode(m), "user_logout")) {
      owl_fmtext_append_bold(&(m->fmtext), "LOGOUT");
    }
    owl_fmtext_append_normal(&(m->fmtext), " for ");
    ptr=short_zuser(owl_message_get_instance(m));
    owl_fmtext_append_bold(&(m->fmtext), ptr);
    owl_free(ptr);
    owl_fmtext_append_normal(&(m->fmtext), " at ");
    owl_fmtext_append_normal(&(m->fmtext), host);
    owl_fmtext_append_normal(&(m->fmtext), " ");
    owl_fmtext_append_normal(&(m->fmtext), tty);
    owl_fmtext_append_normal(&(m->fmtext), "\n");
  } else {
    owl_fmtext_append_normal(&(m->fmtext), "From: ");
    if (strcasecmp(owl_message_get_class(m), "message")) {
      owl_fmtext_append_normal(&(m->fmtext), "Class ");
      owl_fmtext_append_normal(&(m->fmtext), owl_message_get_class(m));
      owl_fmtext_append_normal(&(m->fmtext), " / Instance ");
      owl_fmtext_append_normal(&(m->fmtext), owl_message_get_instance(m));
      owl_fmtext_append_normal(&(m->fmtext), " / ");
    }
    owl_fmtext_append_normal(&(m->fmtext), frombuff);
    if (strcasecmp(owl_message_get_realm(m), ZGetRealm())) {
      owl_fmtext_append_normal(&(m->fmtext), " {");
      owl_fmtext_append_normal(&(m->fmtext), owl_message_get_realm(m));
      owl_fmtext_append_normal(&(m->fmtext), "} ");
    }

    /* stick on the zsig */
    zsigbuff=owl_malloc(strlen(owl_message_get_zsig(m))+30);
    owl_message_pretty_zsig(m, zsigbuff);
    owl_fmtext_append_normal(&(m->fmtext), "    (");
    owl_fmtext_append_ztext(&(m->fmtext), zsigbuff);
    owl_fmtext_append_normal(&(m->fmtext), ")");
    owl_fmtext_append_normal(&(m->fmtext), "\n");
    owl_free(zsigbuff);

    /* then the indented message */
    owl_fmtext_append_ztext(&(m->fmtext), indent);

    /* make personal messages bold for smaat users */
    if (owl_global_is_userclue(&g, OWL_USERCLUE_CLASSES)) {
      if (owl_message_is_personal(m)) {
	owl_fmtext_addattr((&m->fmtext), OWL_FMTEXT_ATTR_BOLD);
      }
    }
  }

  owl_free(body);
  owl_free(indent);
}

void owl_message_pretty_zsig(owl_message *m, char *buff)
{
  /* stick a one line version of the zsig in buff */
  char *ptr;

  strcpy(buff, owl_message_get_zsig(m));
  ptr=strchr(buff, '\n');
  if (ptr) ptr[0]='\0';
}

void owl_message_free(owl_message *m)
{
  int i, j;
  owl_pair *p;
    
  if (owl_message_is_type_zephyr(m) && owl_message_is_direction_in(m)) {
    ZFreeNotice(&(m->notice));
  }
  if (m->time) owl_free(m->time);
  if (m->zwriteline) owl_free(m->zwriteline);

  /* free all the attributes */
  j=owl_list_get_size(&(m->attributes));
  for (i=0; i<j; i++) {
    p=owl_list_get_element(&(m->attributes), i);
    owl_free(owl_pair_get_key(p));
    owl_free(owl_pair_get_value(p));
    owl_free(p);
  }

  owl_list_free_simple(&(m->attributes));
 
  owl_fmtext_free(&(m->fmtext));
}
