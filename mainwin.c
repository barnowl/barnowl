#include "owl.h"

void owl_mainwin_init(owl_mainwin *mw)
{
  mw->curtruncated=0;
  mw->lastdisplayed=-1;
}

void owl_mainwin_redisplay(owl_mainwin *mw)
{
  owl_message *m;
  int i, lines, isfull, viewsize;
  int x, y, savey, recwinlines, start;
  int topmsg, curmsg, markedmsgid, fgcolor, bgcolor;
  WINDOW *recwin;
  const owl_view *v;
  GList *fl;
  const owl_filter *f;

  recwin = owl_global_get_curs_recwin(&g);
  topmsg = owl_global_get_topmsg(&g);
  curmsg = owl_global_get_curmsg(&g);
  markedmsgid = owl_global_get_markedmsgid(&g);
  v = owl_global_get_current_view(&g);
  owl_fmtext_reset_colorpairs();

  if (v==NULL) {
    owl_function_debugmsg("Hit a null window in owl_mainwin_redisplay.");
    return;
  }

  werase(recwin);

  recwinlines=owl_global_get_recwin_lines(&g);
  viewsize=owl_view_get_size(v);

  /* if there are no messages or if topmsg is past the end of the messages,
   * just draw a blank screen */
  if (viewsize==0 || topmsg>=viewsize) {
    if (viewsize==0) {
      owl_global_set_topmsg(&g, 0);
    }
    mw->curtruncated=0;
    mw->lastdisplayed=-1;
    update_panels();
    owl_global_set_needrefresh(&g);
    return;
  }

  /* write the messages out */
  isfull=0;
  mw->curtruncated=0;
  mw->lasttruncated=0;

  for (i=topmsg; i<viewsize; i++) {
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
    fgcolor=OWL_COLOR_DEFAULT;
    bgcolor=OWL_COLOR_DEFAULT;
    for (fl = g.filterlist; fl; fl = g_list_next(fl)) {
      f = fl->data;
      if ((owl_filter_get_fgcolor(f)!=OWL_COLOR_DEFAULT) ||
          (owl_filter_get_bgcolor(f)!=OWL_COLOR_DEFAULT)) {
        if (owl_filter_message_match(f, m)) {
          if (owl_filter_get_fgcolor(f)!=OWL_COLOR_DEFAULT) fgcolor=owl_filter_get_fgcolor(f);
          if (owl_filter_get_bgcolor(f)!=OWL_COLOR_DEFAULT) bgcolor=owl_filter_get_bgcolor(f);
	}
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
			       fgcolor, bgcolor);
    } else {
      /* otherwise print the whole thing */
      owl_message_curs_waddstr(m, owl_global_get_curs_recwin(&g),
			       start,
			       start+lines,
			       owl_global_get_rightshift(&g),
			       owl_global_get_cols(&g)+owl_global_get_rightshift(&g)-1,
			       fgcolor, bgcolor);
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
	if (owl_message_is_delete(m)) {
	  waddstr(recwin, "D");
	} else if (markedmsgid == owl_message_get_id(m)) {
	  waddstr(recwin, "*");
	} else {
	  waddstr(recwin, ">");
	}
	wmove(recwin, y, x);
	wattroff(recwin, A_BOLD);
      } else if (owl_message_is_delete(m)) {
	wmove(recwin, savey, 0);
	waddstr(recwin, " D");
	wmove(recwin, y, x);
      } else if (markedmsgid == owl_message_get_id(m)) {
	wmove(recwin, savey, 0);
	waddstr(recwin, " *");
	wmove(recwin, y, x);
      }
    }
    wattroff(recwin, A_BOLD);
  }
  mw->lastdisplayed=i-1;

  update_panels();
  owl_global_set_needrefresh(&g);
}


int owl_mainwin_is_curmsg_truncated(const owl_mainwin *mw)
{
  if (mw->curtruncated) return(1);
  return(0);
}

int owl_mainwin_is_last_msg_truncated(const owl_mainwin *mw)
{
  if (mw->lasttruncated) return(1);
  return(0);
}

int owl_mainwin_get_last_msg(const owl_mainwin *mw)
{
  /* return the number of the last message displayed. -1 if none */
  return(mw->lastdisplayed);
}
