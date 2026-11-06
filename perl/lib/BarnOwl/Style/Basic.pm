use strict;
use warnings;

package BarnOwl::Style::Basic;
# Inherit format_message to dispatch
use base qw(BarnOwl::Style::Default);

sub description {"Compatibility alias for the default style";}

BarnOwl::create_style("basic", "BarnOwl::Style::Basic");


1;
