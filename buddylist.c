#include "owl.h"

void owl_buddylist_init(owl_buddylist *b)
{
  owl_list_create(&(b->buddies));
}

/* Deal with an "oncoming" message.  This means recognizing the user
 * has logged in, and sending a message if they were not already
 * logged in.
 */
void owl_buddylist_oncoming(owl_buddylist *b, char *screenname)
{
  int i, j, found;
  owl_message *m;

  found=0;
  j=owl_list_get_size(&(b->buddies));
  for (i=0; i<j; i++) {
    if (!strcasecmp(owl_list_get_element(&(b->buddies), i), screenname)) {
      found=1;
      break;
    }
  }

  if (!found) {
    owl_list_append_element(&(b->buddies), owl_strdup(screenname));

    m=owl_malloc(sizeof(owl_message));
    owl_message_create_aim_login(m, 0, screenname);
    owl_global_messagequeue_addmsg(&g, m);
  }
}
    


/* Deal with an "offgoing" message.  This means recognizing the user
 * has logged out, and sending a message if they were logged in.
 */
void owl_buddylist_offgoing(owl_buddylist *b, char *screenname)
{
  int i, j, found;
  owl_message *m;

  found=0;
  j=owl_list_get_size(&(b->buddies));
  for (i=0; i<j; i++) {
    if (!strcasecmp(owl_list_get_element(&(b->buddies), i), screenname)) {
      found=1;
      owl_free(owl_list_get_element(&(b->buddies), i));
      owl_list_remove_element(&(b->buddies), i);
      break;
    }
  }

  if (found) {
    m=owl_malloc(sizeof(owl_message));
    owl_message_create_aim_login(m, 1, screenname);
    owl_global_messagequeue_addmsg(&g, m);
  }
}

int owl_buddylist_get_size(owl_buddylist *b)
{
  return(owl_list_get_size(&(b->buddies)));
}

char *owl_buddylist_get_buddy(owl_buddylist *b, int n)
{
  return(owl_list_get_element(&(b->buddies), n));
}

void owl_buddylist_clear(owl_buddylist *b) {
  int i, j;

  owl_list_free_all(&(b->buddies), owl_free);
  owl_list_create(&(b->buddies));
}
