#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_buddylist_init(owl_buddylist *b)
{
  owl_list_create(&(b->buddies));
  owl_list_create(&(b->idletimes));
  /* owl_list_create(&(g->buddymsg_queue)); */
}

/* Deal with an "oncoming" message.  This means recognizing the user
 * has logged in, and displaying a message if they were not already
 * logged in.
 */
void owl_buddylist_oncoming(owl_buddylist *b, char *screenname)
{
  int *zero;
  owl_message *m;

  if (!owl_buddylist_is_buddy_loggedin(b, screenname)) {

    /* add the buddy */
    owl_list_append_element(&(b->buddies), owl_strdup(screenname));
    zero=owl_malloc(sizeof(int));
    *zero=0;
    owl_list_append_element(&(b->idletimes), zero);

    /* do a request for idle time */
    owl_buddylist_request_idletime(b, screenname);
	
    /* are we ingoring login messages for a while? */
    if (!owl_timer_is_expired(owl_global_get_aim_login_timer(&g))) return;

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
void owl_buddylist_offgoing(owl_buddylist *b, char *screenname)
{
  int index;
  owl_message *m;

  index=owl_buddylist_get_buddy_index(b, screenname);
  if (index==-1) return;

  owl_free(owl_list_get_element(&(b->buddies), index));
  owl_free(owl_list_get_element(&(b->idletimes), index));
  owl_list_remove_element(&(b->buddies), index);
  owl_list_remove_element(&(b->idletimes), index);

  m=owl_malloc(sizeof(owl_message));
  owl_message_create_aim(m,
			 screenname,
			 owl_global_get_aim_screenname(&g),
			 "",
			 OWL_MESSAGE_DIRECTION_IN,
			 -1);
  owl_global_messagequeue_addmsg(&g, m);
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

/* send request to the AIM server to retrieve info on one buddy.  The
 * AIM callback then fills in the values when the responses are
 * received.  The buddy must be logged in or no request will be
 * performed.
 */
void owl_buddylist_request_idletime(owl_buddylist *b, char *screenname)
{
  if (!owl_buddylist_is_buddy_loggedin(b, screenname)) return;
  
  owl_aim_get_idle(screenname);
}

/* return the number of logged in buddies */
int owl_buddylist_get_size(owl_buddylist *b)
{
  return(owl_list_get_size(&(b->buddies)));
}

/* get buddy number 'n' */
char *owl_buddylist_get_buddy(owl_buddylist *b, int n)
{
  if (n > owl_buddylist_get_size(b)-1) return("");
  return(owl_list_get_element(&(b->buddies), n));
}

/* Return the index of the buddy 'screename' or -1
 * if the buddy is not logged in.
 */
int owl_buddylist_get_buddy_index(owl_buddylist *b, char *screenname)
{
  int i, j;
  
  j=owl_list_get_size(&(b->buddies));
  for (i=0; i<j; i++) {
    if (!strcasecmp(owl_list_get_element(&(b->buddies), i), screenname)) {
      return(i);
    }
  }
  return(-1);
}

/* return 1 if the buddy 'screenname' is logged in,
 * otherwise return 0
 */
int owl_buddylist_is_buddy_loggedin(owl_buddylist *b, char *screenname)
{
  if (owl_buddylist_get_buddy_index(b, screenname)!=-1) return(1);
  return(0);
}

/* get the idle time for buddy number 'n' */
int owl_buddylist_get_idletime(owl_buddylist *b, int n)
{
  int *foo;

  foo=owl_list_get_element(&(b->idletimes), n);
  return(*foo);
}

/* Set the idle time for user 'screenname'.  If the given screenname
 * is not on the buddy list do nothing.  If there is a queued request
 * for this screename, remove it from the queue.
 */
void owl_buddylist_set_idletime(owl_buddylist *b, char *screenname, int minutes)
{
  int index, *idle;

  index=owl_buddylist_get_buddy_index(b, screenname);
  if (index==-1) return;

  owl_free(owl_list_get_element(&(b->idletimes), index));
  idle=owl_malloc(sizeof(int));
  *idle=minutes;
  owl_list_replace_element(&(b->idletimes), index, idle);
}

/* remove all buddies from the list */
void owl_buddylist_clear(owl_buddylist *b) {
  owl_list_free_all(&(b->buddies), owl_free);
  owl_list_free_all(&(b->idletimes), owl_free);
  owl_list_create(&(b->buddies));
  owl_list_create(&(b->idletimes));
}
