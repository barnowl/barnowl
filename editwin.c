#include "owl.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

static const char fileIdent[] = "$Id$";

struct _owl_editwin { /*noproto*/
  char *buff;
  owl_history *hist;
  int bufflen;
  int allocated;
  int index;
  int goal_column;
  int topindex;
  int winlines, wincols, fillcol, wrapcol;
  WINDOW *curswin;
  int style;
  int lock;
  int dotsend;
  int echochar;

  char *command;
  void (*callback)(struct _owl_editwin*);
  void *cbdata;
};

typedef struct { /*noproto*/
  int index;
  int goal_column;
  int lock;
} oe_excursion;

static void oe_reframe(owl_editwin *e);
static void oe_save_excursion(owl_editwin *e, oe_excursion *x);
static void oe_restore_excursion(owl_editwin *e, oe_excursion *x);
static int oe_find_display_line(owl_editwin *e, int *x, int index);
static int owl_editwin_limit_maxcols(int v, int maxv);
static int owl_editwin_check_dotsend(owl_editwin *e);
static int owl_editwin_is_char_in(owl_editwin *e, char *set);
static gunichar owl_editwin_get_char_at_point(owl_editwin *e);
static void owl_editwin_replace(owl_editwin *e, int count, char *s);
static int oe_count_glyphs(char *s);

#define INCR 4096

#define WHITESPACE " \n\t"

owl_editwin *owl_editwin_allocate(void)
{
  return owl_malloc(sizeof(owl_editwin));
}

static int oe_count_glyphs(char *s)
{
  int count = 0;
  char *p;

  for(p = s; *p != 0; p = g_utf8_find_next_char(p, NULL))
    if (!g_unichar_ismark(g_utf8_get_char(p)))
      count++;

  return count;
}

static inline void oe_set_index(owl_editwin *e, int index)
{
  e->index = index;
  e->goal_column = -1;
}

/* initialize the editwin e.
 * 'win' is an already initialzed curses window that will be used by editwin
 */
void owl_editwin_init(owl_editwin *e, WINDOW *win, int winlines, int wincols, int style, owl_history *hist)
{
  e->buff=owl_malloc(INCR);
  e->buff[0]='\0';
  e->bufflen=0;
  e->hist=hist;
  e->allocated=INCR;
  oe_set_index(e, 0);
  e->goal_column = -1;
  e->topindex=0;
  e->winlines=winlines;
  e->wincols=wincols;
  e->fillcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxfillcols(&g));
  e->wrapcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxwrapcols(&g));
  e->curswin=win;
  e->style=style;
  if ((style!=OWL_EDITWIN_STYLE_MULTILINE) &&
      (style!=OWL_EDITWIN_STYLE_ONELINE)) {
    e->style=OWL_EDITWIN_STYLE_MULTILINE;
  }
  e->lock=0;
  e->dotsend=0;
  e->echochar='\0';

  /* We get initialized multiple times, but we need to hold on to
     the callbacks, so we can't NULL them here. */
  /*
    e->command = NULL;
    e->callback = NULL;
    e->cbdata = NULL;
  */
  if (win) werase(win);
}

void owl_editwin_set_curswin(owl_editwin *e, WINDOW *w, int winlines, int wincols)
{
  e->curswin=w;
  e->winlines=winlines;
  e->wincols=wincols;
  e->fillcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxfillcols(&g));
  e->wrapcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxwrapcols(&g));
}

/* echo the character 'ch' for each normal character keystroke,
 * excepting locktext.  This is useful for entering passwords etc.  If
 * ch=='\0' characters are echo'd normally
 */
void owl_editwin_set_echochar(owl_editwin *e, int ch)
{
  e->echochar=ch;
}

WINDOW *owl_editwin_get_curswin(owl_editwin *e)
{
  return(e->curswin);
}

owl_history *owl_editwin_get_history(owl_editwin *e)
{
  return(e->hist);
}

void owl_editwin_set_dotsend(owl_editwin *e)
{
  e->dotsend=1;
}

void owl_editwin_set_command(owl_editwin *e, char *command)
{
  if(e->command) owl_free(e->command);
  e->command = owl_strdup(command);
}

char *owl_editwin_get_command(owl_editwin *e)
{
  if(e->command) return e->command;
  return "";
}

void owl_editwin_set_callback(owl_editwin *e, void (*cb)(owl_editwin*))
{
  e->callback = cb;
}

void (*owl_editwin_get_callback(owl_editwin *e))(owl_editwin*)
{
  return e->callback;
}

void owl_editwin_set_cbdata(owl_editwin *e, void *data)
{
  e->cbdata = data;
}

void *owl_editwin_get_cbdata(owl_editwin *e) {
  return e->cbdata;
}

void owl_editwin_do_callback(owl_editwin *e) {
  void (*cb)(owl_editwin*);
  cb=owl_editwin_get_callback(e);
  if(!cb) {
    owl_function_error("Internal error: No editwin callback!");
  } else {
    /* owl_function_error("text: |%s|", owl_editwin_get_text(e)); */
    cb(e);
  }
}

static int owl_editwin_limit_maxcols(int v, int maxv)
{
  if (maxv > 5 && v > maxv) {
    return(maxv);
  } else {
    return(v);
  }
}

/* set text to be 'locked in' at the beginning of the buffer, any
 * previous text (including locked text) will be overwritten
 */
void owl_editwin_set_locktext(owl_editwin *e, char *text)
{
  oe_set_index(e, 0);
  e->lock = 0;
  owl_editwin_replace(e, e->bufflen, text);
  e->buff[e->bufflen] = 0;
  e->lock=e->bufflen;
  oe_set_index(e, e->lock);
  owl_editwin_redisplay(e, 0);
}

int owl_editwin_get_style(owl_editwin *e)
{
  return(e->style);
}

void owl_editwin_new_style(owl_editwin *e, int newstyle, owl_history *h)
{
  e->hist = h;

  if (e->style==newstyle) return;

  if (newstyle==OWL_EDITWIN_STYLE_MULTILINE) {
    e->style=newstyle;
  } else if (newstyle==OWL_EDITWIN_STYLE_ONELINE) {
    e->style=newstyle;

    /* nuke everything after the first line */
    owl_editwin_move_to_top(e);
    owl_editwin_move_to_end_of_line(e);
    owl_editwin_replace(e, oe_count_glyphs(e->buff + e->index),  "");
  }
}

/* completly reinitialize the buffer */
void owl_editwin_fullclear(owl_editwin *e)
{
  owl_free(e->buff);
  owl_editwin_init(e, e->curswin, e->winlines, e->wincols, e->style, e->hist);
}

/* clear all text except for locktext and put the cursor at the
 * beginning
 */
void owl_editwin_clear(owl_editwin *e)
{

  int lock = e->lock;
  int dotsend=e->dotsend;
  char *locktext=NULL;
  char echochar=e->echochar;

  if (lock > 0) {
    locktext = owl_malloc(lock+1);
    strncpy(locktext, e->buff, lock);
    locktext[lock] = 0;
  }

  owl_free(e->buff);
  owl_editwin_init(e, e->curswin, e->winlines, e->wincols, e->style, e->hist);

  if (lock > 0) {
    owl_editwin_set_locktext(e, locktext);
  }
  if (dotsend) {
    owl_editwin_set_dotsend(e);
  }
  if (echochar) {
    owl_editwin_set_echochar(e, echochar);
  }

  if (locktext)
    owl_free(locktext);

  oe_set_index(e, lock);
}

void owl_editwin_recenter(owl_editwin *e)
{
  e->topindex = -1;
}

static void oe_save_excursion(owl_editwin *e, oe_excursion *x)
{
  x->index = e->index;
  x->goal_column = e->goal_column;
  x->lock = e->lock;
}

static void oe_restore_excursion(owl_editwin *e, oe_excursion *x)
{
  oe_set_index(e, x->index);
  e->goal_column = x->goal_column;
  e->lock = x->lock;
}

static inline char *oe_next_point(owl_editwin *e, char *p)
{
  char *boundary = e->buff + e->bufflen + 1;
  char *q;

  q = g_utf8_find_next_char(p, boundary);
  while (q && g_unichar_ismark(g_utf8_get_char(q)))
    q = g_utf8_find_next_char(q, boundary);

  if (q == p)
    return NULL;
  return q;
}

static inline char *oe_prev_point(owl_editwin *e, char *p)
{
  char *boundary = e->buff + e->lock;

  p = g_utf8_find_prev_char(boundary, p);
  while (p && g_unichar_ismark(g_utf8_get_char(p)))
    p = g_utf8_find_prev_char(boundary, p);

  return p;
}

static int oe_find_display_line(owl_editwin *e, int *x, int index)
{
  int width = 0, cw;
  gunichar c = -1;
  char *p;

  while(1) {
    /* note the position of the dot */
    if (x != NULL && index == e->index && width < e->wincols)
      *x = width;

    /* get the current character */
    c = g_utf8_get_char(e->buff + index);

    /* figure out how wide it is */
    if (c == 9) /* TAB */
      cw = TABSIZE - width % TABSIZE;
    else
      cw = mk_wcwidth(c);
    if (cw < 0) /* control characters */
	cw = 0;

    if (width + cw > e->wincols) {
      if (x != NULL && *x == width)
	*x = -1;
      break;
    }
    width += cw;

    if (c == '\n') {
      if (width < e->wincols)
	++index; /* skip the newline */
      break;
    }

    /* find the next character */
    p = oe_next_point(e, e->buff + index);
    if (p == NULL) { /* we ran off the end */
      if (x != NULL && e->index > index)
	*x = width + 1;
      break;
    }
    index = p - e->buff;

  }
  return index;
}

static void oe_reframe(owl_editwin *e) {
  oe_excursion x;
  int goal = e->winlines / 2;
  int index;
  int count = 0;
  int point;
  int n, i;
  int last;

  oe_save_excursion(e, &x);
  /* step back line-by-line through the buffer until we have >= goal lines of
     display text */
  e->lock = 0; /* we can (must) tread on the locktext */

  point = e->index;
  last = -1;
  while (count < goal) {
    index = e->index;
    owl_editwin_move_to_beginning_of_line(e);
    if (last == e->index)
      break;
    last = e->index;
    for (n = 0, i = e->index; i < index; n++)
      i = oe_find_display_line(e, NULL, i);
    count += n == 0 ? 1 : n;
    if (count < goal)
      owl_editwin_point_move(e, -1);
  }

  e->topindex = e->index;
  /* if we overshot, backtrack */
  for (n = 0; n < (count - goal); n++)
    e->topindex = oe_find_display_line(e, NULL, e->topindex);

  oe_restore_excursion(e, &x);
}

/* regenerate the text on the curses window */
/* if update == 1 then do a doupdate(), otherwise do not */
void owl_editwin_redisplay(owl_editwin *e, int update)
{
  int x = -1, y = -1, t;
  int line, index, lineindex, times = 0;

  do {
    werase(e->curswin);

    if (e->topindex == -1 || e->index < e->topindex)
      oe_reframe(e);

    line = 0;
    index = e->topindex;
    while(line < e->winlines) {
      lineindex = index;
      t = -1;
      index = oe_find_display_line(e, &t, lineindex);
      if (x == -1 && t != -1)
	x = t, y = line;
      if (index - lineindex)
	mvwaddnstr(e->curswin, line, 0,
		   e->buff + lineindex,
		   index - lineindex);
      line++;
    }
    if (x == -1)
	e->topindex = -1; /* force a reframe */
    times++;
  } while(x == -1 && times < 3);

  wmove(e->curswin, y, x);
  wnoutrefresh(e->curswin);
  if (update == 1)
    doupdate();
}

static void owl_editwin_replace(owl_editwin *e, int replace, char *s)
{ /* replace count characters at the point with s */
  int start, end, i, free, need, size;
  char *p;

  if (!g_utf8_validate(s, -1, NULL)) {
    owl_function_debugmsg("owl_editwin_insert_string: received non-utf-8 string.");
    return;
  }

  start = e->index;
  for (i = 0, p = e->buff + start; i < replace && p != NULL; i++)
    p = oe_next_point(e, p);
  if (p != NULL)
    end = p - e->buff;
  else
    end = e->bufflen;

  free = e->allocated - e->bufflen + end - start;

  need = strlen(s) - free;
  if (need > 0) {
    size = e->allocated + need + INCR - (need % INCR);
    p = realloc(e->buff, size);
    if (p == NULL) {
      /* XXX signal impending doom somehow and don't do anything */
      return;
    }
    e->buff = p;
    e->allocated = size;
  }

  memmove(e->buff + start + strlen(s), e->buff + end, e->bufflen + 1 - end);
  memcpy(e->buff + start, s, strlen(s));
  e->bufflen += start - end + strlen(s);
  e->index += strlen(s);
}

#if 0 /*XXX*/
/* linewrap the word just before the cursor.
 * returns 0 on success
 * returns -1 if we could not wrap.
 */
static int _owl_editwin_linewrap_word(owl_editwin *e)
{
  int i;
  char *ptr1, *start;
  gunichar c;

  start = e->buff + e->lock;

  ptr1 = e->buff + e->index;
  ptr1 = g_utf8_find_prev_char(start, ptr1);

  while (ptr1) {
    c = g_utf8_get_char(ptr1);
    if (owl_util_can_break_after(c)) {
      if (c != ' ') {
        i = ptr1 - e->buff;
	oe_set_index(e, i);
        _owl_editwin_insert_bytes(e, 1);
        /* _owl_editwin_insert_bytes may move e->buff. */
        ptr1 = e->buff + i;
      }
      *ptr1 = '\n';
      return 0;
    }
    else if (c == '\n') {
      return 0;
    }
    ptr1 = g_utf8_find_prev_char(start, ptr1);
  }
  return -1;
}
#endif

/* delete the character at the current point, following chars
 * shift left.
 */
void owl_editwin_delete_char(owl_editwin *e)
{
  owl_editwin_replace(e, 1, "");
}

/* Swap the character at point with the character at point-1 and
 * advance the pointer.  If point is at beginning of buffer do
 * nothing.  If point is after the last character swap point-1 with
 * point-2.  (Behaves as observed in tcsh and emacs).
 */
void owl_editwin_transpose_chars(owl_editwin *e)
{
  char *middle, *end, *start, *tmp;

  if (e->bufflen == 0) return;

  if (e->index == e->bufflen)
    owl_editwin_point_move(e, -1);     /* point is after last character */

  if (owl_editwin_at_beginning_of_buffer(e))
    return;     /* point is at beginning of buffer, do nothing */

  /* Transpose two utf-8 unicode glyphs. */
  middle = e->buff + e->index;

  end = oe_next_point(e, middle);
  if (end == NULL)
    return;

  start = oe_prev_point(e, middle);
  if (start == NULL)
    return;

  tmp = owl_malloc((end - start) + 1);
  tmp[(end - start)] = 0;
  memcpy(tmp, middle, end - middle);
  memcpy(tmp + (end - middle), start, middle - start);

  owl_editwin_point_move(e, -1);
  owl_editwin_replace(e, 2, tmp);
}

/* insert 'string' at the current point, later text is shifted
 * right
 */
void owl_editwin_insert_string(owl_editwin *e, char *s)
{
  owl_editwin_replace(e, 0, s);
}

/* We assume index is not set to point to a mid-char */
static gunichar owl_editwin_get_char_at_point(owl_editwin *e)
{
  return g_utf8_get_char(e->buff + e->index);
}

int owl_editwin_point_move(owl_editwin *e, int delta)
{
  char *p;
  int change, d = 0;

  change = MAX(delta, - delta);
  p = e->buff + e->index;

  while (d < change && p != NULL) {
    if (delta > 0)
      p = oe_next_point(e, p);
    else
      p = oe_prev_point(e, p);
    if (p != NULL) {
      oe_set_index(e, p - e->buff);
      d++;
    }
  }

  return delta > 0 ? d : -d;
}

int owl_editwin_at_beginning_of_buffer(owl_editwin *e) {
  if (e->index == e->lock)
    return 1;

  return 0;
}

int owl_at_end_of_buffer(owl_editwin *e) {
  if (e->index == e->bufflen)
    return 1;

  return 0;
}

int owl_editwin_at_beginning_of_line(owl_editwin *e) /*noproto*/
{
  oe_excursion x;
  int ret;

  if (owl_editwin_at_beginning_of_buffer(e))
    return 1;

  oe_save_excursion(e, &x);
  owl_editwin_point_move(e, -1);
  ret = (owl_editwin_get_char_at_point(e) == '\n');
  oe_restore_excursion(e, &x);

  return ret;
}

static int owl_editwin_is_char_in(owl_editwin *e, char *set)
{
  char *p;

  for (p = set; *p != 0; p = g_utf8_find_next_char(p, NULL))
    if (owl_editwin_get_char_at_point(e) == g_utf8_get_char(p))
      return 1;
  return 0;
}

int owl_editwin_move_if_in(owl_editwin *e, int delta, char *set)
{
  int change, distance = 0;
  while (owl_editwin_is_char_in(e, set)) {
    change = owl_editwin_point_move(e, delta);
    distance += change;
    if (change == 0)
      break;
  }
  return distance;
}

int owl_editwin_move_if_not_in(owl_editwin *e, int delta, char *set)
{
  int change, distance = 0;
  while (!owl_editwin_is_char_in(e, set)) {
    change = owl_editwin_point_move(e, delta);
    distance += change;
    if (change == 0)
      break;
  }
  return distance;
}

int owl_editwin_move_to_beginning_of_line(owl_editwin *e)
{
  int distance = 0;

  if (!owl_editwin_at_beginning_of_line(e)) {
    /* move off the \n if were at the end of a line */
    distance += owl_editwin_point_move(e, -1);
    distance += owl_editwin_move_if_not_in(e, -1, "\n");
    if (distance && !owl_editwin_at_beginning_of_buffer(e))
      distance += owl_editwin_point_move(e, 1);
  }
  e->goal_column = 0; /* subtleties */

  return distance;
}

int owl_editwin_move_to_end_of_line(owl_editwin *e)
{
  return owl_editwin_move_if_not_in(e, 1, "\n");
}

int owl_editwin_line_move(owl_editwin *e, int delta)
{
  int goal_column, change, ll, distance;
  int count = 0;

  change = MAX(delta, -delta);

  goal_column = e->goal_column;
  distance = owl_editwin_move_to_beginning_of_line(e);
  goal_column = goal_column == -1 ? -distance : goal_column;

  while(count < change) {
    if (delta > 0) {
      distance += owl_editwin_move_if_not_in(e, 1, "\n");
      distance += owl_editwin_point_move(e, 1);
    } else {
      /* I really want to assert delta < 0 here */
      distance += owl_editwin_point_move(e, -1); /* to the newline on
						    the previous line */
      distance += owl_editwin_move_to_beginning_of_line(e);
    }
    count++;
  }

  distance += (ll = owl_editwin_move_to_end_of_line(e));
  if (ll > goal_column)
    distance += owl_editwin_point_move(e, goal_column - ll);

  e->goal_column = goal_column;

  return distance;
}

void owl_editwin_backspace(owl_editwin *e)
{
  /* delete the char before the current one
   * and shift later chars left
   */
  if(owl_editwin_point_move(e, -1))
    owl_editwin_delete_char(e);
}

void owl_editwin_key_up(owl_editwin *e)
{
  owl_editwin_line_move(e, -1);
}

void owl_editwin_key_down(owl_editwin *e)
{
  owl_editwin_line_move(e, 1);
}

void owl_editwin_key_left(owl_editwin *e)
{
  owl_editwin_point_move(e, -1);
}

void owl_editwin_key_right(owl_editwin *e)
{
  owl_editwin_point_move(e, 1);
}

int owl_editwin_forward_word(owl_editwin *e)
{
  int distance;
  /* if we're starting on a space, find the first non-space */
  distance = owl_editwin_move_if_in(e, 1, WHITESPACE);

  /* now find the end of this word */
  distance += owl_editwin_move_if_not_in(e, 1, WHITESPACE);

  return distance;
}

void owl_editwin_move_to_nextword(owl_editwin *e)
{
  owl_editwin_forward_word(e);
}

/* go backwards to the last non-space character
 */
int owl_editwin_backward_word(owl_editwin *e)
{
  oe_excursion x;
  int distance = 0;
  int further = 0;
  int beginning;
  /* if in middle of word, beginning of word */

  /* if at beginning of a word, find beginning of previous word */

  if (owl_editwin_is_char_in(e, WHITESPACE)) {
    /* if in whitespace past end of word, find a word , the find the beginning*/
    distance += owl_editwin_move_if_in(e, -1, WHITESPACE); /* leaves us on the last
							      character of the word */
    oe_save_excursion(e, &x);
    /* are we at the beginning of a word? */
    owl_editwin_point_move(e, -1);
    beginning = owl_editwin_is_char_in(e, WHITESPACE);
    oe_restore_excursion(e, &x);
    if (beginning)
      return distance;
   } else {
    /* in the middle of the word; */
    oe_save_excursion(e, &x);
    further += owl_editwin_point_move(e, -1);
    if (owl_editwin_is_char_in(e, WHITESPACE)) { /* we were at the beginning */
      distance += owl_editwin_backward_word(e); /* previous case */
      return distance + further;
    } else {
      oe_restore_excursion(e, &x);
    }
  }
  distance += owl_editwin_move_if_not_in(e, -1, WHITESPACE);
  /* will go past */
  if (e->index > e->lock)
    distance += owl_editwin_point_move(e, 1);
  return distance;
}

void owl_editwin_move_to_previousword(owl_editwin *e)
{
  owl_editwin_backward_word(e);
}

void owl_editwin_delete_nextword(owl_editwin *e)
{
  oe_excursion x;
  int distance;

  oe_save_excursion(e, &x);
  distance = owl_editwin_forward_word(e);
  oe_restore_excursion(e, &x);
  owl_editwin_replace(e, distance, "");
}

void owl_editwin_delete_previousword(owl_editwin *e)
{
  /* go backwards to the last non-space character, then delete chars */
  int distance;

  distance = owl_editwin_backward_word(e);
  owl_editwin_replace(e, -distance, "");
}

void owl_editwin_move_to_line_end(owl_editwin *e)
{
  owl_editwin_move_to_end_of_line(e);
}

void owl_editwin_delete_to_endofline(owl_editwin *e)
{
  oe_excursion x;
  int distance;

  oe_save_excursion(e, &x);
  distance = owl_editwin_move_to_end_of_line(e);
  oe_restore_excursion(e, &x);
  owl_editwin_replace(e, distance ? distance : 1, "");
}

void owl_editwin_move_to_line_start(owl_editwin *e)
{
  owl_editwin_move_to_beginning_of_line(e);
}

void owl_editwin_move_to_end(owl_editwin *e)
{
  oe_set_index(e, e->bufflen);
}

void owl_editwin_move_to_top(owl_editwin *e)
{
  oe_set_index(e, e->lock);
}

void owl_editwin_fill_paragraph(owl_editwin *e)
{
#if 0 /* XXX */
  oe_excursion x;
  int i, save;

  /* save our starting point */
  oe_save_excursion(e, &x);

  save = e->index;

  /* scan back to the beginning of this paragraph */
  for (i=save; i>=e->lock; i--) {
    if ( (i<=e->lock) ||
	 ((e->buff[i]=='\n') && (e->buff[i-1]=='\n'))) {
      oe_set_index(i + 1);
      break;
    }
  }

  /* main loop */
  while (1) {
    i = _owl_editwin_get_index_from_xy(e);

    /* bail if we hit the end of the buffer */
    if (i >= e->bufflen || e->buff[i] == '\0') break;

    /* bail if we hit the end of the paragraph */
    if (e->buff[i] == '\n' && e->buff[i+1] == '\n') break;

    /* bail if we hit a trailing dot on the buffer */
    if (e->buff[i] == '\n' && e->buff[i+1] == '.'
        && ((i+2) >= e->bufflen || e->buff[i+2] == '\0'))
      break;

    /* if we've travelled too far, linewrap */
    if ((e->buffx) >= e->fillcol) {
      int len = e->bufflen;
      _owl_editwin_linewrap_word(e);
      /* we may have added a character. */
      if (i < save) save += e->bufflen - len;
      oe_set_index(i);
    }

    /* did we hit the end of a line too soon? */
    /* asedeno: Here we replace a newline with a space. We may want to
       consider removing the space if the characters to either side
       are CJK ideograms.*/
    i = _owl_editwin_get_index_from_xy(e);
    if (e->buff[i] == '\n' && e->buffx < e->fillcol - 1) {
      /* ********* we need to make sure we don't pull in a word that's too long ***********/
      e->buff[i]=' ';
    }

    /* fix spacing */
    i = _owl_editwin_get_index_from_xy(e);
    if (e->buff[i] == ' ' && e->buff[i+1] == ' ') {
      if (e->buff[i-1] == '.' || e->buff[i-1] == '!' || e->buff[i-1] == '?') {
	owl_editwin_key_right(e);
      } else {
	owl_editwin_delete_char(e);
	/* if we did this ahead of the save point, adjust it. Changing
           by one is fine here because we're only removing an ASCII
           space. */
	if (i < save) save--;
      }
    } else {
      owl_editwin_key_right(e);
    }
  }

  /* put cursor back at starting point */
  oe_restore_excursion(e, &x);
#endif
}

/* returns true if only whitespace remains */
int owl_editwin_is_at_end(owl_editwin *e)
{
  return (only_whitespace(e->buff + e->index));
}

static int owl_editwin_check_dotsend(owl_editwin *e)
{
  char *p, *p_n, *p_p;
  gunichar c;

  if (!e->dotsend) return(0);

  p = g_utf8_find_prev_char(e->buff, e->buff + e->bufflen);
  p_n = g_utf8_find_next_char(p, NULL);
  p_p = g_utf8_find_prev_char(e->buff, p);
  c = g_utf8_get_char(p);
  while (p != NULL) {
    if (*p == '.'
	&& p_p != NULL && (*p_p == '\n' || *p_p == '\r')
	&& p_n != NULL && (*p_n == '\n' || *p_n == '\r')) {
      e->bufflen = p - e->buff;
      e->buff[e->bufflen] = '\0';
      return(1);
    }
    if (c != '\0' && !g_unichar_isspace(c)) return(0);
    p_n = p;
    p = p_p;
    c = g_utf8_get_char(p);
    p_p = g_utf8_find_prev_char(e->buff, p);
  }
  return(0);
}

void owl_editwin_post_process_char(owl_editwin *e, owl_input j)
{
  /* XXX force a redisplay? */
  if ((j.ch==13 || j.ch==10) && owl_editwin_check_dotsend(e)) {
    owl_command_editmulti_done(e);
    return;
  }
  owl_editwin_redisplay(e, 0);
}

void owl_editwin_process_char(owl_editwin *e, owl_input j)
{
  char tmp[7];

  if (j.ch == ERR)
    return;
  /* Ignore ncurses control characters. */
  if (j.ch < 0x100) {
    if (!(g_unichar_iscntrl(j.uch) && (j.uch != 10) && (j.uch != 13)) || j.uch==9 ) {
      memset(tmp, 0, 7);

      /* \r is \n */
      if (j.uch == '\r') {
	j.uch = '\n';
      }

      if (j.uch == '\n' && e->style == OWL_EDITWIN_STYLE_ONELINE) {
	return;
      }

      g_unichar_to_utf8(j.uch, tmp);
      owl_editwin_replace(e, 0, tmp);
    }
  }
}

char *owl_editwin_get_text(owl_editwin *e)
{
  return(e->buff+e->lock);
}

int owl_editwin_get_echochar(owl_editwin *e) {
  return e->echochar;
}
