use warnings;
use strict;

package BarnOwl::Help;

use BarnOwl::Parse qw(tokenize);
use BarnOwl::Editwin qw(text_before_point text_after_point);

sub show_help {
    my $cmd = shift;

    my $words = tokenize(text_before_point() . text_after_point());
    BarnOwl::help($words->[0]) if @$words;
}

1;
