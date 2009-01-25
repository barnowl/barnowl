#include "owl.h"

void owl_pair_create(owl_pair *p, char *key, char *value) {
  p->key=key;
  p->value=value;
}

void owl_pair_set_key(owl_pair *p, char *key) {
  p->key=key;
}

void owl_pair_set_value(owl_pair *p, char *value) {
  p->value=value;
}

char *owl_pair_get_key(owl_pair *p) {
  return(p->key);
}

char *owl_pair_get_value(owl_pair *p) {
  return(p->value);
}
