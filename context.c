#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

#define SET_ACTIVE(ctx, new) ctx->mode = ((ctx->mode)&~OWL_CTX_ACTIVE_BITS)|new
#define SET_MODE(ctx, new) ctx->mode = ((ctx->mode)&~OWL_CTX_MODE_BITS)|new

int owl_context_init(owl_context *ctx) {
  ctx->mode = OWL_CTX_STARTUP;
  ctx->data = NULL;
  return 0;
}


/* returns whether test matches the current context */
int owl_context_matches(owl_context *ctx, int test) {
  /*owl_function_debugmsg(", current: 0x%04x test: 0x%04x\n", ctx->mode, test);*/
  if ((((ctx->mode&OWL_CTX_MODE_BITS) & test)
       || !(test&OWL_CTX_MODE_BITS))
      && 
      (((ctx->mode&OWL_CTX_ACTIVE_BITS) & test) 
       || !(test&OWL_CTX_ACTIVE_BITS))) {
    return 1;
  } else {
    return 0;
  }
}

void *owl_context_get_data(owl_context *ctx) {
  return ctx->data;
}

int owl_context_get_mode(owl_context *ctx) {
  return ctx->mode & OWL_CTX_MODE_BITS;
}

int owl_context_get_active(owl_context *ctx) {
  return ctx->mode & OWL_CTX_ACTIVE_BITS;
}

int owl_context_is_startup(owl_context *ctx) {
  return (ctx->mode & OWL_CTX_STARTUP)?1:0;
}


void owl_context_set_startup(owl_context *ctx) {
  SET_MODE(ctx, OWL_CTX_STARTUP);
}

void owl_context_set_readconfig(owl_context *ctx) {
  SET_MODE(ctx, OWL_CTX_READCONFIG);
}

void owl_context_set_interactive(owl_context *ctx) {
  SET_MODE(ctx, OWL_CTX_INTERACTIVE);
}

void owl_context_set_popless(owl_context *ctx, owl_viewwin *vw) {
  ctx->data = (void*)vw;
  SET_ACTIVE(ctx, OWL_CTX_POPLESS);
}

void owl_context_set_recv(owl_context *ctx) {
  SET_ACTIVE(ctx, OWL_CTX_RECV);
}

void owl_context_set_editmulti(owl_context *ctx, owl_editwin *ew) {
  ctx->data = (void*)ew;
  SET_ACTIVE(ctx, OWL_CTX_EDITMULTI);
}

void owl_context_set_editline(owl_context *ctx, owl_editwin *ew) {
  ctx->data = (void*)ew;
  SET_ACTIVE(ctx, OWL_CTX_EDITLINE);
}

