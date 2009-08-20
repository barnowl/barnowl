use strict;
use warnings;

package BarnOwl::Message::AIM;

use base qw( BarnOwl::Message );

# all non-loginout AIM messages are private for now...
sub is_private {
    return !(shift->is_loginout);
}

sub replycmd {
    my $self = shift;
    if ($self->is_incoming) {
        return BarnOwl::quote('aimwrite', $self->sender);
    } else {
        return BarnOwl::quote('aimwrite', $self->recipient);
    }
}

sub replysendercmd {
    return shift->replycmd;
}


1;
