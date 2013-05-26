#include "owl.h"
#include <stdio.h>

typedef struct _owl_log_entry { /* noproto */
  char *filename;
  char *message;
} owl_log_entry;


static GMainContext *log_context;
static GMainLoop *log_loop;
static GThread *logging_thread;
bool defer_logs;
static GQueue *deferred_entry_queue;

/* This is now the one function that should be called to log a
 * message.  It will do all the work necessary by calling the other
 * functions in this file as necessary.
 */
void owl_log_message(const owl_message *m) {
  owl_function_debugmsg("owl_log_message: entering");

  if (m == NULL) {
    owl_function_debugmsg("owl_log_message: passed null message");
    return;
  }

  /* should we be logging this message? */
  if (!owl_log_shouldlog_message(m)) {
    owl_function_debugmsg("owl_log_message: not logging message");
    return;
  }

  /* handle incmoing messages */
  if (owl_message_is_direction_in(m)) {
    owl_log_incoming(m);
    owl_function_debugmsg("owl_log_message: leaving");
    return;
  }

  /* handle outgoing messages */
  owl_log_outgoing(m);

  owl_function_debugmsg("owl_log_message: leaving");
}

/* Return 1 if we should log the given message, otherwise return 0 */
int owl_log_shouldlog_message(const owl_message *m) {
  const owl_filter *f;

  /* If there's a logfilter and this message matches it, log */
  f=owl_global_get_filter(&g, owl_global_get_logfilter(&g));
  if (f && owl_filter_message_match(f, m)) return(1);

  /* otherwise we do things based on the logging variables */

  /* skip login/logout messages if appropriate */
  if (!owl_global_is_loglogins(&g) && owl_message_is_loginout(m)) return(0);
      
  /* check direction */
  if ((owl_global_get_loggingdirection(&g)==OWL_LOGGING_DIRECTION_IN) && owl_message_is_direction_out(m)) {
    return(0);
  }
  if ((owl_global_get_loggingdirection(&g)==OWL_LOGGING_DIRECTION_OUT) && owl_message_is_direction_in(m)) {
    return(0);
  }

  if (owl_message_is_type_zephyr(m)) {
    if (owl_message_is_personal(m) && !owl_global_is_logging(&g)) return(0);
    if (!owl_message_is_personal(m) && !owl_global_is_classlogging(&g)) return(0);
  } else {
    if (owl_message_is_private(m) || owl_message_is_loginout(m)) {
      if (!owl_global_is_logging(&g)) return(0);
    } else {
      if (!owl_global_is_classlogging(&g)) return(0);
    }
  }
  return(1);
}

CALLER_OWN char *owl_log_zephyr(const owl_message *m)
{
    char *tmp = NULL;
    GString *buffer = NULL;
    buffer = g_string_new("");
    tmp = short_zuser(owl_message_get_sender(m));
    g_string_append_printf(buffer, "Class: %s Instance: %s", 
                           owl_message_get_class(m), 
                           owl_message_get_instance(m));
    if (strcmp(owl_message_get_opcode(m), "")) {
      g_string_append_printf(buffer, " Opcode: %s", 
                             owl_message_get_opcode(m));
    }
    g_string_append_printf(buffer, "\n");
    g_string_append_printf(buffer, "Time: %s Host: %s\n", 
                           owl_message_get_timestr(m), 
                           owl_message_get_hostname(m));
    g_string_append_printf(buffer, "From: %s <%s>\n\n", 
                           owl_message_get_zsig(m), tmp);
    g_string_append_printf(buffer, "%s\n\n", owl_message_get_body(m));
    g_free(tmp);
    return g_string_free(buffer, FALSE);
}

CALLER_OWN char *owl_log_aim(const owl_message *m)
{
    GString *buffer = NULL;
    buffer = g_string_new("");
    g_string_append_printf(buffer, "From: <%s> To: <%s>\n", 
                           owl_message_get_sender(m), owl_message_get_recipient(m));
    g_string_append_printf(buffer, "Time: %s\n\n", 
                           owl_message_get_timestr(m));
    if (owl_message_is_login(m)) {
        g_string_append_printf(buffer, "LOGIN\n\n");
    } else if (owl_message_is_logout(m)) {
        g_string_append_printf(buffer, "LOGOUT\n\n");
    } else {
        g_string_append_printf(buffer, "%s\n\n", owl_message_get_body(m));
    }
    return g_string_free(buffer, FALSE);
}

CALLER_OWN char *owl_log_jabber(const owl_message *m)
{
    GString *buffer = NULL;
    buffer = g_string_new("");
    g_string_append_printf(buffer, "From: <%s> To: <%s>\n",
                           owl_message_get_sender(m), 
                           owl_message_get_recipient(m));
    g_string_append_printf(buffer, "Time: %s\n\n", 
                           owl_message_get_timestr(m));
    g_string_append_printf(buffer, "%s\n\n", owl_message_get_body(m));
    return g_string_free(buffer, FALSE);
}

CALLER_OWN char *owl_log_generic(const owl_message *m)
{
    GString *buffer;
    buffer = g_string_new("");
    g_string_append_printf(buffer, "From: <%s> To: <%s>\n", 
                           owl_message_get_sender(m), 
                           owl_message_get_recipient(m));
    g_string_append_printf(buffer, "Time: %s\n\n", 
                           owl_message_get_timestr(m));
    g_string_append_printf(buffer, "%s\n\n", 
                           owl_message_get_body(m));
    return g_string_free(buffer, FALSE);
}

static void owl_log_error_main_thread(gpointer data)
{
  owl_function_error("%s", (const char*)data);
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

static void owl_log_file_error(owl_log_entry *msg, int ret)
{
  owl_log_error("Unable to open file for logging: %s (file %s)",
                g_strerror(ret),
                msg->filename);
}

/* If we are deferring log messages, enqueue this entry for writing.
 * Otherwise, try to write this log message, and, if it fails with
 * EPERM or EACCES, go into deferred logging mode and queue an admin
 * message.  If it fails with anything else, display an error message,
 * but do not go into deferred logging mode. */
static void owl_log_eventually_write_entry(gpointer data)
{
  int ret;
  owl_log_entry *msg = (owl_log_entry*)data;
  if (defer_logs) {
    owl_log_deferred_enqueue_message(msg->message, msg->filename);
  } else {
    ret = owl_log_try_write_entry(msg);
    if (ret == EPERM || ret == EACCES) {
      defer_logs = true;
      owl_log_error("Unable to open file for logging (%s): \n"
                    "%s.  \n"
                    "Consider renewing your tickets.  Logging has been \n"
                    "suspended, and your messages will be saved.  To \n"
                    "resume logging, use the command :flush-logs.\n\n",
                    msg->filename,
                    g_strerror(ret));
      owl_log_deferred_enqueue_message(msg->message, msg->filename);
    } else if (ret != 0) {
      owl_log_file_error(msg, ret);
    }
  }
}

/* tries to write the deferred log entries */
static void owl_log_write_deferred_entries(gpointer data)
{
  owl_log_entry *entry;
  bool drop_failed_logs = *(bool *)data;
  int ret;

  defer_logs = false;
  while (!g_queue_is_empty(deferred_entry_queue) && !defer_logs) {
    entry = (owl_log_entry*)g_queue_pop_head(deferred_entry_queue);
    if (drop_failed_logs) {
      owl_log_eventually_write_entry(entry);
    } else {
      ret = owl_log_try_write_entry(entry);
      if (ret != 0) {
        owl_log_file_error(entry, ret);
      }
    }
    owl_log_entry_free(entry);
  }
}

void owl_log_flush_logs(bool drop_failed_logs)
{
  bool *data = g_new(bool, 1);
  *data = drop_failed_logs;

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

void owl_log_append(const owl_message *m, const char *filename) {
  char *buffer = NULL;
  if (owl_message_is_type_zephyr(m)) {
    buffer = owl_log_zephyr(m);
  } else if (owl_message_is_type_jabber(m)) {
    buffer = owl_log_jabber(m);
  } else if (owl_message_is_type_aim(m)) {
    buffer = owl_log_aim(m);
  } else {
    buffer = owl_log_generic(m);
  }
  owl_log_enqueue_message(buffer, filename);
  g_free(buffer);
}

void owl_log_outgoing(const owl_message *m)
{
  char *filename, *logpath;
  char *to, *temp;
  GList *cc;

  /* expand ~ in path names */
  logpath = owl_util_makepath(owl_global_get_logpath(&g));

  /* Figure out what path to log to */
  if (owl_message_is_type_zephyr(m)) {
    /* If this has CC's, do all but the "recipient" which we'll do below */
    cc = owl_message_get_cc_without_recipient(m);
    while (cc != NULL) {
      temp = short_zuser(cc->data);
      filename = g_build_filename(logpath, temp, NULL);
      owl_log_append(m, filename);

      g_free(filename);
      g_free(temp);
      g_free(cc->data);
      cc = g_list_delete_link(cc, cc);
    }

    to = short_zuser(owl_message_get_recipient(m));
  } else if (owl_message_is_type_jabber(m)) {
    to = g_strdup_printf("jabber:%s", owl_message_get_recipient(m));
    g_strdelimit(to, "/", '_');
  } else if (owl_message_is_type_aim(m)) {
    char *temp2;
    temp = owl_aim_normalize_screenname(owl_message_get_recipient(m));
    temp2 = g_utf8_strdown(temp,-1);
    to = g_strdup_printf("aim:%s", temp2);
    g_free(temp2);
    g_free(temp);
  } else {
    to = g_strdup("loopback");
  }

  filename = g_build_filename(logpath, to, NULL);
  owl_log_append(m, filename);
  g_free(to);
  g_free(filename);

  filename = g_build_filename(logpath, "all", NULL);
  owl_log_append(m, filename);
  g_free(logpath);
  g_free(filename);
}


void owl_log_outgoing_zephyr_error(const owl_zwrite *zw, const char *text)
{
  char *filename, *logpath;
  char *tobuff, *recip;
  owl_message *m;
  GString *msgbuf;
  /* create a present message so we can pass it to
   * owl_log_shouldlog_message(void)
   */
  m = g_slice_new(owl_message);
  /* recip_index = 0 because there can only be one recipient anyway */
  owl_message_create_from_zwrite(m, zw, text, 0);
  if (!owl_log_shouldlog_message(m)) {
    owl_message_delete(m);
    return;
  }
  owl_message_delete(m);

  /* chop off a local realm */
  recip = owl_zwrite_get_recip_n_with_realm(zw, 0);
  tobuff = short_zuser(recip);
  g_free(recip);

  /* expand ~ in path names */
  logpath = owl_util_makepath(owl_global_get_logpath(&g));
  filename = g_build_filename(logpath, tobuff, NULL);
  msgbuf = g_string_new("");
  g_string_printf(msgbuf, "ERROR (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1] != '\n') {
    g_string_append_printf(msgbuf, "\n");
  }
  owl_log_enqueue_message(msgbuf->str, filename);
  g_string_free(msgbuf, TRUE);

  filename = g_build_filename(logpath, "all", NULL);
  g_free(logpath);
  msgbuf = g_string_new("");
  g_string_printf(msgbuf, "ERROR (owl): %s\n%s\n", tobuff, text);
  if (text[strlen(text)-1] != '\n') {
    g_string_append_printf(msgbuf, "\n");
  }
  owl_log_enqueue_message(msgbuf->str, filename);
  g_string_free(msgbuf, TRUE);

  g_free(tobuff);
}

void owl_log_incoming(const owl_message *m)
{
  char *filename, *allfilename, *logpath;
  const char *from=NULL;
  char *frombuff=NULL;
  int len, ch, i, personal;

  /* figure out if it's a "personal" message or not */
  if (owl_message_is_type_zephyr(m)) {
    if (owl_message_is_personal(m)) {
      personal = 1;
    } else {
      personal = 0;
    }
  } else if (owl_message_is_type_jabber(m)) {
    /* This needs to be fixed to handle groupchat */
    const char* msgtype = owl_message_get_attribute_value(m,"jtype");
    if (msgtype && !strcmp(msgtype,"groupchat")) {
      personal = 0;
    } else {
      personal = 1;
    }
  } else {
    if (owl_message_is_private(m) || owl_message_is_loginout(m)) {
      personal = 1;
    } else {
      personal = 0;
    }
  }


  if (owl_message_is_type_zephyr(m)) {
    if (personal) {
      from=frombuff=short_zuser(owl_message_get_sender(m));
    } else {
      from=frombuff=g_strdup(owl_message_get_class(m));
    }
  } else if (owl_message_is_type_aim(m)) {
    /* we do not yet handle chat rooms */
    char *normalto, *temp;
    temp = owl_aim_normalize_screenname(owl_message_get_sender(m));
    normalto = g_utf8_strdown(temp, -1);
    from=frombuff=g_strdup_printf("aim:%s", normalto);
    g_free(normalto);
    g_free(temp);
  } else if (owl_message_is_type_loopback(m)) {
    from=frombuff=g_strdup("loopback");
  } else if (owl_message_is_type_jabber(m)) {
    if (personal) {
      from=frombuff=g_strdup_printf("jabber:%s", 
				    owl_message_get_sender(m));
    } else {
      from=frombuff=g_strdup_printf("jabber:%s", 
				    owl_message_get_recipient(m));
    }
  } else {
    from=frombuff=g_strdup("unknown");
  }
  
  /* check for malicious sender formats */
  len=strlen(frombuff);
  if (len<1 || len>35) from="weird";
  if (strchr(frombuff, '/')) from="weird";

  ch=frombuff[0];
  if (!g_ascii_isalnum(ch)) from="weird";

  for (i=0; i<len; i++) {
    if (frombuff[i]<'!' || frombuff[i]>='~') from="weird";
  }

  if (!strcmp(frombuff, ".") || !strcasecmp(frombuff, "..")) from="weird";

  if (!personal) {
    if (strcmp(from, "weird")) {
      char* temp = g_utf8_strdown(frombuff, -1);
      if (temp) {
	g_free(frombuff);
	from = frombuff = temp;
      }
    }
  }

  /* create the filename (expanding ~ in path names) */
  if (personal) {
    logpath = owl_util_makepath(owl_global_get_logpath(&g));
    filename = g_build_filename(logpath, from, NULL);
    allfilename = g_build_filename(logpath, "all", NULL);
    owl_log_append(m, allfilename);
    g_free(allfilename);
  } else {
    logpath = owl_util_makepath(owl_global_get_classlogpath(&g));
    filename = g_build_filename(logpath, from, NULL);
  }

  owl_log_append(m, filename);
  g_free(filename);

  if (personal && owl_message_is_type_zephyr(m)) {
    /* We want to log to all of the CC'd people who were not us, or
     * the sender, as well.
     */
    char *temp;
    GList *cc;
    cc = owl_message_get_cc_without_recipient(m);
    while (cc != NULL) {
      temp = short_zuser(cc->data);
      if (strcasecmp(temp, frombuff) != 0) {
	filename = g_build_filename(logpath, temp, NULL);
        owl_log_append(m, filename);
	g_free(filename);
      }

      g_free(temp);
      g_free(cc->data);
      cc = g_list_delete_link(cc, cc);
    }
  }

  g_free(frombuff);
  g_free(logpath);
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
   * entries */
  bool bool_true = true;
  owl_log_write_deferred_entries(&bool_true);
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
