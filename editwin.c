#include "owl.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define VALID_EXCURSION	(0x9a2b4729)

typedef struct _owl_editwin_excursion { /*noproto*/
  int valid;
  int index;
  int mark;
  int goal_column;
  int lock;
  struct _owl_editwin_excursion *next;
} oe_excursion;

struct _owl_editwin { /*noproto*/
  int refcount;
  char *buff;
  owl_history *hist;
  int bufflen;
  int allocated;
  int index;
  int mark;
  char *killbuf;
  int goal_column;
  int topindex;
  int cursorx;
  int winlines, wincols, fillcol, wrapcol;
  owl_window *win;
  gulong repaint_id;
  gulong resized_id;
  int style;
  int lock;
  int dotsend;
  int echochar;
  oe_excursion *excursions;

  void (*callback)(struct _owl_editwin*);
  void (*destroy_cbdata)(void *);
  void *cbdata;
};

static void oe_set_window(owl_editwin *e, owl_window *w, int winlines, int wincols);
static void oe_redraw(owl_window *win, WINDOW *curswin, void *user_data);
static void oe_reframe(owl_editwin *e);
static void oe_save_excursion(owl_editwin *e, oe_excursion *x);
static void oe_release_excursion(owl_editwin *e, oe_excursion *x);
static void oe_restore_excursion(owl_editwin *e, oe_excursion *x);
static void oe_restore_mark_only(owl_editwin *e, oe_excursion *x);
static int oe_char_width(gunichar c, int column);
static int oe_region_width(owl_editwin *e, int start, int end, int width);
static int oe_find_display_line(owl_editwin *e, int *x, int index, int *hard);
static void oe_insert_char(owl_editwin *e, gunichar c);
static int owl_editwin_limit_maxcols(int width, int cols);
static int owl_editwin_check_dotsend(owl_editwin *e);
static int owl_editwin_is_char_in(owl_editwin *e, const char *set);
static gunichar owl_editwin_get_char_at_point(owl_editwin *e);
static int owl_editwin_replace_internal(owl_editwin *e, int replace, const char *s);
static const char *oe_copy_buf(owl_editwin *e, const char *buf, int len);
static int oe_copy_region(owl_editwin *e);
static char *oe_chunk(owl_editwin *e, int start, int end);
static void oe_destroy_cbdata(owl_editwin *e);
static void oe_dirty(owl_editwin *e);
static void oe_window_resized(owl_window *w, owl_editwin *e);

#define INCR 4096

#define WHITESPACE " \n\t"

static owl_editwin *owl_editwin_allocate(void)
{
  owl_editwin *e;
  e = owl_malloc(sizeof(owl_editwin));
  memset(e, 0, sizeof(*e));
  e->refcount = 1;
  return e;
}

static void _owl_editwin_delete(owl_editwin *e)
{
  if (e->win) {
    g_signal_handler_disconnect(e->win, e->repaint_id);
    g_signal_handler_disconnect(e->win, e->resized_id);
    g_object_unref(e->win);
  }
  owl_free(e->buff);
  owl_free(e->killbuf);
  /* just in case someone forgot to clean up */
  while (e->excursions) {
    oe_release_excursion(e, e->excursions);
  }
  oe_destroy_cbdata(e);

  owl_free(e);
}

static inline void oe_set_index(owl_editwin *e, int index)
{
  if (index != e->index) {
    e->goal_column = -1;
    e->cursorx = -1;
  }
  e->index = index;
  oe_dirty(e);
}

static inline void oe_set_mark(owl_editwin *e, int mark)
{
  e->mark = mark;
}

void owl_editwin_set_mark(owl_editwin *e)
{
  oe_set_mark(e, e->index);
  /* owl_function_makemsg("Mark set."); */
}

static void _owl_editwin_init(owl_editwin *e,
                              int winlines,
                              int wincols,
                              int style,
                              owl_history *hist)
{
  e->buff=owl_malloc(INCR);
  e->buff[0]='\0';
  e->bufflen=0;
  e->hist=hist;
  e->allocated=INCR;
  oe_set_index(e, 0);
  oe_set_mark(e, -1);
  if (e->killbuf != NULL)
    owl_free(e->killbuf);
  e->killbuf = NULL;
  e->goal_column = -1;
  e->cursorx = -1;
  e->topindex = 0;
  e->excursions = NULL;
  e->style=style;
  if ((style!=OWL_EDITWIN_STYLE_MULTILINE) &&
      (style!=OWL_EDITWIN_STYLE_ONELINE)) {
    e->style=OWL_EDITWIN_STYLE_MULTILINE;
  }
  e->lock=0;
  e->dotsend=0;
  e->echochar='\0';
}

owl_editwin *owl_editwin_new(owl_window *win, int winlines, int wincols, int style, owl_history *hist)
{
  owl_editwin *e = owl_editwin_allocate();

  _owl_editwin_init(e, winlines, wincols, style, hist);
  oe_set_window(e, win, winlines, wincols);
  return e;
}

owl_editwin *owl_editwin_ref(owl_editwin *e)
{
  e->refcount++;
  return e;
}

void owl_editwin_unref(owl_editwin *e)
{
  e->refcount--;
  if (e->refcount <= 0)
    _owl_editwin_delete(e);
}

/* TODO: collapse this window into oe_window_resized. Need to stop
 * passing winlines and wincols to oe_set_window and get them out of
 * the owl_window. The tester code will first need to pass in an
 * owl_window headless or so. */
static void oe_set_window_size(owl_editwin *e, int winlines, int wincols)
{
  e->winlines=winlines;
  e->wincols=wincols;
  /* fillcol and wrapcol may have changed. */
  e->fillcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxfillcols(&g));
  if (e->style == OWL_EDITWIN_STYLE_MULTILINE)
    e->wrapcol=owl_editwin_limit_maxcols(wincols-7, owl_global_get_edit_maxwrapcols(&g));
  else
    e->wrapcol = 0;
}

static void oe_window_resized(owl_window *w, owl_editwin *e)
{
  int winlines, wincols;
  owl_window_get_position(w, &winlines, &wincols, NULL, NULL);
  oe_set_window_size(e, winlines, wincols);
}

static void oe_set_window(owl_editwin *e, owl_window *w, int winlines, int wincols)
{
  e->win=w;
  oe_set_window_size(e, winlines, wincols);
  if (e->win) {
    g_object_ref(e->win);
    e->repaint_id = g_signal_connect(w, "redraw", G_CALLBACK(oe_redraw), e);
    e->resized_id = g_signal_connect(w, "resized", G_CALLBACK(oe_window_resized), e);
    owl_window_dirty(e->win);
  }
}

owl_window *owl_editwin_get_window(owl_editwin *e)
{
  return e->win;
}

/* echo the character 'ch' for each normal character keystroke,
 * excepting locktext.  This is useful for entering passwords etc.  If
 * ch=='\0' characters are echo'd normally
 */
void owl_editwin_set_echochar(owl_editwin *e, int ch)
{
  e->echochar=ch;
  oe_dirty(e);
}

owl_history *owl_editwin_get_history(owl_editwin *e)
{
  return(e->hist);
}

void owl_editwin_set_dotsend(owl_editwin *e)
{
  e->dotsend=1;
}

void owl_editwin_set_callback(owl_editwin *e, void (*cb)(owl_editwin*))
{
  e->callback = cb;
}

void (*owl_editwin_get_callback(owl_editwin *e))(owl_editwin*)
{
  return e->callback;
}

static void oe_destroy_cbdata(owl_editwin *e) {
  if (e->destroy_cbdata)
    e->destroy_cbdata(e->cbdata);
  e->cbdata = NULL;
  e->destroy_cbdata = NULL;
}

void owl_editwin_set_cbdata(owl_editwin *e, void *data, void (*destroy)(void *))
{
  oe_destroy_cbdata(e);
  e->cbdata = data;
  e->destroy_cbdata = destroy;
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

static int owl_editwin_limit_maxcols(int width, int cols)
{
  if (cols == 0)
    return width;
  return cols;
}

/* set text to be 'locked in' at the beginning of the buffer, any
 * previous text (including locked text) will be overwritten
 */
void owl_editwin_set_locktext(owl_editwin *e, const char *text)
{
  oe_set_index(e, 0);
  e->lock = 0;
  owl_editwin_replace(e, e->bufflen, text);
  e->buff[e->bufflen] = 0;
  e->lock=e->bufflen;
  oe_set_index(e, e->lock);
  oe_dirty(e);
}

int owl_editwin_get_style(owl_editwin *e)
{
  return(e->style);
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
  _owl_editwin_init(e, e->winlines, e->wincols, e->style, e->hist);

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
  oe_dirty(e);
}

static void oe_save_excursion(owl_editwin *e, oe_excursion *x)
{
  x->index = e->index;
  x->mark = e->mark;
  x->goal_column = e->goal_column;
  x->lock = e->lock;

  x->valid = VALID_EXCURSION;
  x->next = e->excursions;
  e->excursions = x;
}

static void oe_release_excursion(owl_editwin *e, oe_excursion *x)
{
  oe_excursion **px;

  x->valid = 0;
  for (px = &e->excursions; *px != NULL; px = &(*px)->next)
    if (*px == x) {
      *px = x->next;
      return;
    }
  abort();
}

static void oe_restore_excursion(owl_editwin *e, oe_excursion *x)
{
  if (x->valid == VALID_EXCURSION) {
    oe_set_index(e, x->index);
    e->goal_column = x->goal_column;
    e->mark = x->mark;
    e->lock = x->lock;

    oe_release_excursion(e, x);
  }
}

static void oe_restore_mark_only(owl_editwin *e, oe_excursion *x)
{
  if (x->valid == VALID_EXCURSION) {
    e->mark = x->mark;

    oe_release_excursion(e, x);
  }
}

/* External interface to oe_save_excursion */
owl_editwin_excursion *owl_editwin_begin_excursion(owl_editwin *e)
{
  owl_editwin_excursion *x = owl_malloc(sizeof *x);
  oe_save_excursion(e, x);
  return x;
}

void owl_editwin_end_excursion(owl_editwin *e, owl_editwin_excursion *x)
{
  oe_restore_excursion(e, x);
  owl_free(x);
}

static inline const char *oe_next_point(owl_editwin *e, const char *p)
{
  const char *boundary = e->buff + e->bufflen + 1;
  const char *q;

  q = g_utf8_find_next_char(p, boundary);
  while (q && g_unichar_ismark(g_utf8_get_char(q)))
    q = g_utf8_find_next_char(q, boundary);

  if (q == p)
    return NULL;
  return q;
}

static inline const char *oe_prev_point(owl_editwin *e, const char *p)
{
  const char *boundary = e->buff + e->lock;

  p = g_utf8_find_prev_char(boundary, p);
  while (p && g_unichar_ismark(g_utf8_get_char(p)))
    p = g_utf8_find_prev_char(boundary, p);

  return p;
}

static int oe_char_width(gunichar c, int column)
{
  int cw;

  if (c == 9) /* TAB */
    return TABSIZE - column % TABSIZE;

  cw = mk_wcwidth(c);

  if (cw < 0) /* control characters */
    cw = 0;

  return cw;
}

/* Finds the display line of 'e' starting at the character
 * 'index'. The index just after the line is returned. Whether the
 * line ends at a hard break (newline) or soft break (wrapping) is
 * returned in 'hard'.
 *
 * If the point (e->index) is contained in the line, its position is
 * returned in 'x'.
 */
static int oe_find_display_line(owl_editwin *e, int *x, int index, int *hard)
{
  int width = 0, cw;
  gunichar c;
  const char *p;

  while(1) {
    /* note the position of the dot */
    if (x != NULL && index == e->index && width < e->wincols)
      *x = width;

    /* get the current character */
    c = g_utf8_get_char(e->buff + index);

    /* figure out how wide it is */
    cw = oe_char_width(c, width);

    if (width + cw > e->wincols - 1) {
      if (x != NULL && *x == width)
	*x = -1;
      if (hard != NULL) *hard = 0;
      break;
    }
    width += cw;

    if (c == '\n') {
      if (width < e->wincols)
	++index; /* skip the newline */
      if (hard != NULL) *hard = 1;
      break;
    }

    /* find the next character */
    p = oe_next_point(e, e->buff + index);
    if (p == NULL) { /* we ran off the end */
      if (x != NULL && e->index > index)
	*x = width + 1;
      if (hard != NULL) *hard = 1;
      break;
    }
    index = p - e->buff;

  }
  return index;
}

static void oe_reframe(owl_editwin *e) {
  oe_excursion x;
  int goal = 1 + e->winlines / 2;
  int index;
  int count = 0;
  int n, i;
  int last;

  oe_save_excursion(e, &x);
  /* step back line-by-line through the buffer until we have >= goal lines of
     display text */
  e->lock = 0; /* we can (must) tread on the locktext */

  last = -1;
  while (count < goal) {
    index = e->index;
    owl_editwin_move_to_beginning_of_line(e);
    if (last == e->index)
      break;
    last = e->index;
    for (n = 0, i = e->index; i < index; n++)
      i = oe_find_display_line(e, NULL, i, NULL);
    count += n == 0 ? 1 : n;
    if (count < goal)
      owl_editwin_point_move(e, -1);
  }

  e->topindex = e->index;
  /* if we overshot, backtrack */
  for (n = 0; n < (count - goal); n++)
    e->topindex = oe_find_display_line(e, NULL, e->topindex, NULL);

  oe_restore_excursion(e, &x);
  oe_dirty(e);
}

static void oe_addnec(owl_editwin *e, WINDOW *curswin, int count)
{
  int i;

  for (i = 0; i < count; i++)
    waddch(curswin, e->echochar);
}

static void oe_mvaddnec(owl_editwin *e, WINDOW *curswin, int y, int x, int count)
{
  wmove(curswin, y, x);
  oe_addnec(e, curswin, count);
}

/* regenerate the text on the curses window */
static void oe_redraw(owl_window *win, WINDOW *curswin, void *user_data)
{
  int x = -1, y = -1, t, hard;
  int line, index, lineindex, times = 0;
  owl_editwin *e = user_data;

  do {
    werase(curswin);

    if (e->topindex == -1 || e->index < e->topindex)
      oe_reframe(e);

    line = 0;
    index = e->topindex;
    while(line < e->winlines) {
      lineindex = index;
      t = -1;
      index = oe_find_display_line(e, &t, lineindex, &hard);
      if (x == -1 && t != -1)
	x = t, y = line;
      if (index - lineindex) {
	if (!e->echochar)
	  mvwaddnstr(curswin, line, 0,
		     e->buff + lineindex,
		     index - lineindex);
	else {
	  if(lineindex < e->lock) {
	    mvwaddnstr(curswin, line, 0,
		       e->buff + lineindex,
		       MIN(index - lineindex,
			   e->lock - lineindex));
	    if (e->lock < index)
	      oe_addnec(e, curswin,
			oe_region_width(e, e->lock, index,
					oe_region_width(e, lineindex, e->lock, 0)));
	  } else
	    oe_mvaddnec(e, curswin, line, 0, oe_region_width(e, lineindex, index, 0));
	}
        if (!hard)
          waddch(curswin, '\\');
      }
      line++;
    }
    if (x == -1)
	e->topindex = -1; /* force a reframe */
    times++;
  } while(x == -1 && times < 3);

  wmove(curswin, y, x);
  e->cursorx = x;
}

static inline void oe_fixup(int *target, int start, int end, int change) {
  if (*target > start) {
    if (*target <= end)
      *target = end + change;
    else
      *target += change;
  }
}

int owl_editwin_replace_region(owl_editwin *e, const char *s)
{
  oe_excursion x;
  int ret;

  if (e->mark == -1) {
    owl_function_error("The mark is unset, there is no region to replace.");
    return 0;
  }

  oe_save_excursion(e, &x);

  if(e->index > e->mark) {
    owl_editwin_exchange_point_and_mark(e);
  }

  ret = owl_editwin_replace_internal(e, e->mark - e->index, s);

  oe_restore_excursion(e, &x);

  return ret;
}

/* replace 'replace' characters at the point with s, returning the change in size */
int owl_editwin_replace(owl_editwin *e, int replace, const char *s)
{
  int start, end, i;
  const char *p;

  if (!g_utf8_validate(s, -1, NULL)) {
    owl_function_debugmsg("owl_editwin_insert_string: received non-utf-8 string.");
    return 0;
  }

  start = e->index;
  for (i = 0, p = e->buff + start; i < replace && p != NULL; i++)
    p = oe_next_point(e, p);
  if (p != NULL)
    end = p - e->buff;
  else
    end = e->bufflen;

  return owl_editwin_replace_internal(e, end - start, s);
}

static int owl_editwin_replace_internal(owl_editwin *e, int replace, const char *s)
{
  int start, end, free, need, size, change;
  oe_excursion *x;

  start = e->index;
  end   = start + replace;

  free = e->allocated - e->bufflen + end - start;

  need = strlen(s) - free;
  if (need > 0) {
    size = e->allocated + need + INCR - (need % INCR);
    e->buff = owl_realloc(e->buff, size);
    e->allocated = size;
  }

  memmove(e->buff + start + strlen(s), e->buff + end, e->bufflen + 1 - end);
  memcpy(e->buff + start, s, strlen(s));
  change = start - end + strlen(s);
  e->bufflen += change;
  e->index += strlen(s);

  /* fix up the mark */
  oe_fixup(&e->mark, start, end, change);
  oe_fixup(&e->topindex, start, end, change);
  /* fix up any saved points after the replaced area */
  for (x = e->excursions; x != NULL; x = x->next) {
    oe_fixup(&x->index, start, end, change);
    oe_fixup(&x->mark, start, end, change);
  }

  /* recenter if needed */
  if (start <= e->topindex)
    owl_editwin_recenter(e);

  oe_dirty(e);

  return change;
}

/* linewrap the word just before the cursor.
 * returns 0 on success
 * returns -1 if we could not wrap.
 */
static void _owl_editwin_linewrap_word(owl_editwin *e)
{
  oe_excursion x;
  gunichar c;

  oe_save_excursion(e, &x);

  while (owl_editwin_point_move(e, -1)) {
    c = owl_editwin_get_char_at_point(e);
    if (owl_util_can_break_after(c) || c == '\n') {
      if (c != '\n')
        owl_editwin_replace(e, c != ' ' ? 0 : 1, "\n");
      break;
    }
  }

  oe_restore_excursion(e, &x);
}

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
  const char *middle, *end, *start;
  char *tmp;

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
void owl_editwin_insert_string(owl_editwin *e, const char *s)
{
  owl_editwin_replace(e, 0, s);
}

/* We assume index is not set to point to a mid-char */
static gunichar owl_editwin_get_char_at_point(owl_editwin *e)
{
  return g_utf8_get_char(e->buff + e->index);
}

void owl_editwin_exchange_point_and_mark(owl_editwin *e) {
  int tmp;

  if (e->mark != -1) {
    tmp = e->mark;
    owl_editwin_set_mark(e);
    oe_set_index(e, tmp);
  }
}

int owl_editwin_point_move(owl_editwin *e, int delta)
{
  const char *p;
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

static int owl_editwin_at_beginning_of_line(owl_editwin *e)
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

static int owl_editwin_is_char_in(owl_editwin *e, const char *set)
{
  const char *p;

  for (p = set; *p != 0; p = g_utf8_find_next_char(p, NULL))
    if (owl_editwin_get_char_at_point(e) == g_utf8_get_char(p))
      return 1;
  return 0;
}

int owl_editwin_move_if_in(owl_editwin *e, int delta, const char *set)
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

int owl_editwin_move_if_not_in(owl_editwin *e, int delta, const char *set)
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
    /* If we stopped because we reached a '\n', rather than because we
     * hit the top of the buffer, move forward from the end of the
     * previous line to the start of the current. */
    if (owl_editwin_get_char_at_point(e) == '\n')
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
  oe_dirty(e);

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
      oe_release_excursion(e, &x);
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

  oe_save_excursion(e, &x);
  oe_set_mark(e, e->index);
  owl_editwin_forward_word(e);
  owl_editwin_kill_region(e);
  oe_restore_mark_only(e, &x);
}

void owl_editwin_delete_previousword(owl_editwin *e)
{
  oe_excursion x;

  oe_save_excursion(e, &x);
  oe_set_mark(e, e->index);
  owl_editwin_backward_word(e);
  owl_editwin_kill_region(e);
  oe_restore_mark_only(e, &x);
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
  owl_editwin_set_mark(e);
  distance = owl_editwin_move_to_end_of_line(e);
  if (distance)
    owl_editwin_kill_region(e);
  else
    owl_editwin_replace(e, 1, "");
  oe_restore_excursion(e, &x);
}

void owl_editwin_yank(owl_editwin *e)
{
  if (e->killbuf != NULL)
    owl_editwin_replace(e, 0, e->killbuf);
}

static const char *oe_copy_buf(owl_editwin *e, const char *buf, int len)
{
  char *p;

  p = owl_malloc(len + 1);

  if (p != NULL) {
    owl_free(e->killbuf);
    e->killbuf = p;
    memcpy(e->killbuf, buf, len);
    e->killbuf[len] = 0;
  }

  return p;
}

static int oe_copy_region(owl_editwin *e)
{
  const char *p;
  int start, end;

  if (e->mark == -1)
    return 0;

  start = MIN(e->index, e->mark);
  end = MAX(e->index, e->mark);

  p = oe_copy_buf(e, e->buff + start, end - start);
  if (p != NULL)
    return end - start;
  return 0;
}

void owl_editwin_copy_region_as_kill(owl_editwin *e)
{
  oe_copy_region(e);
}

void owl_editwin_kill_region(owl_editwin *e)
{
  if (e->index > e->mark)
    owl_editwin_exchange_point_and_mark(e);

  owl_editwin_replace_internal(e, oe_copy_region(e), "");
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

void owl_editwin_backward_paragraph(owl_editwin *e)
{
  owl_editwin_point_move(e, -1);
  for (; e->index >= e->lock; owl_editwin_point_move(e, -1)) {
    if (e->index <= e->lock ||
        ((e->buff[e->index] == '\n') && (e->buff[e->index - 1]=='\n')))
      break;
  }
}

void owl_editwin_forward_paragraph(owl_editwin *e)
{
  owl_editwin_point_move(e, 1);
  /* scan forward to the start of the next paragraph */
  for(; e->index < e->bufflen; owl_editwin_point_move(e, 1)) {
    if (e->buff[e->index -1] == '\n' && e->buff[e->index] == '\n')
      break;
  }
}

int owl_editwin_current_column(owl_editwin *e)
{
  oe_excursion x;
  int lineindex;

  oe_save_excursion(e, &x);
  owl_editwin_move_to_beginning_of_line(e);
  lineindex = e->index;
  oe_restore_excursion(e, &x);
  return oe_region_width(e, lineindex, e->index, 0);
}

void owl_editwin_fill_paragraph(owl_editwin *e)
{
  oe_excursion x;
  gunichar ch;
  int sentence;

  if (e->fillcol < 0)
    /* auto-fill disabled */
    return;

  oe_save_excursion(e, &x);

  /* Mark the end of the paragraph */
  owl_editwin_forward_paragraph(e);
  /* Skip the trailing newline */
  owl_editwin_point_move(e, -1);
  owl_editwin_set_mark(e);

  owl_editwin_backward_paragraph(e);

  /* Don't mess with the leading newline */
  if (owl_editwin_get_char_at_point(e) == '\n')
    owl_editwin_point_move(e, 1);

  /*
   * First pass: Scan forward replacing all series of spaces with ' '
   * (or nothing after CJK ideograms)
   */
  sentence = 0;
  for(;e->index < e->mark; owl_editwin_point_move(e, 1)) {
    /* bail if we hit a trailing dot on the buffer */
    if (strcmp(e->buff + e->index, "\n.") == 0) {
      owl_editwin_set_mark(e);
      break;
    }

    ch = owl_editwin_get_char_at_point(e);

    if (owl_util_can_break_after(ch) || ch == '\n') {
      if (g_unichar_isspace(ch)) {
        owl_editwin_replace(e, 1, " ");
      }

      if (sentence && g_unichar_isspace(owl_editwin_get_char_at_point(e))
	  && e->index < e->mark)
        owl_editwin_point_move(e, 1);

      while(g_unichar_isspace(owl_editwin_get_char_at_point(e))
            && e->index < e->mark) {
        owl_editwin_delete_char(e);
      }
    }

    if(ch == '.' || ch == '!' || ch == '?')
      sentence = 1;
    else
      sentence = 0;
  }

  owl_editwin_backward_paragraph(e);

  /* Now go through inserting newlines as needed */
  while(e->index < e->mark) {
    /* if we've travelled too far, linewrap */
    if (owl_editwin_current_column(e) >= e->fillcol)
      _owl_editwin_linewrap_word(e);
    owl_editwin_point_move(e, 1);
  }

  oe_restore_excursion(e, &x);
}

/* returns true if only whitespace remains */
int owl_editwin_is_at_end(owl_editwin *e)
{
  return (only_whitespace(e->buff + e->index));
}

static int owl_editwin_check_dotsend(owl_editwin *e)
{
  int zdot = 0;
  oe_excursion x;

  if (!e->dotsend) return(0);
  if (!owl_editwin_is_at_end(e)) return (0);

  oe_save_excursion(e, &x);

  owl_editwin_point_move(e, -3);

  if(strncmp(e->buff + e->index, "\n.\n", 3) == 0) {
    owl_editwin_point_move(e, 1);
    zdot = 1;
  } else if(e->index == e->lock &&
            strncmp(e->buff + e->index, ".\n", 2) == 0) {
    zdot = 1;
  }

  if(zdot) {
    owl_editwin_set_mark(e);
    owl_editwin_move_to_end(e);
    owl_editwin_replace_region(e, "");
  }

  oe_restore_excursion(e, &x);

  return zdot;
}

void owl_editwin_post_process_char(owl_editwin *e, owl_input j)
{
  /* XXX force a redisplay? */
  if ((j.ch==13 || j.ch==10) && owl_editwin_check_dotsend(e)) {
    owl_command_edit_done(e);
    return;
  }
}

static int oe_region_width(owl_editwin *e, int start, int end, int offset)
{
  const char *p;
  int width = offset;
  
  for(p = e->buff + start;
      p < e->buff + end;
      p = g_utf8_find_next_char(p, NULL))
    width += oe_char_width(g_utf8_get_char(p), width);

  return width - offset;
}

static void oe_insert_char(owl_editwin *e, gunichar c)
{
  oe_excursion x;
  char tmp[7];
  int replaced = -1;

  if (c == '\r') /* translate CRs to NLs */
    c = '\n';

  if (!g_unichar_iscntrl(c) || c == '\n' || c== '\t' ) {
    if (c == '\n' && e->style == OWL_EDITWIN_STYLE_ONELINE) {
      return;
    }

    if (e->wrapcol > 0 && e->cursorx != -1 &&
        e->cursorx + oe_char_width(c, e->cursorx) > e->wrapcol) {
      /* XXX this is actually wrong:
       * + If the line has been been wrapped, we can be past the wrap column but
       *   e->cursorx be much smaller.
       * + If the user went back and inserted a bunch of stuff in the middle of
       *   the line, there may be more than one word past the wrap column.
       */
      oe_save_excursion(e, &x);

      if (c == ' ' || c == '\t') {
	owl_editwin_point_move(e, -1);
	replaced = -owl_editwin_move_if_in(e, -1, " \t");
	if (!replaced) {
	  c = '\n';
	  replaced = -1;
	}
      } else {
	while(!owl_editwin_at_beginning_of_line(e)) {
	  owl_editwin_point_move(e, -1);
	  if (owl_util_can_break_after(owl_editwin_get_char_at_point(e))) {
	    replaced = -owl_editwin_move_if_in(e, -1, " \t");
	    break;
	  }
	}
	if (owl_editwin_at_beginning_of_line(e))
	  replaced = -1;
      }
      if (replaced && !owl_editwin_at_beginning_of_line(e))
	owl_editwin_point_move(e, 1);
      if (replaced >= 0) {
	owl_editwin_replace(e, replaced, "\n");
      }
      oe_restore_excursion(e, &x);
    }

    if (replaced >= 0 && (c == ' ' || c == '\t'))
      return; /* our work here is done */

    tmp[g_unichar_to_utf8(c, tmp)] = '\0';
    owl_editwin_replace(e, 0, tmp);
  }
}

void owl_editwin_process_char(owl_editwin *e, owl_input j)
{
  if (j.ch == ERR)
    return;
  /* Ignore ncurses control characters. */
  if (j.ch < 0x100) {
    oe_insert_char(e, j.uch);
  }
}

const char *owl_editwin_get_text(owl_editwin *e)
{
  return(e->buff+e->lock);
}

char *owl_editwin_get_region(owl_editwin *e)
{
  int start, end;
  start = e->index;
  end   = e->mark;
  if(start > end) {
    int tmp = end;
    end = start;
    start = tmp;
  }

  return oe_chunk(e, start, end);
}

int owl_editwin_get_echochar(owl_editwin *e)
{
  return e->echochar;
}

static char *oe_chunk(owl_editwin *e, int start, int end)
{
  char *p;
  
  p = owl_malloc(end - start + 1);
  memcpy(p, e->buff + start, end - start);
  p[end - start] = 0;

  return p;
}

/*
 * The only guarantee made about these values is that comparisons
 * between them, as well as comparison between multiple calls to these
 * functions without modifying the editwin in-between, are meaningful.
 */

int owl_editwin_get_point(owl_editwin *e)
{
  return e->index;
}

int owl_editwin_get_mark(owl_editwin *e)
{
  return e->mark;
}

static void oe_dirty(owl_editwin *e)
{
  if (e->win) owl_window_dirty(e->win);
}


/*
 * Local Variables:
 * mode:C
 * c-basic-offset:2
 * End:
 */
