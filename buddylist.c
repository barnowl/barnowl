#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_buddylist_init(owl_buddylist *bl)
{
  owl_list_create(&(bl->buddies));
}

/* add a (logged-in) AIM buddy to the buddy list
 */
void owl_buddylist_add_aim_buddy(owl_buddylist *bl, char *screenname)
{
  owl_buddy *b;
  b=owl_malloc(sizeof(owl_buddy));
  
  owl_buddy_create(b, OWL_PROTOCOL_AIM, screenname);
  owl_list_append_element(&(bl->buddies), b);
}

/* remove an AIM buddy from the buddy list
 */
int owl_buddylist_remove_aim_buddy(owl_buddylist *bl, char *name)
{
  int i, j;
  owl_buddy *b;

  j=owl_list_get_size(&(bl->buddies));
  for (i=0; i<j; i++) {
    b=owl_list_get_element(&(bl->buddies), i);
    if (!strcasecmp(name, owl_buddy_get_name(b)) && owl_buddy_is_proto_aim(b)) {
      owl_list_remove_element(&(bl->buddies), i);
      owl_buddy_free(b);
      return(0);
    }
  }
  return(1);
}

/* Deal with an "oncoming" message.  This means recognizing the user
 * has logged in, and displaying a message if they were not already
 * logged in.
 */
void owl_buddylist_oncoming(owl_buddylist *bl, char *screenname)
{
  owl_message *m;

  if (!owl_buddylist_is_aim_buddy_loggedin(bl, screenname)) {

    owl_buddylist_add_aim_buddy(bl, screenname);

    /* are we ingoring login messages for a while? */
    if (owl_global_is_ignore_aimlogin(&g)) return;

    /* if not, create the login message */
    m=owl_malloc(sizeof(owl_message));
    owl_message_create_aim(m,
			   screenname,
			   owl_global_get_aim_screenname(&g),
			   "",
			   OWL_MESSAGE_DIRECTION_IN,
			   1);
    owl_global_messagequeue_addmsg(&g, m);
  }
}

/* Deal with an "offgoing" message.  This means recognizing the user
 * has logged out, and sending a message if they were logged in.
 */
void owl_buddylist_offgoing(owl_buddylist *bl, char *screenname)
{
  owl_message *m;

  if (owl_buddylist_is_aim_buddy_loggedin(bl, screenname)) {
    m=owl_malloc(sizeof(owl_message));
    owl_message_create_aim(m,
			   screenname,
			   owl_global_get_aim_screenname(&g),
			   "",
			   OWL_MESSAGE_DIRECTION_IN,
			   -1);
    owl_global_messagequeue_addmsg(&g, m);
  }

  owl_buddylist_remove_aim_buddy(bl, screenname);
}

/* return the number of logged in buddies */
int owl_buddylist_get_size(owl_buddylist *bl)
{
  return(owl_list_get_size(&(bl->buddies)));
}

/* return the buddy with index N.  If out of range, return NULL
 */
owl_buddy *owl_buddylist_get_buddy_n(owl_buddylist *bl, int index)
{
  if (index<0) return(NULL);
  if (index>(owl_buddylist_get_size(bl)-1)) return(NULL);

  return(owl_list_get_element(&(bl->buddies), index));
}

/* return the AIM buddy with screenname 'name'.  If
 * no such buddy is logged in, return NULL.
 */
owl_buddy *owl_buddylist_get_aim_buddy(owl_buddylist *bl, char *name)
{
  int i, j;
  owl_buddy *b;

  j=owl_list_get_size(&(bl->buddies));
  for (i=0; i<j; i++) {
    b=owl_list_get_element(&(bl->buddies), i);
    if (!strcasecmp(name, owl_buddy_get_name(b))) return(b);
  }
  return(NULL);
}

/* return 1 if the buddy 'screenname' is logged in,
 * otherwise return 0
 */
int owl_buddylist_is_aim_buddy_loggedin(owl_buddylist *bl, char *screenname)
{
  owl_buddy *b;

  b=owl_buddylist_get_aim_buddy(bl, screenname);
  if (b==NULL) return(0);
  return(1);
}

/* remove all buddies from the list */
void owl_buddylist_clear(owl_buddylist *bl)
{
  owl_list_free_all(&(bl->buddies), (void(*)(void*))owl_buddy_free);
  owl_list_create(&(bl->buddies));
}

void owl_buddylist_free(owl_buddylist *bl)
{
  owl_list_free_all(&(bl->buddies), (void(*)(void*))owl_buddy_free);
}
