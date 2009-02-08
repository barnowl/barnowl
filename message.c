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

static owl_fmtext_cache fmtext_cache[OWL_FMTEXT_CACHE_SIZE];
static owl_fmtext_cache * fmtext_cache_next = fmtext_cache;

void owl_message_init_fmtext_cache ()
{
    int i;
    for(i = 0; i < OWL_FMTEXT_CACHE_SIZE; i++) {
        owl_fmtext_init_null(&(fmtext_cache[i].fmtext));
        fmtext_cache[i].message = NULL;
    }
}

owl_fmtext_cache * owl_message_next_fmtext() /*noproto*/
{
    owl_fmtext_cache * f = fmtext_cache_next;
    if(fmtext_cache_next->message != NULL) {
        owl_message_invalidate_format(fmtext_cache_next->message);
    }
    fmtext_cache_next++;
    if(fmtext_cache_next - fmtext_cache == OWL_FMTEXT_CACHE_SIZE)
        fmtext_cache_next = fmtext_cache;
    return f;
}

void owl_message_init(owl_message *m)
{
  m->id=owl_global_get_nextmsgid(&g);
  owl_message_set_direction_none(m);
  m->delete=0;

  owl_message_set_hostname(m, "");
  owl_list_create(&(m->attributes));
  
  /* save the time */
  m->time=time(NULL);
  m->timestr=owl_strdup(ctime(&(m->time)));
  m->timestr[strlen(m->timestr)-1]='\0';

  m->fmtext = NULL;
}

/* add the named attribute to the message.  If an attribute with the
 * name already exists, replace the old value with the new value
 */
void owl_message_set_attribute(owl_message *m, char *attrname, char *attrvalue)
{
  int i, j;
  owl_pair *p = NULL, *pair = NULL;

  /* look for an existing pair with this key, */
  j=owl_list_get_size(&(m->attributes));
  for (i=0; i<j; i++) {
    p=owl_list_get_element(&(m->attributes), i);
    if (!strcmp(owl_pair_get_key(p), attrname)) {
      owl_free(owl_pair_get_value(p));
      pair = p;
      break;
    }
  }

  if(pair ==  NULL) {
    pair = owl_malloc(sizeof(owl_pair));
    owl_pair_create(pair, owl_global_intern(&g, attrname), NULL);
    owl_list_append_element(&(m->attributes), pair);
  }
  owl_pair_set_value(pair, owl_validate_or_convert(attrvalue));
}

/* return the value associated with the named attribute, or NULL if
 * the attribute does not exist
 */
char *owl_message_get_attribute_value(owl_message *m, char *attrname)
{
  int i, j;
  owl_pair *p;

  j=owl_list_get_size(&(m->attributes));
  for (i=0; i<j; i++) {
    p=owl_list_get_element(&(m->attributes), i);
    if (!strcmp(owl_pair_get_key(p), attrname)) {
      return(owl_pair_get_value(p));
    }
  }

  /*
  owl_function_debugmsg("No attribute %s found for message %i",
			attrname,
			owl_message_get_id(m));
  */
  return(NULL);
}

/* We cheat and indent it for now, since we really want this for
 * the 'info' function.  Later there should just be a generic
 * function to indent fmtext.
 */
void owl_message_attributes_tofmtext(owl_message *m, owl_fmtext *fm) {
  int i, j;
  owl_pair *p;
  char *buff;

  owl_fmtext_init_null(fm);

  j=owl_list_get_size(&(m->attributes));
  for (i=0; i<j; i++) {
    p=owl_list_get_element(&(m->attributes), i);
    buff=owl_sprintf("  %-15.15s: %-35.35s\n", owl_pair_get_key(p), owl_pair_get_value(p));
    if(buff == NULL) {
      buff=owl_sprintf("  %-15.15s: %-35.35s\n", owl_pair_get_key(p), "<error>");
      if(buff == NULL)
        buff=owl_strdup("   <error>\n");
    }
    owl_fmtext_append_normal(fm, buff);
    owl_free(buff);
  }
}

void owl_message_invalidate_format(owl_message *m)
{
  if(m->fmtext) {
    m->fmtext->message = NULL;
    owl_fmtext_clear(&(m->fmtext->fmtext));
    m->fmtext=NULL;
  }
}

owl_fmtext *owl_message_get_fmtext(owl_message *m)
{
  owl_message_format(m);
  return(&(m->fmtext->fmtext));
}

void owl_message_format(owl_message *m)
{
  owl_style *s;
  owl_view *v;

  if (!m->fmtext) {
    m->fmtext = owl_message_next_fmtext();
    m->fmtext->message = m;
    /* for now we assume there's just the one view and use that style */
    v=owl_global_get_current_view(&g);
    s=owl_view_get_style(v);

    owl_style_get_formattext(s, &(m->fmtext->fmtext), m);
  }
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


void owl_message_set_islogin(owl_message *m)
{
  owl_message_set_attribute(m, "loginout", "login");
}


void owl_message_set_islogout(owl_message *m)
{
  owl_message_set_attribute(m, "loginout", "logout");
}

int owl_message_is_loginout(owl_message *m)
{
  char *res;

  res=owl_message_get_attribute_value(m, "loginout");
  if (!res) return(0);
  return(1);
}

int owl_message_is_login(owl_message *m)
{
  char *res;

  res=owl_message_get_attribute_value(m, "loginout");
  if (!res) return(0);
  if (!strcmp(res, "login")) return(1);
  return(0);
}


int owl_message_is_logout(owl_message *m)
{
  char *res;

  res=owl_message_get_attribute_value(m, "loginout");
  if (!res) return(0);
  if (!strcmp(res, "logout")) return(1);
  return(0);
}

void owl_message_set_isprivate(owl_message *m)
{
  owl_message_set_attribute(m, "isprivate", "true");
}

int owl_message_is_private(owl_message *m)
{
  char *res;

  res=owl_message_get_attribute_value(m, "isprivate");
  if (!res) return(0);
  return !strcmp(res, "true");
}

char *owl_message_get_timestr(owl_message *m)
{
  if (m->timestr) return(m->timestr);
  return("");
}

void owl_message_set_type_admin(owl_message *m)
{
  owl_message_set_attribute(m, "type", "admin");
}

void owl_message_set_type_loopback(owl_message *m)
{
  owl_message_set_attribute(m, "type", "loopback");
}

void owl_message_set_type_zephyr(owl_message *m)
{
  owl_message_set_attribute(m, "type", "zephyr");
}

void owl_message_set_type_aim(owl_message *m)
{
  owl_message_set_attribute(m, "type", "AIM");
}

void owl_message_set_type(owl_message *m, char* type)
{
  owl_message_set_attribute(m, "type", type);
}

int owl_message_is_type(owl_message *m, char *type) {
  char * t = owl_message_get_attribute_value(m, "type");
  if(!t) return 0;
  return !strcasecmp(t, type);
}
						
int owl_message_is_type_admin(owl_message *m)
{
  return owl_message_is_type(m, "admin");
}

int owl_message_is_type_zephyr(owl_message *m)
{
  return owl_message_is_type(m, "zephyr");
}

int owl_message_is_type_aim(owl_message *m)
{
  return owl_message_is_type(m, "aim");
}

/* XXX TODO: deprecate this */
int owl_message_is_type_jabber(owl_message *m)
{
  return owl_message_is_type(m, "jabber");
}

int owl_message_is_type_loopback(owl_message *m)
{
  return owl_message_is_type(m, "loopback");
}

int owl_message_is_pseudo(owl_message *m)
{
  if (owl_message_get_attribute_value(m, "pseudo")) return(1);
  return(0);
}

char *owl_message_get_text(owl_message *m)
{
  owl_message_format(m);
  return(owl_fmtext_get_text(&(m->fmtext->fmtext)));
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

void owl_message_set_direction(owl_message *m, int direction)
{
  m->direction=direction;
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
  owl_message_format(m);
  return(owl_fmtext_num_lines(&(m->fmtext->fmtext)));
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
  char *z = owl_message_get_attribute_value(m, "zwriteline");
  if (!z) return "";
  return z;
}

void owl_message_set_zwriteline(owl_message *m, char *line)
{
  owl_message_set_attribute(m, "zwriteline", line);
}

int owl_message_is_delete(owl_message *m)
{
  if (m == NULL) return(0);
  if (m->delete==1) return(1);
  return(0);
}

#ifdef HAVE_LIBZEPHYR
ZNotice_t *owl_message_get_notice(owl_message *m)
{
  return(&(m->notice));
}
#else
void *owl_message_get_notice(owl_message *m)
{
  return(NULL);
}
#endif

void owl_message_set_hostname(owl_message *m, char *hostname)
{
  m->hostname=owl_global_intern(&g, hostname);
}

char *owl_message_get_hostname(owl_message *m)
{
  return(m->hostname);
}

void owl_message_curs_waddstr(owl_message *m, WINDOW *win, int aline, int bline, int acol, int bcol, int fgcolor, int bgcolor)
{
  owl_fmtext a, b;

  /* this will ensure that our cached copy is up to date */
  owl_message_format(m);

  owl_fmtext_init_null(&a);
  owl_fmtext_init_null(&b);
  
  owl_fmtext_truncate_lines(&(m->fmtext->fmtext), aline, bline-aline, &a);
  owl_fmtext_truncate_cols(&a, acol, bcol, &b);
  owl_fmtext_colorize(&b, fgcolor);
  owl_fmtext_colorizebg(&b, bgcolor);

  owl_fmtext_curs_waddstr(&b, win);

  owl_fmtext_free(&a);
  owl_fmtext_free(&b);
}

int owl_message_is_personal(owl_message *m)
{
  owl_filter * f = owl_global_get_filter(&g, "personal");
  if(!f) {
      owl_function_error("personal filter is not defined");
      return (0);
  }
  return owl_filter_message_match(f, m);
}

int owl_message_is_question(owl_message *m)
{
  if(!owl_message_is_type_admin(m)) return 0;
  if(owl_message_get_attribute_value(m, "question") != NULL) return 1;
  return 0;
}

int owl_message_is_answered(owl_message *m) {
  char *q;
  if(!owl_message_is_question(m)) return 0;
  q = owl_message_get_attribute_value(m, "question");
  if(!q) return 0;
  return !strcmp(q, "answered");
}

void owl_message_set_isanswered(owl_message *m) {
  owl_message_set_attribute(m, "question", "answered");
}

int owl_message_is_mail(owl_message *m)
{
  if (owl_message_is_type_zephyr(m)) {
    if (!strcasecmp(owl_message_get_class(m), "mail") && owl_message_is_private(m)) {
      return(1);
    } else {
      return(0);
    }
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

/* caller must free return value */
char *owl_message_get_cc_without_recipient(owl_message *m)
{
  char *cc, *out, *end, *user, *shortuser, *recip;

  cc = owl_message_get_cc(m);
  if (cc == NULL)
    return NULL;

  recip = short_zuser(owl_message_get_recipient(m));
  out = owl_malloc(strlen(cc));
  end = out;

  user = strtok(cc, " ");
  while (user != NULL) {
    shortuser = short_zuser(user);
    if (strcasecmp(shortuser, recip) != 0) {
      strcpy(end, user);
      end[strlen(user)] = ' ';
      end += strlen(user) + 1;
    }
    free(shortuser);
    user = strtok(NULL, " ");
  }
  end[0] = '\0';

  owl_free(recip);
  owl_free(cc);

  if (strlen(out) == 0) {
    owl_free(out);
    out = NULL;
  }

  return(out);
}

int owl_message_get_id(owl_message *m)
{
  return(m->id);
}

char *owl_message_get_type(owl_message *m) {
  char * type = owl_message_get_attribute_value(m, "type");
  if(!type) {
    return "generic";
  }
  return type;
}

char *owl_message_get_direction(owl_message *m) {
  switch (m->direction) {
  case OWL_MESSAGE_DIRECTION_IN:
    return("in");
  case OWL_MESSAGE_DIRECTION_OUT:
    return("out");
  case OWL_MESSAGE_DIRECTION_NONE:
    return("none");
  default:
    return("unknown");
  }
}

int owl_message_parse_direction(char *d) {
  if(!strcmp(d, "in")) {
    return OWL_MESSAGE_DIRECTION_IN;
  } else if(!strcmp(d, "out")) {
    return OWL_MESSAGE_DIRECTION_OUT;
  } else {
    return OWL_MESSAGE_DIRECTION_NONE;
  }
}


char *owl_message_get_login(owl_message *m) {
  if (owl_message_is_login(m)) {
    return "login";
  } else if (owl_message_is_logout(m)) {
    return "logout";
  } else {
    return "none";
  }
}


char *owl_message_get_header(owl_message *m) {
  return owl_message_get_attribute_value(m, "adminheader");
}

/* return 1 if the message contains "string", 0 otherwise.  This is
 * case insensitive because the functions it uses are
 */
int owl_message_search(owl_message *m, char *string)
{

  owl_message_format(m); /* is this necessary? */
  
  return (owl_fmtext_search(&(m->fmtext->fmtext), string));
}


/* if loginout == -1 it's a logout message
 *                 0 it's not a login/logout message
 *                 1 it's a login message
 */
void owl_message_create_aim(owl_message *m, char *sender, char *recipient, char *text, int direction, int loginout)
{
  owl_message_init(m);
  owl_message_set_body(m, text);
  owl_message_set_sender(m, sender);
  owl_message_set_recipient(m, recipient);
  owl_message_set_type_aim(m);

  if (direction==OWL_MESSAGE_DIRECTION_IN) {
    owl_message_set_direction_in(m);
  } else if (direction==OWL_MESSAGE_DIRECTION_OUT) {
    owl_message_set_direction_out(m);
  }

  /* for now all messages that aren't loginout are private */
  if (!loginout) {
    owl_message_set_isprivate(m);
  }

  if (loginout==-1) {
    owl_message_set_islogout(m);
  } else if (loginout==1) {
    owl_message_set_islogin(m);
  }
}

void owl_message_create_admin(owl_message *m, char *header, char *text)
{
  owl_message_init(m);
  owl_message_set_type_admin(m);
  owl_message_set_body(m, text);
  owl_message_set_attribute(m, "adminheader", header); /* just a hack for now */
}

/* caller should set the direction */
void owl_message_create_loopback(owl_message *m, char *text)
{
  owl_message_init(m);
  owl_message_set_type_loopback(m);
  owl_message_set_body(m, text);
  owl_message_set_sender(m, "loopsender");
  owl_message_set_recipient(m, "looprecip");
  owl_message_set_isprivate(m);
}

#ifdef HAVE_LIBZEPHYR
void owl_message_create_from_znotice(owl_message *m, ZNotice_t *n)
{
  struct hostent *hent;
  char *ptr, *tmp, *tmp2;
  int len;

  owl_message_init(m);
  
  owl_message_set_type_zephyr(m);
  owl_message_set_direction_in(m);
  
  /* first save the full notice */
  memcpy(&(m->notice), n, sizeof(ZNotice_t));

  /* a little gross, we'll replace \r's with ' ' for now */
  owl_zephyr_hackaway_cr(&(m->notice));
  
  /* save the time, we need to nuke the string saved by message_init */
  if (m->timestr) owl_free(m->timestr);
  m->time=n->z_time.tv_sec;
  m->timestr=owl_strdup(ctime(&(m->time)));
  m->timestr[strlen(m->timestr)-1]='\0';

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
  owl_message_set_zsig(m, owl_zephyr_get_zsig(n, &len));

  if ((ptr=strchr(n->z_recipient, '@'))!=NULL) {
    owl_message_set_realm(m, ptr+1);
  } else {
    owl_message_set_realm(m, owl_zephyr_get_realm());
  }

  /* Set the "isloginout" attribute if it's a login message */
  if (!strcasecmp(n->z_class, "login") || !strcasecmp(n->z_class, OWL_WEBZEPHYR_CLASS)) {
    if (!strcasecmp(n->z_opcode, "user_login") || !strcasecmp(n->z_opcode, "user_logout")) {
      tmp=owl_zephyr_get_field(n, 1);
      owl_message_set_attribute(m, "loginhost", tmp);
      owl_free(tmp);

      tmp=owl_zephyr_get_field(n, 3);
      owl_message_set_attribute(m, "logintty", tmp);
      owl_free(tmp);
    }

    if (!strcasecmp(n->z_opcode, "user_login")) {
      owl_message_set_islogin(m);
    } else if (!strcasecmp(n->z_opcode, "user_logout")) {
      owl_message_set_islogout(m);
    }
  }

  
  /* set the "isprivate" attribute if it's a private zephyr.
   ``private'' means recipient is non-empty and doesn't start wit 
   `@' */
  if (*n->z_recipient && *n->z_recipient != '@') {
    owl_message_set_isprivate(m);
  }

  /* set the "isauto" attribute if it's an autoreply */
  if (!strcasecmp(n->z_message, "Automated reply:") ||
      !strcasecmp(n->z_opcode, "auto")) {
    owl_message_set_attribute(m, "isauto", "");
  }

  /* save the hostname */
  owl_function_debugmsg("About to do gethostbyaddr");
  hent=gethostbyaddr((char *) &(n->z_uid.zuid_addr), sizeof(n->z_uid.zuid_addr), AF_INET);
  if (hent && hent->h_name) {
    owl_message_set_hostname(m, hent->h_name);
  } else {
    owl_message_set_hostname(m, inet_ntoa(n->z_sender_addr));
  }

  /* set the body */
  tmp=owl_zephyr_get_message(n, m);
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
    ret=owl_zcrypt_decrypt(out, owl_message_get_body(m), owl_message_get_class(m), owl_message_get_instance(m));
    if (ret==0) {
      owl_message_set_body(m, out);
    } else {
      owl_free(out);
    }
  }
#endif  
}
#else
void owl_message_create_from_znotice(owl_message *m, void *n)
{
}
#endif

/* If 'direction' is '0' it is a login message, '1' is a logout message. */
void owl_message_create_pseudo_zlogin(owl_message *m, int direction, char *user, char *host, char *time, char *tty)
{
  char *longuser, *ptr;

#ifdef HAVE_LIBZEPHYR
  memset(&(m->notice), 0, sizeof(ZNotice_t));
#endif
  
  longuser=long_zuser(user);
  
  owl_message_init(m);
  
  owl_message_set_type_zephyr(m);
  owl_message_set_direction_in(m);

  owl_message_set_attribute(m, "pseudo", "");
  owl_message_set_attribute(m, "loginhost", host ? host : "");
  owl_message_set_attribute(m, "logintty", tty ? tty : "");

  owl_message_set_sender(m, longuser);
  owl_message_set_class(m, "LOGIN");
  owl_message_set_instance(m, longuser);
  owl_message_set_recipient(m, "");
  if (direction==0) {
    owl_message_set_opcode(m, "USER_LOGIN");
    owl_message_set_islogin(m);
  } else if (direction==1) {
    owl_message_set_opcode(m, "USER_LOGOUT");
    owl_message_set_islogout(m);
  }

  if ((ptr=strchr(longuser, '@'))!=NULL) {
    owl_message_set_realm(m, ptr+1);
  } else {
    owl_message_set_realm(m, owl_zephyr_get_realm());
  }

  owl_message_set_body(m, "<uninitialized>");

  /* save the hostname */
  owl_function_debugmsg("create_pseudo_login: host is %s", host ? host : "");
  owl_message_set_hostname(m, host ? host : "");
  owl_free(longuser);
}

void owl_message_create_from_zwriteline(owl_message *m, char *line, char *body, char *zsig)
{
  owl_zwrite z;
  int ret;
  char hostbuff[5000];
  
  owl_message_init(m);

  /* create a zwrite for the purpose of filling in other message fields */
  owl_zwrite_create_from_line(&z, line);

  /* set things */
  owl_message_set_direction_out(m);
  owl_message_set_type_zephyr(m);
  owl_message_set_sender(m, owl_zephyr_get_sender());
  owl_message_set_class(m, owl_zwrite_get_class(&z));
  owl_message_set_instance(m, owl_zwrite_get_instance(&z));
  if (owl_zwrite_get_numrecips(&z)>0) {
    char *longzuser = long_zuser(owl_zwrite_get_recip_n(&z, 0));
    owl_message_set_recipient(m,
			      longzuser); /* only gets the first user, must fix */
    owl_free(longzuser);
  }
  owl_message_set_opcode(m, owl_zwrite_get_opcode(&z));
  owl_message_set_realm(m, owl_zwrite_get_realm(&z)); /* also a hack, but not here */
  owl_message_set_zwriteline(m, line);
  owl_message_set_body(m, body);
  owl_message_set_zsig(m, zsig);
  
  /* save the hostname */
  ret=gethostname(hostbuff, MAXHOSTNAMELEN);
  hostbuff[MAXHOSTNAMELEN]='\0';
  if (ret) {
    owl_message_set_hostname(m, "localhost");
  } else {
    owl_message_set_hostname(m, hostbuff);
  }

  /* set the "isprivate" attribute if it's a private zephyr. */
  if (owl_zwrite_is_personal(&z)) {
    owl_message_set_isprivate(m);
  }

  owl_zwrite_free(&z);
}

void owl_message_free(owl_message *m)
{
  int i, j;
  owl_pair *p;
#ifdef HAVE_LIBZEPHYR    
  if (owl_message_is_type_zephyr(m) && owl_message_is_direction_in(m)) {
    ZFreeNotice(&(m->notice));
  }
#endif
  if (m->timestr) owl_free(m->timestr);

  /* free all the attributes */
  j=owl_list_get_size(&(m->attributes));
  for (i=0; i<j; i++) {
    p=owl_list_get_element(&(m->attributes), i);
    owl_free(owl_pair_get_value(p));
    owl_free(p);
  }

  owl_list_free_simple(&(m->attributes));
 
  owl_message_invalidate_format(m);
}
