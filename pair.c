#include "owl.h"

void owl_pair_create(owl_pair *p, void *key, void *value) {
  p->key=key;
  p->value=value;
}

void owl_pair_set_key(owl_pair *p, void *key) {
  p->key=key;
}

void owl_pair_set_value(owl_pair *p, void *value) {
  p->value=value;
}

void *owl_pair_get_key(owl_pair *p) {
  return(p->key);
}

void *owl_pair_get_value(owl_pair *p) {
  return(p->value);
}
