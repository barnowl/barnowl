
# $Id$

if ($#ARGV eq -1) {
    @ARGV=`ls *.c`;
    chop(@ARGV);
}

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
	    && !/\/\*/)
	{s/\s+\{/\;/; s/\)[ \t]*$/\)\;/; print "extern "; print;}
	    
    }
    close(FILE);
    print "\n";
}
