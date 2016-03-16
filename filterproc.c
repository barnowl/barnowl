#include "filterproc.h"
#include <sys/wait.h>
#include <string.h>
#include <glib.h>

struct filter_data {
  const char **in;
  const char *in_end;
  GString *out_str;
  GMainLoop *loop;
  int err;
};

static gboolean filter_stdin(GIOChannel *channel, GIOCondition condition, gpointer data_)
{
  struct filter_data *data = data_;
  gboolean done = condition & (G_IO_ERR | G_IO_HUP);

  if (condition & G_IO_OUT) {
    gsize n;
    GIOStatus ret = g_io_channel_write_chars(channel, *data->in, data->in_end - *data->in, &n, NULL);
    *data->in += n;
    if (ret == G_IO_STATUS_ERROR)
      data->err = 1;
    if (ret == G_IO_STATUS_ERROR || *data->in == data->in_end)
      done = TRUE;
  }

  if (condition & G_IO_ERR)
    data->err = 1;

  if (done)
    g_io_channel_shutdown(channel, TRUE, NULL);
  return !done;
}

static gboolean filter_stdout(GIOChannel *channel, GIOCondition condition, gpointer data_)
{
  struct filter_data *data = data_;
  gboolean done = condition & (G_IO_ERR | G_IO_HUP);

  if (condition & (G_IO_IN | G_IO_HUP)) {
    gchar *buf;
    gsize n;
    GIOStatus ret = g_io_channel_read_to_end(channel, &buf, &n, NULL);
    g_string_append_len(data->out_str, buf, n);
    g_free(buf);
    if (ret == G_IO_STATUS_ERROR) {
      data->err = 1;
      done = TRUE;
    }
  }

  if (condition & G_IO_ERR)
    data->err = 1;

  if (done) {
    g_io_channel_shutdown(channel, TRUE, NULL);
    g_main_loop_quit(data->loop);
  }
  return !done;
}

int call_filter(const char *const *argv, const char *in, char **out, int *status)
{
  GPid child_pid;
  int child_stdin, child_stdout;

  if (!g_spawn_async_with_pipes(NULL, (char**)argv, NULL,
                                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                NULL, NULL,
                                &child_pid, &child_stdin, &child_stdout, NULL,
                                NULL)) {
    *out = NULL;
    return 1;
  }

  if (in == NULL) in = "";
  GMainContext *context = g_main_context_new();
  struct filter_data data = {&in, in + strlen(in), g_string_new(""), g_main_loop_new(context, FALSE), 0};

  GIOChannel *channel = g_io_channel_unix_new(child_stdin);
  g_io_channel_set_encoding(channel, NULL, NULL);
  g_io_channel_set_flags(channel, g_io_channel_get_flags(channel) | G_IO_FLAG_NONBLOCK, NULL);
  GSource *source = g_io_create_watch(channel, G_IO_OUT | G_IO_ERR | G_IO_HUP);
  g_io_channel_unref(channel);
  g_source_set_callback(source, (GSourceFunc)filter_stdin, &data, NULL);
  g_source_attach(source, context);
  g_source_unref(source);

  channel = g_io_channel_unix_new(child_stdout);
  g_io_channel_set_encoding(channel, NULL, NULL);
  g_io_channel_set_flags(channel, g_io_channel_get_flags(channel) | G_IO_FLAG_NONBLOCK, NULL);
  source = g_io_create_watch(channel, G_IO_IN | G_IO_ERR | G_IO_HUP);
  g_io_channel_unref(channel);
  g_source_set_callback(source, (GSourceFunc)filter_stdout, &data, NULL);
  g_source_attach(source, context);
  g_source_unref(source);

  g_main_loop_run(data.loop);

  g_main_loop_unref(data.loop);
  g_main_context_unref(context);

  waitpid(child_pid, status, 0);
  g_spawn_close_pid(child_pid);
  *out = g_string_free(data.out_str, data.err);
  return data.err;
}
