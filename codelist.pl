#! /usr/bin/perl

my $guard_symbol = "INC_BARNOWL_OWL_PROTOTYPES_H";
print "#ifndef $guard_symbol\n";
print "#define $guard_symbol\n";
foreach $file (@ARGV) {
    open(FILE, $file);

    print "/* -------------------------------- $file -------------------------------- */\n";
    while (<FILE>) {
	if (/^\S/
	    && (/\{\s*$/ || /\)\s*$/)
	    && !/\}/
	    && !/^\{/
	    && !/^#/
	    && !/^static/
	    && !/^system/
	    && !/^XS/
	    && !/\/\*/
	    && !/ZWRITEOPTIONS/)
	{

	    s/\s+\{/\;/;
	    s/\)[ \t]*$/\)\;/;
	    print "extern ";
	    print;
	} elsif (/^#if/ || /^#else/ || /^#endif/) {
	    print;
	}
	    
    }
    close(FILE);
    print "\n";
}
print "#endif /* $guard_symbol */\n";
