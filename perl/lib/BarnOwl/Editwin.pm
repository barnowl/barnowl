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
                    point_move replace_region save_excursion);

1;
