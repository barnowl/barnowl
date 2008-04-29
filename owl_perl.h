#ifndef INC_OWL_PERL_H
#define INC_OWL_PERL_H

#define OWL_PERL_VOID_CALL (void)POPs;

/*
 * This macro defines a convenience wrapper around the boilerplate of
 * calling a method on a perl object (SV*) from C.
 *
 * Arguments are
 * * obj    - the SV* to call the method on
 * * meth   - a char* method name
 * * args   - a code block responsible for pushing args (other than the object)
 * * err    - a string with a %s format specifier to log in case of error
 * * fatalp - if true, perl errors terminate barnowl
 * * ret    - a code block executed if the call succeeded
 *
 * See also: `perldoc perlcall', `perldoc perlapi'
 */
#define OWL_PERL_CALL_METHOD(obj, meth, args, err, fatalp, ret) { \
    int count; \
    dSP; \
    ENTER; \
    SAVETMPS; \
    PUSHMARK(SP); \
    XPUSHs(obj); \
    {args} \
    PUTBACK; \
    \
    count = call_method(meth, G_SCALAR|G_EVAL); \
    \
    SPAGAIN; \
    \
    if(count != 1) { \
      fprintf(stderr, "perl returned wrong count: %d\n", count); \
      abort();                                                   \
    } \
    if (SvTRUE(ERRSV)) { \
      if(fatalp) { \
        printf(err, SvPV_nolen(ERRSV)); \
        exit(-1); \
      } else { \
        owl_function_error(err, SvPV_nolen(ERRSV)); \
        (void)POPs; \
        sv_setsv(ERRSV, &PL_sv_undef); \
      } \
    } else { \
      ret; \
    } \
    PUTBACK; \
    FREETMPS; \
    LEAVE; \
}

#endif //INC_PERL_PERL_H
