
#ifndef __g_cclosure_user_marshal_MARSHAL_H__
#define __g_cclosure_user_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* INT:STRING (marshal_types:2) */
extern void g_cclosure_user_marshal_INT__STRING (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* INT:INT (marshal_types:3) */
extern void g_cclosure_user_marshal_INT__INT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* INT:BOOL (marshal_types:4) */
extern void g_cclosure_user_marshal_INT__BOOLEAN (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);
#define g_cclosure_user_marshal_INT__BOOL	g_cclosure_user_marshal_INT__BOOLEAN

/* INT:VOID (marshal_types:6) */
extern void g_cclosure_user_marshal_INT__VOID (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* BOOL:VOID (marshal_types:7) */
extern void g_cclosure_user_marshal_BOOLEAN__VOID (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);
#define g_cclosure_user_marshal_BOOL__VOID	g_cclosure_user_marshal_BOOLEAN__VOID

/* STRING:VOID (marshal_types:8) */
extern void g_cclosure_user_marshal_STRING__VOID (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* INT:STRING (marshal_types:11) */

/* STRING:BOOL (marshal_types:13) */
extern void g_cclosure_user_marshal_STRING__BOOLEAN (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);
#define g_cclosure_user_marshal_STRING__BOOL	g_cclosure_user_marshal_STRING__BOOLEAN

/* STRING:INT (marshal_types:14) */
extern void g_cclosure_user_marshal_STRING__INT (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* STRING:STRING (marshal_types:15) */
extern void g_cclosure_user_marshal_STRING__STRING (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

G_END_DECLS

#endif /* __g_cclosure_user_marshal_MARSHAL_H__ */

