use warnings;
use strict;

=head1 NAME

BarnOwl::Zephyr

=cut

package BarnOwl::Zephyr;

use BarnOwl::Hook;

my $zephyrStartup = BarnOwl::Hook->new;

sub _zephyr_startup {
    $zephyrStartup->run;
}

1;
