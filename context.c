#include "owl.h"

#define SET_ACTIVE(ctx, new) ctx->mode = ((ctx->mode)&~OWL_CTX_ACTIVE_BITS)|new
#define SET_MODE(ctx, new) ctx->mode = ((ctx->mode)&~OWL_CTX_MODE_BITS)|new

/* TODO: dependency from owl_context -> owl_window is annoying. */
CALLER_OWN owl_context *owl_context_new(int mode, void *data, const char *keymap, owl_window *cursor)
{
  owl_context *c;
  if (!(mode & OWL_CTX_MODE_BITS))
    mode |= OWL_CTX_INTERACTIVE;
  c = g_new0(owl_context, 1);
  c->mode = mode;
  c->data = data;
  c->cursor = cursor ? g_object_ref(cursor) : NULL;
  c->keymap = g_strdup(keymap);
  return c;
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

void owl_context_deactivate(owl_context *ctx)
{
  if (ctx->deactivate_cb)
    ctx->deactivate_cb(ctx);
}

void owl_context_delete(owl_context *ctx)
{
  if (ctx->cursor)
    g_object_unref(ctx->cursor);
  g_free(ctx->keymap);
  if (ctx->delete_cb)
    ctx->delete_cb(ctx);
  g_free(ctx);
}
