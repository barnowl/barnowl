#! /usr/bin/perl

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
	    && !/ZWRITEOPTIONS/
	    && !/owlfaim_priv/)
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


