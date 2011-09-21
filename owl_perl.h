#ifndef INC_BARNOWL_OWL_PERL_H
#define INC_BARNOWL_OWL_PERL_H

#include <stdio.h>

/*
 * This macro defines a convenience wrapper around the boilerplate
 * of pushing char * arguments on to the stack for perl calling.
 *
 * Arguments are
 * * i     - the counter variable to use, which must be declared prior
 *           to calling this macro
 * * argc  - the number of arguments
 * * argv  - an array of char*s, of length at least argc; the arguments
 *           to push on to the stack
 */
#define OWL_PERL_PUSH_ARGS(i, argc, argv) { \
  for (i = 0; i < argc; i++) { \
    XPUSHs(sv_2mortal(owl_new_sv(argv[i]))); \
  } \
}

/*
 * This macro defines a convenience wrapper around the boilerplate of
 * the perlcall methods.
 *
 * Arguments are
 * * call       - the line of code to make the perl call
 * * args       - a code block responsible for pushing args
 * * err        - a string with a %s format specifier to log in case of error
 * * fatalp     - if true, perl errors terminate BarnOwl
 * * discardret - should be true if no return is expected
 *                (if the call is passed the flag G_DISCARD or G_VOID)
 * * ret        - a code block executed if the call succeeded
 *
 * See also: `perldoc perlcall', `perldoc perlapi'
 */
#define OWL_PERL_CALL(call, args, err, fatalp, discardret, ret) { \
  int count; \
  dSP; \
  \
  ENTER; \
  SAVETMPS; \
  \
  PUSHMARK(SP); \
  {args} \
  PUTBACK; \
  \
  count = call; \
  \
  SPAGAIN; \
  \
  if (!discardret && count != 1) { \
    croak("Perl returned wrong count: %d\n", count); \
  } \
  \
  if (SvTRUE(ERRSV)) { \
    if (fatalp) { \
      fprintf(stderr, err, SvPV_nolen(ERRSV)); \
      exit(-1); \
    } else { \
      owl_function_error(err, SvPV_nolen(ERRSV)); \
      if (!discardret) (void)POPs; \
      sv_setsv(ERRSV, &PL_sv_undef); \
    } \
  } else if (!discardret) { \
    ret; \
  } \
  PUTBACK; \
  FREETMPS; \
  LEAVE; \
}

#endif /* INC_BARNOWL_OWL_PERL_H */
