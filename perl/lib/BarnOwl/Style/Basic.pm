use strict;
use warnings;

package BarnOwl::Style::Basic;
our @ISA=qw(BarnOwl::Style::Default);

sub description {"Compatability alias for the default style";}

BarnOwl::create_style("basic", "BarnOwl::Style::Basic");


1;
