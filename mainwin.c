#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_mainwin_init(owl_mainwin *mw) {
  mw->curtruncated=0;
  mw->lastdisplayed=-1;
}

void owl_mainwin_redisplay(owl_mainwin *mw) {
  owl_message *m;
  int i, j, p, q, lines, isfull;
  int x, y, savey, recwinlines, start;
  int topmsg, curmsg, color;
  WINDOW *recwin;
  owl_view *v;
  owl_list *filtlist;
  owl_filter *f;

  recwin=owl_global_get_curs_recwin(&g);
  topmsg=owl_global_get_topmsg(&g);
  curmsg=owl_global_get_curmsg(&g);
  v=owl_global_get_current_view(&g);

  werase(recwin);

  recwinlines=owl_global_get_recwin_lines(&g);
  topmsg=owl_global_get_topmsg(&g);

  /* if there are no messages, just draw a blank screen */
  if (owl_view_get_size(v)==0) {
    owl_global_set_topmsg(&g, 0);
    mw->curtruncated=0;
    mw->lastdisplayed=-1;
    wnoutrefresh(recwin);
    owl_global_set_needrefresh(&g);
    return;
  }

  /* write the messages out */
  isfull=0;
  mw->curtruncated=0;
  mw->lasttruncated=0;
  j=owl_view_get_size(v);
  for (i=topmsg; i<j; i++) {
    if (isfull) break;
    m=owl_view_get_element(v, i);

    /* hold on to y in case this is the current message or deleted */
    getyx(recwin, y, x);
    savey=y;

    /* if it's the current message, account for a vert_offset */
    if (i==owl_global_get_curmsg(&g)) {
      start=owl_global_get_curmsg_vert_offset(&g);
      lines=owl_message_get_numlines(m)-start;
    } else {
      start=0;
      lines=owl_message_get_numlines(m);
    }

    /* if we match filters set the color */
    color=OWL_COLOR_DEFAULT;
    filtlist=owl_global_get_filterlist(&g);
    q=owl_list_get_size(filtlist);
    for (p=0; p<q; p++) {
      f=owl_list_get_element(filtlist, p);
      if (owl_filter_message_match(f, m)) {
	if (owl_filter_get_color(f)!=OWL_COLOR_DEFAULT) color=owl_filter_get_color(f);
      }
    }

    /* if we'll fill the screen print a partial message */
    if ((y+lines > recwinlines) && (i==owl_global_get_curmsg(&g))) mw->curtruncated=1;
    if (y+lines > recwinlines) mw->lasttruncated=1;
    if (y+lines > recwinlines-1) {
      isfull=1;
      owl_message_curs_waddstr(m, owl_global_get_curs_recwin(&g),
			       start,
			       start+recwinlines-y,
			       owl_global_get_rightshift(&g),
			       owl_global_get_cols(&g)+owl_global_get_rightshift(&g)-1,
			       color);
    } else {
      /* otherwise print the whole thing */
      owl_message_curs_waddstr(m, owl_global_get_curs_recwin(&g),
			       start,
			       start+lines,
			       owl_global_get_rightshift(&g),
			       owl_global_get_cols(&g)+owl_global_get_rightshift(&g)-1,
			       color);
    }


    /* is it the current message and/or deleted? */
    getyx(recwin, y, x);
    wattrset(recwin, A_NORMAL);
    if (owl_global_get_rightshift(&g)==0) {   /* this lame and should be fixed */
      if (m==owl_view_get_element(v, curmsg)) {
	wmove(recwin, savey, 0);
	wattron(recwin, A_BOLD);	
	if (owl_global_get_curmsg_vert_offset(&g)>0) {
	  waddstr(recwin, "+");
	} else {
	  waddstr(recwin, "-");
	}
	if (!owl_message_is_delete(m)) {
	  waddstr(recwin, ">");
	} else {
	  waddstr(recwin, "D");
	}
	wmove(recwin, y, x);
	wattroff(recwin, A_BOLD);
      } else if (owl_message_is_delete(m)) {
	wmove(recwin, savey, 0);
	waddstr(recwin, " D");
	wmove(recwin, y, x);
      }
    }
    wattroff(recwin, A_BOLD);
  }
  mw->lastdisplayed=i-1;

  wnoutrefresh(recwin);
  owl_global_set_needrefresh(&g);
}


int owl_mainwin_is_curmsg_truncated(owl_mainwin *mw) {
  if (mw->curtruncated) return(1);
  return(0);
}

int owl_mainwin_is_last_msg_truncated(owl_mainwin *mw) {
  if (mw->lasttruncated) return(1);
  return(0);
}

int owl_mainwin_get_last_msg(owl_mainwin *mw) {
  /* return the number of the last message displayed. -1 if none */
  return(mw->lastdisplayed);
}
