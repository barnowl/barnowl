#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_buddylist_init(owl_buddylist *b)
{
  owl_list_create(&(b->buddies));
  owl_list_create(&(b->idletimes));
}

/* Deal with an "oncoming" message.  This means recognizing the user
 * has logged in, and sending a message if they were not already
 * logged in.
 */
void owl_buddylist_oncoming(owl_buddylist *b, char *screenname)
{
  int i, j, found;
  owl_message *m;
  int *zero;

  found=0;
  j=owl_list_get_size(&(b->buddies));
  for (i=0; i<j; i++) {
    if (!strcasecmp(owl_list_get_element(&(b->buddies), i), screenname)) {
      found=1;
      break;
    }
  }

  if (!found) {
    /* add the buddy */
    owl_list_append_element(&(b->buddies), owl_strdup(screenname));
    zero=owl_malloc(sizeof(int));
    *zero=0;
    owl_list_append_element(&(b->idletimes), zero);

    /* do a request for idle time */
    owl_buddylist_request_idletimes(owl_global_get_buddylist(&g));
	
    /* are we ingoring login messages for a while? */
    if (!owl_timer_is_expired(owl_global_get_aim_login_timer(&g))) return;

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
      owl_free(owl_list_get_element(&(b->idletimes), i));
      owl_list_remove_element(&(b->buddies), i);
      owl_list_remove_element(&(b->idletimes), i);
      break;
    }
  }

  if (found) {
    m=owl_malloc(sizeof(owl_message));
    owl_message_create_aim(m,
			   screenname,
			   owl_global_get_aim_screenname(&g),
			   "",
			   OWL_MESSAGE_DIRECTION_IN,
			   -1);

    owl_global_messagequeue_addmsg(&g, m);
  }
}

/* send requests to the AIM server to retrieve info
 * on all buddies.  The AIM callback then fills in the
 * values when the responses are received
 */
void owl_buddylist_request_idletimes(owl_buddylist *b)
{
  int i, j;

  j=owl_buddylist_get_size(b);
  for (i=0; i<j; i++) {
    owl_aim_get_idle(owl_buddylist_get_buddy(b, i));
  }
}

/* return the number of logged in buddies */
int owl_buddylist_get_size(owl_buddylist *b)
{
  return(owl_list_get_size(&(b->buddies)));
}

/* get buddy number 'n' */
char *owl_buddylist_get_buddy(owl_buddylist *b, int n)
{
  return(owl_list_get_element(&(b->buddies), n));
}

/* get the idle time for buddy number 'n' */
int owl_buddylist_get_idletime(owl_buddylist *b, int n)
{
  int *foo;

  foo=owl_list_get_element(&(b->idletimes), n);
  return(*foo);
}

/* set the idle time for user 'screenname'.  If the given
 * screenname is not on the buddy list do nothing
 */
void owl_buddylist_set_idletime(owl_buddylist *b, char *screenname, int seconds)
{
  int i, j, *idle;

  j=owl_buddylist_get_size(b);
  for (i=0; i<j; i++) {
    if (!strcasecmp(owl_list_get_element(&(b->buddies), i), screenname)) {
      owl_free(owl_list_get_element(&(b->idletimes), i));
      idle=owl_malloc(sizeof(int));
      *idle=seconds;
      owl_list_replace_element(&(b->idletimes), i, idle);
      return;
    }
  }
}

/* remove all buddies from the list */
void owl_buddylist_clear(owl_buddylist *b) {
  owl_list_free_all(&(b->buddies), owl_free);
  owl_list_free_all(&(b->idletimes), owl_free);
  owl_list_create(&(b->buddies));
  owl_list_create(&(b->idletimes));
}
