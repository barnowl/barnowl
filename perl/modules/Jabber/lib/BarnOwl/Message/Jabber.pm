use warnings;
use strict;

=head1 NAME

BarnOwl::Message::Jabber

=head1 DESCRIPTION

A subclass of BarnOwl::Message for Jabber messages

=cut

package BarnOwl::Message::Jabber;

use base qw( BarnOwl::Message );

sub jtype { shift->{jtype} };
sub from { shift->{from} };
sub to { shift->{to} };
sub room { shift->{room} };
sub status { shift->{status} }

sub login_extra {
    my $self = shift;
    my $show = $self->{show};
    my $status = $self->status;
    my $s = "";
    $s .= $show if $show;
    $s .= ", $status" if $status;
    return $s;
}

sub long_sender {
    my $self = shift;
    return $self->from;
}

sub context {
    return shift->room;
}

sub smartfilter {
    my $self = shift;
    my $inst = shift;

    my ($filter, $ftext);

    if($self->jtype eq 'chat') {
        my $user;
        if($self->direction eq 'in') {
            $user = $self->from;
        } else {
            $user = $self->to;
        }
        return smartfilter_user($user, $inst);
    } elsif ($self->jtype eq 'groupchat') {
        my $room = $self->room;
        $filter = "jabber-room-$room";
        $ftext = qq{type ^jabber\$ and room ^$room\$};
        BarnOwl::filter("$filter $ftext");
        return $filter;
    } elsif ($self->login ne 'none') {
        return smartfilter_user($self->from, $inst);
    }
}

sub smartfilter_user {
    my $user = shift;
    my $inst = shift;

    $user   = Net::Jabber::JID->new($user)->GetJID( $inst ? 'full' : 'base' );
    my $filter = "jabber-user-$user";
    my $ftext  =
        qq{type ^jabber\$ and ( ( direction ^in\$ and from ^$user ) }
      . qq{or ( direction ^out\$ and to ^$user ) ) };
    BarnOwl::filter("$filter $ftext");
    return $filter;

}


=head1 SEE ALSO

L<BarnOwl::Message>

=cut

1;
