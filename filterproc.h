#ifndef __FILTER_PROC_H__
#define __FILTER_PROC_H__

int call_filter(const char *prog,
                const char *const *argv,
                const char *in,
                char **out, int *status);

#endif
