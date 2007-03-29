#line 1
use warnings;
use strict;

#line 32

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
\tcd blib; zip ../$name.par -r arch lib

END_MAKEFILE
}

#line 60

1;
