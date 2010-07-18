#include "owl.h"

void owl_mainpanel_init(owl_mainpanel *mp)
{
  /* Create windows */
  mp->panel = owl_window_new(NULL);
  /* HACK for now: the sepwin must be drawn /after/ the recwin for
   * lastdisplayed to work */
  mp->sepwin = owl_window_new(mp->panel);
  mp->recwin = owl_window_new(mp->panel);
  mp->msgwin = owl_window_new(mp->panel);
  mp->typwin = owl_window_new(mp->panel);

  /* Set up sizing hooks */
  owl_signal_connect_object(owl_window_get_screen(), "resized", G_CALLBACK(owl_window_fill_parent_cb), mp->panel, 0);
  g_signal_connect_swapped(mp->panel, "resized", G_CALLBACK(owl_mainpanel_layout_contents), mp);

  /* Bootstrap the sizes and go */
  owl_window_fill_parent_cb(owl_window_get_screen(), mp->panel);
  owl_window_show_all(mp->panel);
}

void owl_mainpanel_layout_contents(owl_mainpanel *mp)
{
  int lines, cols, typwin_lines;

  /* skip if we haven't been initialized */
  if (!mp->panel) return;

  owl_window_get_position(mp->panel, &lines, &cols, NULL, NULL);
  typwin_lines = owl_global_get_typwin_lines(&g);

  /* set the new window sizes */
  mp->recwinlines=lines-(typwin_lines+2);
  if (mp->recwinlines<0) {
    /* gotta deal with this */
    mp->recwinlines=0;
    typwin_lines = lines - 2;
  }

  /* resize all the windows */
  owl_window_set_position(mp->recwin, mp->recwinlines, cols, 0, 0);
  owl_window_set_position(mp->sepwin, 1, cols, mp->recwinlines, 0);
  owl_window_set_position(mp->msgwin, 1, cols, mp->recwinlines+1, 0);
  owl_window_set_position(mp->typwin, typwin_lines, cols, mp->recwinlines+2, 0);
}

void owl_mainpanel_cleanup(owl_mainpanel *mp)
{
  owl_window_unlink(mp->panel);
  g_object_unref(mp->panel);
  g_object_unref(mp->recwin);
  g_object_unref(mp->sepwin);
  g_object_unref(mp->msgwin);
  g_object_unref(mp->typwin);
  mp->panel = mp->recwin = mp->sepwin = mp->msgwin = mp->typwin = NULL;
}
