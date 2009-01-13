use warnings;
use strict;

=head1 NAME

BarnOwl::Message::Twitter

=head1 DESCRIPTION

=cut

package BarnOwl::Message::Twitter;
use base qw(BarnOwl::Message);

sub context {'twitter'}
sub subcontext {undef}
sub long_sender {"http://twitter.com/" . shift->sender}

sub replycmd {
    my $self = shift;
    if($self->is_private) {
        return $self->replysendercmd;
    } else {
        return 'twitter';
    }
}

sub replysendercmd {
    my $self = shift;
    return 'twitter-direct ' . $self->sender;
}

sub smartfilter {
    my $self = shift;
    my $inst = shift;
    my $filter;

    if($inst) {
        $filter = "twitter-" . $self->sender;
        BarnOwl::command("filter", $filter,
                         qw{type ^twitter$ and sender}, '^'.$self->sender.'$');
    } else {
        $filter = "twitter";
    }
    return $filter;
}

=head1 SEE ALSO

Foo, Bar, Baz

=cut

1;
