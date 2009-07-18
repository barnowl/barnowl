use warnings;
use strict;

=head1 NAME

BarnOwl::Editwin

=head1 DESCRIPTION

Functions for interfacing with the BarnOwl editwin. Most of the
functions in this module are defined in perlglue.xs; This module
exists to provide Exporter hooks and documentation for them.

=cut

package BarnOwl::Editwin;
use base qw(Exporter);

our @EXPORT_OK = qw(text_before_point text_after_point replace
                    point_move replace_region get_region
                    save_excursion current_column point mark);
our %EXPORT_TAGS = (all => \@EXPORT_OK);

sub text_before_point {
    save_excursion {
        set_mark();
        move_to_buffer_start();
        get_region();
    }
}

sub text_after_point {
    save_excursion {
        set_mark();
        move_to_buffer_end();
        get_region();
    }
}

1;
