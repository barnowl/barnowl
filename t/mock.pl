use File::Basename;
use lib (dirname($0) . '/../perl/lib');

package BarnOwl;
use strict;
use warnings;
use Carp;

sub bootstrap {}
sub get_data_dir {"."}
sub get_config_dir {"."}
sub create_style {}

sub debug {
    warn "[DEBUG] ", shift, "\n" if $ENV{TEST_VERBOSE};
}

sub BarnOwl::Internal::new_command {}
sub BarnOwl::Internal::new_variable_bool {}
sub BarnOwl::Internal::new_variable_int {}
sub BarnOwl::Internal::new_variable_string {}
sub BarnOwl::Editwin::save_excursion(&) {}

use BarnOwl;

1;
