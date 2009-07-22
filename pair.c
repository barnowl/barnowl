#include "owl.h"

void owl_pair_create(owl_pair *p, const char *key, char *value) {
  p->key=key;
  p->value=value;
}

void owl_pair_set_key(owl_pair *p, const char *key) {
  p->key=key;
}

void owl_pair_set_value(owl_pair *p, char *value) {
  p->value=value;
}

const char *owl_pair_get_key(const owl_pair *p) {
  return(p->key);
}

char *owl_pair_get_value(const owl_pair *p) {
  return(p->value);
}
