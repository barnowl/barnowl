package BarnOwl;
use strict;
use warnings;

no warnings 'redefine';
sub debug($) {
    warn "[DEBUG] ", shift, "\n" if $ENV{TEST_VERBOSE};
}

1;
