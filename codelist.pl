#! /usr/bin/perl
# $Id$

if ($#ARGV eq -1) {
    @ARGV=`ls *.c`;
    chop(@ARGV);
}

foreach $file (@ARGV) {
    next if $file eq 'perlglue.c' || $file eq 'owl.c';
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


