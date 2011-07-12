#include "owl.h"
#include <stdio.h>

typedef struct _owl_log_entry { /* noproto */
  char *filename;
  char *message;
} owl_log_entry;

typedef struct _owl_log_options { /* noproto */
  bool drop_failed_logs;
  bool display_initial_log_count;
} owl_log_options;

static GMainContext *log_context;
static GMainLoop *log_loop;
static GThread *logging_thread;
static bool defer_logs; /* to be accessed only on the disk-writing thread */
static GQueue *deferred_entry_queue;

static void owl_log_error_main_thread(gpointer data)
{
  owl_function_error("%s", (const char*)data);
}

static void owl_log_adminmsg_main_thread(gpointer data)
{
  owl_function_adminmsg("Logging", (const char*)data);
}

static void owl_log_makemsg_main_thread(gpointer data)
{
  owl_function_makemsg((const char*)data);
}

static void G_GNUC_PRINTF(1, 2) owl_log_error(const char *fmt, ...)
{
  va_list ap;
  char *data;

  va_start(ap, fmt);
  data = g_strdup_vprintf(fmt, ap);
  va_end(ap);

  owl_select_post_task(owl_log_error_main_thread,
                       data, g_free, g_main_context_default());
}

static void G_GNUC_PRINTF(1, 2) owl_log_adminmsg(const char *fmt, ...)
{
  va_list ap;
  char *data;

  va_start(ap, fmt);
  data = g_strdup_vprintf(fmt, ap);
  va_end(ap);

  owl_select_post_task(owl_log_adminmsg_main_thread,
                       data, g_free, g_main_context_default());
}

static void G_GNUC_PRINTF(1, 2) owl_log_makemsg(const char *fmt, ...)
{
  va_list ap;
  char *data;

  va_start(ap, fmt);
  data = g_strdup_vprintf(fmt, ap);
  va_end(ap);

  owl_select_post_task(owl_log_makemsg_main_thread,
                       data, g_free, g_main_context_default());
}

static CALLER_OWN owl_log_entry *owl_log_new_entry(const char *buffer, const char *filename)
{
  owl_log_entry *log_msg = g_slice_new(owl_log_entry);
  log_msg->message = g_strdup(buffer);
  log_msg->filename = g_strdup(filename);
  return log_msg;
}

static void owl_log_deferred_enqueue_message(const char *buffer, const char *filename)
{
  g_queue_push_tail(deferred_entry_queue, owl_log_new_entry(buffer, filename));
}

static void owl_log_deferred_enqueue_first_message(const char *buffer, const char *filename)
{
  g_queue_push_head(deferred_entry_queue, owl_log_new_entry(buffer, filename));
}

/* write out the entry if possible
 * return 0 on success, errno on failure to open
 */
static int owl_log_try_write_entry(owl_log_entry *msg)
{
  FILE *file = NULL;
  file = fopen(msg->filename, "a");
  if (!file) {
    return errno;
  }
  fprintf(file, "%s", msg->message);
  fclose(file);
  return 0;
}

static void owl_log_entry_free(void *data)
{
  owl_log_entry *msg = (owl_log_entry*)data;
  if (msg) {
    g_free(msg->message);
    g_free(msg->filename);
    g_slice_free(owl_log_entry, msg);
  }
}

#if GLIB_CHECK_VERSION(2, 32, 0)
#else
static void owl_log_entry_free_gfunc(gpointer data, gpointer user_data)
{
  owl_log_entry_free(data);
}
#endif

static void owl_log_file_error(owl_log_entry *msg, int errnum)
{
  owl_log_error("Unable to open file for logging: %s (file %s)",
                g_strerror(errnum),
                msg->filename);
}

/* If we are deferring log messages, enqueue this entry for writing.
 * Otherwise, try to write this log message, and, if it fails with
 * EPERM, EACCES, or ETIMEDOUT, go into deferred logging mode and
 * queue an admin message.  If it fails with anything else, display an
 * error message, drop the log message, and do not go into deferred
 * logging mode.
 *
 * N.B. This function is called only on the disk-writing thread. */
static void owl_log_eventually_write_entry(gpointer data)
{
  int ret;
  owl_log_entry *msg = (owl_log_entry*)data;
  if (defer_logs) {
    owl_log_deferred_enqueue_message(msg->message, msg->filename);
  } else {
    ret = owl_log_try_write_entry(msg);
    if (ret == EPERM || ret == EACCES || ret == ETIMEDOUT) {
      defer_logs = true;
      owl_log_error("Unable to open file for logging (%s): \n"
                    "%s.  \n"
                    "Consider renewing your tickets.  Logging has been \n"
                    "suspended, and your messages will be saved.  To \n"
                    "resume logging, use the command :flush-logs.\n\n",
                    msg->filename,
                    g_strerror(ret));
      /* If we were not in deferred logging mode, either the queue should be
       * empty, or we are attempting to log a message that we just popped off
       * the head of the queue.  Either way, we should enqueue this message as
       * the first message in the queue, rather than the last, so that we
       * preserve message ordering. */
      owl_log_deferred_enqueue_first_message(msg->message, msg->filename);
    } else if (ret != 0) {
      owl_log_file_error(msg, ret);
    }
  }
}

/* tries to write the deferred log entries
 *
 * N.B. This function is called only on the disk-writing thread. */
static void owl_log_write_deferred_entries(gpointer data)
{
  owl_log_entry *entry;
  owl_log_options *opts = (owl_log_options *)data;
  int ret;
  int logged_message_count = 0;
  bool all_succeeded = true;

  if (opts->display_initial_log_count) {
    if (g_queue_is_empty(deferred_entry_queue)) {
      owl_log_makemsg("There are no logs to flush.");
    } else {
      owl_log_makemsg("Attempting to flush %u logs...",
                      g_queue_get_length(deferred_entry_queue));
    }
  }

  defer_logs = false;
  while (!g_queue_is_empty(deferred_entry_queue) && !defer_logs) {
    logged_message_count++;
    entry = (owl_log_entry*)g_queue_pop_head(deferred_entry_queue);
    if (!opts->drop_failed_logs) {
      /* Attempt to write the log entry.  If it fails, re-queue the entry at
       * the head of the queue. */
      owl_log_eventually_write_entry(entry);
    } else {
      /* Attempt to write the log entry. If it fails, print an error message,
       * drop the log, and keep going through the queue. */
      ret = owl_log_try_write_entry(entry);
      if (ret != 0) {
        all_succeeded = false;
        owl_log_file_error(entry, ret);
      }
    }
    owl_log_entry_free(entry);
  }
  if (logged_message_count > 0) {
    if (opts->display_initial_log_count) {
      /* first clear the "attempting to flush" message from the status bar */
      owl_log_makemsg("");
    }
    if (!defer_logs) {
      if (all_succeeded) {
        owl_log_adminmsg("Flushed %d logs and resumed logging.",
                         logged_message_count);
      } else {
        owl_log_adminmsg("Flushed or dropped %d logs and resumed logging.",
                         logged_message_count);
      }
    } else {
      owl_log_error("Attempted to flush %d logs; %u deferred logs remain.",
                    logged_message_count, g_queue_get_length(deferred_entry_queue));
    }
  }
}

void owl_log_flush_logs(bool drop_failed_logs, bool quiet)
{
  owl_log_options *data = g_new(owl_log_options, 1);
  data->drop_failed_logs = drop_failed_logs;
  data->display_initial_log_count = !quiet;

  owl_select_post_task(owl_log_write_deferred_entries,
                       data,
                       g_free,
                       log_context);
}

void owl_log_enqueue_message(const char *buffer, const char *filename)
{
  owl_log_entry *log_msg = owl_log_new_entry(buffer, filename);
  owl_select_post_task(owl_log_eventually_write_entry, log_msg,
		       owl_log_entry_free, log_context);
}

void owl_log_outgoing_zephyr_error(const owl_zwrite *zw, const char *text)
{
  owl_message *m = g_slice_new(owl_message);
  /* recip_index = 0 because there can only be one recipient anyway */
  owl_message_create_from_zwrite(m, zw, text, 0);
  g_free(owl_perlconfig_call_with_message("BarnOwl::Logging::log_outgoing_error", m));
  owl_message_delete(m);
}

static gpointer owl_log_thread_func(gpointer data)
{
  log_context = g_main_context_new();
  log_loop = g_main_loop_new(log_context, FALSE);
  g_main_loop_run(log_loop);
  return NULL;
}

void owl_log_init(void)
{
  log_context = g_main_context_new();
#if GLIB_CHECK_VERSION(2, 31, 0)
  logging_thread = g_thread_new("logging",
				owl_log_thread_func,
				NULL);
#else
  GError *error = NULL;
  logging_thread = g_thread_create(owl_log_thread_func,
                                   NULL,
                                   TRUE,
                                   &error);
  if (error) {
    endwin();
    fprintf(stderr, "Error spawning logging thread: %s\n", error->message);
    fflush(stderr);
    exit(1);
  }
#endif

  deferred_entry_queue = g_queue_new();
}

static void owl_log_quit_func(gpointer data)
{
  /* flush the deferred logs queue, trying to write the
   * entries to the disk one last time.  Drop any failed
   * entries, and be quiet about it. */
  owl_log_options opts;
  opts.drop_failed_logs = true;
  opts.display_initial_log_count = false;
  owl_log_write_deferred_entries(&opts);
#if GLIB_CHECK_VERSION(2, 32, 0)
  g_queue_free_full(deferred_entry_queue, owl_log_entry_free);
#else
  g_queue_foreach(deferred_entry_queue, owl_log_entry_free_gfunc, NULL);
  g_queue_free(deferred_entry_queue);
#endif

  g_main_loop_quit(log_loop);
}

void owl_log_shutdown(void)
{
  owl_select_post_task(owl_log_quit_func, NULL,
		       NULL, log_context);
  g_thread_join(logging_thread);
}
