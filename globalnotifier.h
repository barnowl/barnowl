#ifndef __BARNOWL_GLOBAL_NOTIFIER_H__
#define __BARNOWL_GLOBAL_NOTIFIER_H__

#include <glib-object.h>
#include "owl.h"

G_BEGIN_DECLS

#define OWL_TYPE_GLOBAL_NOTIFIER owl_global_notifier_get_type()

#define OWL_GLOBAL_NOTIFIER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), OWL_TYPE_GLOBAL_NOTIFIER, OwlGlobalNotifier))

#define OWL_GLOBAL_NOTIFIER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), OWL_TYPE_GLOBAL_NOTIFIER, OwlGlobalNotifierClass))

#define OWL_IS_GLOBAL_NOTIFIER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OWL_TYPE_GLOBAL_NOTIFIER))

#define OWL_IS_GLOBAL_NOTIFIER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), OWL_TYPE_GLOBAL_NOTIFIER))

#define OWL_GLOBAL_NOTIFIER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), OWL_TYPE_GLOBAL_NOTIFIER, OwlGlobalNotifierClass))

typedef struct _OwlGlobalNotifierClass OwlGlobalNotifierClass;

struct _OwlGlobalNotifier {
    GObject parent;
    owl_global *g;
};

struct _OwlGlobalNotifierClass {
    GObjectClass parent_class;
};

GType owl_global_notifier_get_type (void);

OwlGlobalNotifier* owl_global_notifier_new(owl_global *g);

G_END_DECLS

#endif /* __BARNOWL_GLOBAL_NOTIFIER_H__ */
