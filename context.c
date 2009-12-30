#include <string.h>
#include "owl.h"

#define SET_ACTIVE(ctx, new) ctx->mode = ((ctx->mode)&~OWL_CTX_ACTIVE_BITS)|new
#define SET_MODE(ctx, new) ctx->mode = ((ctx->mode)&~OWL_CTX_MODE_BITS)|new

int owl_context_init(owl_context *ctx)
{
  ctx->mode = OWL_CTX_STARTUP;
  ctx->data = NULL;
  return 0;
}


/* returns whether test matches the current context */
int owl_context_matches(const owl_context *ctx, int test)
{
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

void *owl_context_get_data(const owl_context *ctx)
{
  return ctx->data;
}

int owl_context_get_mode(const owl_context *ctx)
{
  return ctx->mode & OWL_CTX_MODE_BITS;
}

int owl_context_get_active(const owl_context *ctx)
{
  return ctx->mode & OWL_CTX_ACTIVE_BITS;
}

int owl_context_is_startup(const owl_context *ctx)
{
  return (ctx->mode & OWL_CTX_STARTUP)?1:0;
}

int owl_context_is_interactive(const owl_context *ctx)
{
  return(ctx->mode & OWL_CTX_INTERACTIVE)?1:0;
}
