#ifndef INC_BARNOWL_ZTEXT_H
#define INC_BARNOWL_ZTEXT_H

typedef enum {
    ZTEXT_NODE_ENV,
    ZTEXT_NODE_STRING,
} ztext_node_type;

typedef struct _ztext_env {
    char *label;
    char opener;
    char closer;
    struct _ztext_node *content;
    struct _ztext_node *tail;
} ztext_env;

typedef struct _ztext_node {
    ztext_node_type type;
    union {
        struct _ztext_env *env;
        char *string;
    };
    struct _ztext_node *next;
} ztext_node;
#endif
