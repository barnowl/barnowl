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

void owl_message_init_raw(owl_message *m) {
  time_t t;

  m->id=owl_global_get_nextmsgid(&g);
  m->type=OWL_MESSAGE_TYPE_GENERIC;
  owl_message_set_direction_none(m);
  m->delete=0;
  m->sender=owl_strdup("");
  m->class=owl_strdup("");
  m->inst=owl_strdup("");
  m->recip=owl_strdup("");
  m->opcode=owl_strdup("");
  m->realm=owl_strdup("");
  m->zsig=owl_strdup("");
  strcpy(m->hostname, "");
  m->zwriteline=strdup("");

  /* save the time */
  t=time(NULL);
  m->time=owl_strdup(ctime(&t));
  m->time[strlen(m->time)-1]='\0';
}


owl_fmtext *owl_message_get_fmtext(owl_message *m) {
  return(&(m->fmtext));
}

void owl_message_set_class(owl_message *m, char *class) {
  if (m->class) owl_free(m->class);
  m->class=owl_strdup(class);
}

char *owl_message_get_class(owl_message *m) {
  return(m->class);
}

void owl_message_set_instance(owl_message *m, char *inst) {
  if (m->inst) owl_free(m->inst);
  m->inst=owl_strdup(inst);
}

char *owl_message_get_instance(owl_message *m) {
  return(m->inst);
}

void owl_message_set_sender(owl_message *m, char *sender) {
  if (m->sender) owl_free(m->sender);
  m->sender=owl_strdup(sender);
}

char *owl_message_get_sender(owl_message *m) {
  return(m->sender);
}

void owl_message_set_zsig(owl_message *m, char *zsig) {
  if (m->zsig) owl_free(m->zsig);
  m->zsig=owl_strdup(zsig);
}

char *owl_message_get_zsig(owl_message *m) {
  return(m->zsig);
}

void owl_message_set_recipient(owl_message *m, char *recip) {
  if (m->recip) owl_free(m->recip);
  m->recip=owl_strdup(recip);
}

char *owl_message_get_recipient(owl_message *m) {
  /* this is stupid for outgoing messages, we need to fix it. */
     
  if (m->type==OWL_MESSAGE_TYPE_ZEPHYR) {
    return(m->recip);
  } else if (owl_message_is_direction_out(m)) {
    return(m->zwriteline);
  } else {
    return(m->recip);
  }
}

void owl_message_set_realm(owl_message *m, char *realm) {
  if (m->realm) owl_free(m->realm);
  m->realm=owl_strdup(realm);
}

char *owl_message_get_realm(owl_message *m) {
  return(m->realm);
}

void owl_message_set_opcode(owl_message *m, char *opcode) {
  if (m->opcode) free(m->opcode);
  m->opcode=owl_strdup(opcode);
}

char *owl_message_get_opcode(owl_message *m) {
  return(m->opcode);
}

char *owl_message_get_timestr(owl_message *m) {
  return(m->time);
}

void owl_message_set_type_admin(owl_message *m) {
  m->type=OWL_MESSAGE_TYPE_ADMIN;
}

void owl_message_set_type_zephyr(owl_message *m) {
  m->type=OWL_MESSAGE_TYPE_ZEPHYR;
}
						
int owl_message_is_type_admin(owl_message *m) {
  if (m->type==OWL_MESSAGE_TYPE_ADMIN) return(1);
  return(0);
}

int owl_message_is_type_zephyr(owl_message *m) {
  if (m->type==OWL_MESSAGE_TYPE_ZEPHYR) return(1);
  return(0);
}

int owl_message_is_type_generic(owl_message *m) {
  if (m->type==OWL_MESSAGE_TYPE_GENERIC) return(1);
  return(0);
}

char *owl_message_get_text(owl_message *m) {
  return(owl_fmtext_get_text(&(m->fmtext)));
}

char *owl_message_get_body(owl_message *m) {
  return(m->body);
}

void owl_message_set_direction_in(owl_message *m) {
  m->direction=OWL_MESSAGE_DIRECTION_IN;
}

void owl_message_set_direction_out(owl_message *m) {
  m->direction=OWL_MESSAGE_DIRECTION_OUT;
}

void owl_message_set_direction_none(owl_message *m) {
  m->direction=OWL_MESSAGE_DIRECTION_NONE;
}

int owl_message_is_direction_in(owl_message *m) {
  if (m->direction==OWL_MESSAGE_DIRECTION_IN) return(1);
  return(0);
}

int owl_message_is_direction_out(owl_message *m) {
  if (m->direction==OWL_MESSAGE_DIRECTION_OUT) return(1);
  return(0);
}

int owl_message_is_direction_none(owl_message *m) {
  if (m->direction==OWL_MESSAGE_DIRECTION_NONE) return(1);
  return(0);
}

int owl_message_get_numlines(owl_message *m) {
  if (m == NULL) return(0);
  return(owl_fmtext_num_lines(&(m->fmtext)));
}

void owl_message_mark_delete(owl_message *m) {
  if (m == NULL) return;
  m->delete=1;
}

void owl_message_unmark_delete(owl_message *m) {
  if (m == NULL) return;
  m->delete=0;
}

char *owl_message_get_zwriteline(owl_message *m) {
  return(m->zwriteline);
}

void owl_message_set_zwriteline(owl_message *m, char *line) {
  m->zwriteline=strdup(line);
}

int owl_message_is_delete(owl_message *m) {
  if (m == NULL) return(0);
  if (m->delete==1) return(1);
  return(0);
}

ZNotice_t *owl_message_get_notice(owl_message *m) {
  return(&(m->notice));
}

char *owl_message_get_hostname(owl_message *m) {
  return(m->hostname);
}


void owl_message_curs_waddstr(owl_message *m, WINDOW *win, int aline, int bline, int acol, int bcol, int color) {
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

int owl_message_is_personal(owl_message *m) {
  if (strcasecmp(owl_message_get_class(m), "message")) return(0);
  if (strcasecmp(owl_message_get_instance(m), "personal")) return(0);
  if (!strcmp(owl_message_get_recipient(m), ZGetSender()) ||
      !strcmp(owl_message_get_sender(m), ZGetSender())) {
    return(1);
  }
  return(0);
}

int owl_message_is_private(owl_message *m) {
  if (!strcmp(owl_message_get_recipient(m), ZGetSender())) return(1);
  return(0);
}

int owl_message_is_mail(owl_message *m) {
  if (!strcasecmp(owl_message_get_class(m), "mail") && owl_message_is_private(m)) {
    return(1);
  }
  return(0);
}

int owl_message_is_ping(owl_message *m) {
  if (!strcasecmp(owl_message_get_opcode(m), "ping")) return(1);
  return(0);
}

int owl_message_is_login(owl_message *m) {
  if (!strcasecmp(owl_message_get_class(m), "login")) return(1);
  return(0);
  /* is this good enough? */
}

int owl_message_is_burningears(owl_message *m) {
  /* we should add a global to cache the short zsender */
  char sender[LINE], *ptr;

  /* if the message is from us or to us, it doesn't count */
  if (!strcasecmp(ZGetSender(), owl_message_get_sender(m))) return(0);
  if (!strcasecmp(ZGetSender(), owl_message_get_recipient(m))) return(0);

  strcpy(sender, ZGetSender());
  ptr=strchr(sender, '@');
  if (ptr) *ptr='\0';

  if (stristr(owl_message_get_body(m), sender)) {
    return(1);
  }
  return(0);
}

/* caller must free return value. */
char *owl_message_get_cc(owl_message *m) {
  char *cur, *out, *end;

  cur = owl_message_get_body(m);
  while (*cur && *cur==' ') cur++;
  if (strncasecmp(cur, "cc:", 2)) return(NULL);
  cur+=3;
  while (*cur && *cur==' ') cur++;
  out = owl_strdup(cur);
  end = strchr(out, '\n');
  if (end) end[0] = '\0';
  return(out);
}

int owl_message_get_id(owl_message *m) {
  return(m->id);
}
					
int owl_message_search(owl_message *m, char *string) {
  /* return 1 if the message contains "string", 0 otherwise.  This is
   * case insensitive because the functions it uses are */

  return (owl_fmtext_search(&(m->fmtext), string));
}

void owl_message_create(owl_message *m, char *header, char *text) {
  char *indent;

  owl_message_init_raw(m);

  m->body=owl_strdup(text);

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

void owl_message_create_admin(owl_message *m, char *header, char *text) {
  char *indent;

  owl_message_init_raw(m);
  owl_message_set_type_admin(m);

  m->body=owl_strdup(text);

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

void owl_message_create_from_znotice(owl_message *m, ZNotice_t *n) {
  struct hostent *hent;
  int k, ret;
  char *ptr;

  m->id=owl_global_get_nextmsgid(&g);
  owl_message_set_type_zephyr(m);
  owl_message_set_direction_in(m);
  
  /* first save the full notice */
  memcpy(&(m->notice), n, sizeof(ZNotice_t));

  /* a little gross, we'll reaplace \r's with ' ' for now */
  owl_zephyr_hackaway_cr(&(m->notice));
  
  m->delete=0;

  /* set other info */
  m->sender=owl_strdup(n->z_sender);
  m->class=owl_strdup(n->z_class);
  m->inst=owl_strdup(n->z_class_inst);
  m->recip=owl_strdup(n->z_recipient);
  if (n->z_opcode) {
    m->opcode=owl_strdup(n->z_opcode);
  } else {
    n->z_opcode=owl_strdup("");
  }
  m->zsig=owl_strdup(n->z_message);

  if ((ptr=strchr(n->z_recipient, '@'))!=NULL) {
    m->realm=owl_strdup(ptr+1);
  } else {
    m->realm=owl_strdup(ZGetRealm());
  }

  m->zwriteline=strdup("");

  /* set the body */
  ptr=owl_zephyr_get_message(n, &k);
  m->body=owl_malloc(k+10);
  memcpy(m->body, ptr, k);
  m->body[k]='\0';

  /* if zcrypt is enabled try to decrypt the message */
  if (owl_global_is_zcrypt(&g) && !strcasecmp(n->z_opcode, "crypt")) {
    char *out;

    out=owl_malloc(strlen(m->body)*16+20);
    ret=zcrypt_decrypt(out, m->body, m->class, m->inst);
    if (ret==0) {
      owl_free(m->body);
      m->body=out;
    } else {
      owl_free(out);
    }
  }

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

void owl_message_create_from_zwriteline(owl_message *m, char *line, char *body, char *zsig) {
  owl_zwrite z;
  int ret;
  
  owl_message_init_raw(m);

  /* create a zwrite for the purpose of filling in other message fields */
  owl_zwrite_create_from_line(&z, line);

  /* set things */
  owl_message_set_direction_out(m);
  owl_message_set_type_zephyr(m);
  owl_message_set_sender(m, ZGetSender());
  owl_message_set_class(m, owl_zwrite_get_class(&z));
  owl_message_set_instance(m, owl_zwrite_get_instance(&z));
  m->recip=long_zuser(owl_zwrite_get_recip_n(&z, 0)); /* only gets the first user, must fix */
  owl_message_set_opcode(m, owl_zwrite_get_opcode(&z));
  m->realm=owl_strdup(owl_zwrite_get_realm(&z)); /* also a hack, but not here */
  m->zwriteline=owl_strdup(line);
  m->body=owl_strdup(body);
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

void _owl_message_make_text_from_config(owl_message *m) {
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

void _owl_message_make_text_from_zwriteline_standard(owl_message *m) {
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

void _owl_message_make_text_from_zwriteline_simple(owl_message *m) {
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

void _owl_message_make_text_from_notice_standard(owl_message *m) {
  char *body, *indent, *ptr, *zsigbuff, frombuff[LINE];
  ZNotice_t *n;

  n=&(m->notice);
  
  /* get the body */
  body=owl_malloc(strlen(m->body)+30);
  strcpy(body, m->body);

  /* add a newline if we need to */
  if (body[0]!='\0' && body[strlen(body)-1]!='\n') {
    strcat(body, "\n");
  }

  /* do the indenting into indent */
  indent=owl_malloc(strlen(body)+owl_text_num_lines(body)*OWL_MSGTAB+10);
  owl_text_indent(indent, body, OWL_MSGTAB);

  /* edit the from addr for printing */
  strcpy(frombuff, m->sender);
  ptr=strchr(frombuff, '@');
  if (ptr && !strncmp(ptr+1, ZGetRealm(), strlen(ZGetRealm()))) {
    *ptr='\0';
  }

  /* set the message for printing */
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);

  if (!strcasecmp(n->z_opcode, "ping") && owl_message_is_private(m)) {
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
    owl_fmtext_append_normal(&(m->fmtext), m->class);
    owl_fmtext_append_normal(&(m->fmtext), " / ");
    owl_fmtext_append_normal(&(m->fmtext), m->inst);
    owl_fmtext_append_normal(&(m->fmtext), " / ");
    owl_fmtext_append_bold(&(m->fmtext), frombuff);
    if (strcasecmp(owl_message_get_realm(m), ZGetRealm())) {
      owl_fmtext_append_normal(&(m->fmtext), " {");
      owl_fmtext_append_normal(&(m->fmtext), owl_message_get_realm(m));
      owl_fmtext_append_normal(&(m->fmtext), "} ");
    }
    if (n->z_opcode[0]!='\0') {
      owl_fmtext_append_normal(&(m->fmtext), " [");
      owl_fmtext_append_normal(&(m->fmtext), n->z_opcode);
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

void _owl_message_make_text_from_notice_simple(owl_message *m) {
  char *body, *indent, *ptr, *zsigbuff, frombuff[LINE];
  ZNotice_t *n;

  n=&(m->notice);

  /* get the body */
  body=owl_malloc(strlen(m->body)+30);
  strcpy(body, m->body);

  /* add a newline if we need to */
  if (body[0]!='\0' && body[strlen(body)-1]!='\n') {
    strcat(body, "\n");
  }

  /* do the indenting into indent */
  indent=owl_malloc(strlen(body)+owl_text_num_lines(body)*OWL_MSGTAB+10);
  owl_text_indent(indent, body, OWL_MSGTAB);

  /* edit the from addr for printing */
  strcpy(frombuff, m->sender);
  ptr=strchr(frombuff, '@');
  if (ptr && !strncmp(ptr+1, ZGetRealm(), strlen(ZGetRealm()))) {
    *ptr='\0';
  }

  /* set the message for printing */
  owl_fmtext_init_null(&(m->fmtext));
  owl_fmtext_append_normal(&(m->fmtext), OWL_TABSTR);

  if (!strcasecmp(n->z_opcode, "ping")) {
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
    owl_fmtext_append_normal(&(m->fmtext), "From: ");
    if (strcasecmp(m->class, "message")) {
      owl_fmtext_append_normal(&(m->fmtext), "Class ");
      owl_fmtext_append_normal(&(m->fmtext), m->class);
      owl_fmtext_append_normal(&(m->fmtext), " / Instance ");
      owl_fmtext_append_normal(&(m->fmtext), m->inst);
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

void owl_message_pretty_zsig(owl_message *m, char *buff) {
  /* stick a one line version of the zsig in buff */
  char *ptr;

  strcpy(buff, m->zsig);
  ptr=strchr(buff, '\n');
  if (ptr) ptr[0]='\0';
}

void owl_message_free(owl_message *m) {
  if (owl_message_is_type_zephyr(m) && owl_message_is_direction_in(m)) {
    ZFreeNotice(&(m->notice));
  }
  if (m->sender) owl_free(m->sender);
  if (m->recip) owl_free(m->recip);
  if (m->class) owl_free(m->class);
  if (m->inst) owl_free(m->inst);
  if (m->opcode) owl_free(m->opcode);
  if (m->time) owl_free(m->time);
  if (m->realm) owl_free(m->realm);
  if (m->body) owl_free(m->body);
  if (m->zwriteline) owl_free(m->zwriteline);
 
  owl_fmtext_free(&(m->fmtext));
}

