#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "owl.h"
#include <perl.h>
#include "XSUB.h"

static const char fileIdent[] = "$Id$";



extern XS(boot_owl);

static void owl_perl_xs_init(pTHX) {
  char *file = __FILE__;
  dXSUB_SYS;
  {
    newXS("owl::bootstrap", boot_owl, file);
  }
}


int owl_readconfig(char *file) {
  int ret;
  PerlInterpreter *p;
  char *codebuff, filename[1024];
  char *embedding[5];
  struct stat statbuff;

  if (file==NULL) {
    sprintf(filename, "%s/%s", getenv("HOME"), ".owlconf");
  } else {
    strcpy(filename, file);
  }
  embedding[0]="";
  embedding[1]=filename;
  embedding[2]=0;

  /* create and initialize interpreter */
  p=perl_alloc();
  owl_global_set_perlinterp(&g, (void*)p);
  perl_construct(p);

  owl_global_set_no_have_config(&g);

  ret=stat(filename, &statbuff);
  if (ret) {
    return(0);
  }

  ret=perl_parse(p, owl_perl_xs_init, 2, embedding, NULL);
  if (ret) return(-1);

  ret=perl_run(p);
  if (ret) return(-1);

  owl_global_set_have_config(&g);

  /* create variables */
  perl_get_sv("owl::id", TRUE);
  perl_get_sv("owl::type", TRUE);
  perl_get_sv("owl::direction", TRUE);
  perl_get_sv("owl::class", TRUE);
  perl_get_sv("owl::instance", TRUE);
  perl_get_sv("owl::recipient", TRUE);
  perl_get_sv("owl::sender", TRUE);
  perl_get_sv("owl::realm", TRUE);
  perl_get_sv("owl::opcode", TRUE);
  perl_get_sv("owl::zsig", TRUE);
  perl_get_sv("owl::msg", TRUE);
  perl_get_sv("owl::time", TRUE);
  perl_get_sv("owl::host", TRUE);
  perl_get_av("owl::fields", TRUE);
  
  /* perl bootstrapping code */
  codebuff = 
    "                                             \n"
    "package owl;                                 \n"
    "                                             \n"
    "bootstrap owl 0.01;                          \n"
    "                                             \n"
    "package main;                                \n";

  perl_eval_pv(codebuff, FALSE);


  /* check if we have the formatting function */
  if (perl_get_cv("owl::format_msg", FALSE)) {
    owl_global_set_config_format(&g, 1);
  }
  return(0);
}


/* caller is responsible for freeing returned string */
char *owl_config_execute(char *line) {
  STRLEN len;
  SV *response;
  char *out, *preout;

  if (!owl_global_have_config(&g)) return NULL;

  /* execute the subroutine */
  response = perl_eval_pv(line, FALSE);

  preout=SvPV(response, len);
  /* leave enough space in case we have to add a newline */
  out = owl_malloc(strlen(preout)+2);
  strncpy(out, preout, len);
  out[len] = '\0';
  if (!strlen(out) || out[strlen(out)-1]!='\n') {
    strcat(out, "\n");
  }

  return(out);
}

char *owl_config_getmsg(owl_message *m, int mode) {
  /* if mode==1 we are doing message formatting.  The returned
   * formatted message needs to be freed by the caller.
   *
   * if mode==0 we are just doing the message-has-been-received
   * thing.
  */

  int i, j, len;
  char *ptr, *ptr2;

  if (!owl_global_have_config(&g)) return("");

  /* set owl::msg */
  ptr=owl_message_get_body(m);
  len=strlen(ptr);
  ptr2=owl_malloc(len+20);
  memcpy(ptr2, ptr, len);
  ptr2[len]='\0';
  if (ptr2[len-1]!='\n') {
    strcat(ptr2, "\n");
  }
  sv_setpv(perl_get_sv("owl::msg", TRUE), ptr2);
  owl_free(ptr2);

  /* set owl::zsig */
  ptr=owl_message_get_zsig(m);
  len=strlen(ptr);
  if (len>0) {
    ptr2=owl_malloc(len+20);
    memcpy(ptr2, ptr, len);
    ptr2[len]='\0';
    if (ptr2[len-1]=='\n') {  /* do we really need this? */
      ptr2[len-1]='\0';
    }
    sv_setpv(perl_get_sv("owl::zsig", TRUE), ptr2);
    owl_free(ptr2);
  } else {
    sv_setpv(perl_get_sv("owl::zsig", TRUE), "");
  }

  /* set owl::type */
  if (owl_message_is_type_zephyr(m)) {
    sv_setpv(perl_get_sv("owl::type", TRUE), "zephyr");
  } else if (owl_message_is_type_admin(m)) {
    sv_setpv(perl_get_sv("owl::type", TRUE), "admin");
  } else {
    sv_setpv(perl_get_sv("owl::type", TRUE), "unknown");
  }

  /* set owl::direction */
  if (owl_message_is_direction_in(m)) {
    sv_setpv(perl_get_sv("owl::direction", TRUE), "in");
  } else if (owl_message_is_direction_out(m)) {
    sv_setpv(perl_get_sv("owl::direction", TRUE), "out");
  } else if (owl_message_is_direction_none(m)) {
    sv_setpv(perl_get_sv("owl::direction", TRUE), "none");
  } else {
    sv_setpv(perl_get_sv("owl::direction", TRUE), "unknown");
  }

  /* set everything else */
  sv_setpv(perl_get_sv("owl::class", TRUE), owl_message_get_class(m));
  sv_setpv(perl_get_sv("owl::instance", TRUE), owl_message_get_instance(m));
  sv_setpv(perl_get_sv("owl::sender", TRUE), owl_message_get_sender(m));
  sv_setpv(perl_get_sv("owl::realm", TRUE), owl_message_get_realm(m));
  sv_setpv(perl_get_sv("owl::recipient", TRUE), owl_message_get_recipient(m));
  sv_setpv(perl_get_sv("owl::opcode", TRUE), owl_message_get_opcode(m));
  sv_setpv(perl_get_sv("owl::time", TRUE), owl_message_get_timestr(m));
  sv_setpv(perl_get_sv("owl::host", TRUE), owl_message_get_hostname(m));
  sv_setiv(perl_get_sv("owl::id", TRUE), owl_message_get_id(m));

  /* free old @fields ? */
  /* I don't think I need to do this, but ask marc to make sure */
  /*
  j=av_len(perl_get_av("fields", TRUE));
  for (i=0; i<j; i++) {
    tmpsv=av_pop(perl_get_av("fields", TRUE));
    SvREFCNT_dec(tmpsv);
  }
  */

  /* set owl::fields */
  if (owl_message_is_type_zephyr(m) && owl_message_is_direction_in(m)) {
    av_clear(perl_get_av("owl::fields", TRUE));
    j=owl_zephyr_get_num_fields(owl_message_get_notice(m));
    for (i=0; i<j; i++) {
      ptr=owl_zephyr_get_field(owl_message_get_notice(m), i+1, &len);
      ptr2=owl_malloc(len+10);
      memcpy(ptr2, ptr, len);
      ptr2[len]='\0';
      av_push(perl_get_av("owl::fields", TRUE), newSVpvn(ptr2, len));
      owl_free(ptr2);
    }
  }

  /* for backwards compatibilty, because I'm an idiot */
  if (owl_message_is_type_zephyr(m) && owl_message_is_direction_in(m)) {
    av_clear(perl_get_av("fields", TRUE));
    j=owl_zephyr_get_num_fields(owl_message_get_notice(m));
    for (i=0; i<j; i++) {
      ptr=owl_zephyr_get_field(owl_message_get_notice(m), i+1, &len);
      ptr2=owl_malloc(len+10);
      memcpy(ptr2, ptr, len);
      ptr2[len]='\0';
      av_push(perl_get_av("fields", TRUE), newSVpvn(ptr2, len));
      owl_free(ptr2);
    }
  }

  /* run the procedure corresponding to the mode */
  if (mode==1) {
    return(owl_config_execute("owl::format_msg();"));
  } else {
    ptr=owl_config_execute("owl::receive_msg();");
    if (ptr) owl_free(ptr);
    return(NULL);
  }
}

