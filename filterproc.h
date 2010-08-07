#ifndef INC_BARNOWL_FILTER_PROC_H
#define INC_BARNOWL_FILTER_PROC_H

int call_filter(const char *prog,
                const char *const *argv,
                const char *in,
                char **out, int *status);

#endif /* INC_BARNOWL_FILTER_PROC_H */
