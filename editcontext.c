#include "owl.h"

#include <assert.h>

bool owl_is_editcontext(const owl_context *ctx)
{
  return owl_context_matches(ctx, OWL_CTX_TYPWIN);
}

owl_context *owl_editcontext_new(int mode, owl_editwin *e, const char *keymap)
{
  owl_context *ctx = owl_context_new(mode, owl_editwin_ref(e), keymap,
				     owl_editwin_get_window(e));
  ctx->delete_cb = owl_editcontext_delete_cb;
  /* TODO: the flags are really screwy. */
  assert(owl_is_editcontext(ctx));
  return ctx;
}

owl_editwin *owl_editcontext_get_editwin(const owl_context *ctx)
{
  if (!owl_is_editcontext(ctx)) return NULL;
  return ctx->data;
}

void owl_editcontext_delete_cb(owl_context *ctx)
{
  if (owl_is_editcontext(ctx) && ctx->data) {
    owl_editwin_unref(ctx->data);
    ctx->data = NULL;
  }
}
