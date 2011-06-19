#include "owl.h"

#include <assert.h>

bool owl_is_editcontext(const owl_context *ctx)
{
  return owl_context_matches(ctx, OWL_CTX_TYPWIN);
}

G_GNUC_WARN_UNUSED_RESULT owl_context *owl_editcontext_new(int mode, owl_editwin *e, const char *keymap, void (*deactivate_cb)(owl_context*), void *cbdata)
{
  owl_context *ctx = owl_context_new(mode, owl_editwin_ref(e), keymap,
				     owl_editwin_get_window(e));
  ctx->deactivate_cb = deactivate_cb;
  ctx->delete_cb = owl_editcontext_delete_cb;
  ctx->cbdata = cbdata;
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
