use warnings;
use strict;

=head1 NAME

Module::Install::BarnOwl

=head1 DESCRIPTION

Module::Install::BarnOwl is a M::I module to help building barnowl
modules,

=head1 SYNOPSIS

    use inc::Module::Install;
    barnowl_module('Jabber');
    WriteAll;

This is roughly equivalent to:

    use inc::Module::Install;

    name('BarnOwl-Module-Jabber');
    all_from('lib/BarnOwl/Module/Jabber.pm');
    requires_external_bin('barnowl');

    WriteAll;

As well as make rules to generate Jabber.par, and to put some
additional barnowl-specific information into META.yml

=cut

package Module::Install::BarnOwl;

use base qw(Module::Install::Base);

sub barnowl_module {
    my $self = shift;
    my $name = ucfirst shift;
    my $class = ref $self;

    $self->name("BarnOwl-Module-$name");
    $self->all_from("lib/BarnOwl/Module/$name.pm");

    $self->postamble(<<"END_MAKEFILE");

# --- $class section:

$name.par: pm_to_blib
\tcd blib; zip -q ../$name.par -r arch lib

END_MAKEFILE
}

=head1 SEE ALSO

L<Module::Install>, L<BarnOwl>

=cut

1;
