use warnings;
use strict;

=head1 NAME

BarnOwl::Module::Jabber::Connection

=head1 DESCRIPTION

A subclass of L<Net::Jabber::Client> used in the BarnOwl jabber module

=cut

package BarnOwl::Module::Jabber::Connection;

use base qw(Net::Jabber::Client);

use Net::Jabber;

sub new {
    my $class = shift;

    my %args = ();
    if(BarnOwl::getvar('debug') eq 'on') {
        $args{debuglevel} = 1;
        $args{debugfile} = 'jabber.log';
    }
    my $self = $class->SUPER::new(%args);
    $self->{_BARNOWL_MUCS} = [];
    return $self;
}

=head2 MUCJoin

Extends MUCJoin to keep track of the MUCs we're joined to as
Net::Jabber::MUC objects. Takes the same arguments as
L<Net::Jabber::MUC/new> and L<Net::Jabber::MUC/Connect>

=cut

sub MUCJoin {
    my $self = shift;
    my $muc = Net::Jabber::MUC->new(connection => $self, @_);
    $muc->Join(@_);
    push @{$self->MUCs}, $muc;
}

=head2 MUCLeave ARGS

Leave a MUC. The MUC is specified in the same form as L</FindMUC>

=cut

sub MUCLeave {
    my $self = shift;
    my $muc = $self->FindMUC(@_);
    return unless $muc;

    $muc->Leave();
    $self->{_BARNOWL_MUCS} = [grep {$_->BaseJID ne $muc->BaseJID} $self->MUCs];
}

=head2 FindMUC ARGS

Return the Net::Jabber::MUC object representing a specific MUC we're
joined to, undef if it doesn't exists. ARGS can be either JID => $JID,
or Room => $room, Server => $server.

=cut

sub FindMUC {
    my $self = shift;

    my %args;
    while($#_ >= 0) { $args{ lc(pop(@_)) } = pop(@_); }

    my $jid;
    if($args{jid}) {
        $jid = $args{jid};
    } elsif($args{room} && $args{server}) {
        $jid = Net::Jabber::JID->new(userid => $args{room},
                                     server => $args{server});
    }
    $jid = $jid->GetJID('base') if UNIVERSAL::isa($jid, 'Net::XMPP::JID');

    foreach my $muc ($self->MUCs) {
        return $muc if $muc->BaseJID eq $jid;
    }
    return undef;
}

=head2 MUCs

Returns a list (or arrayref in scalar context) of Net::Jabber::MUC
objects we believe ourself to be connected to.

=cut

sub MUCs {
    my $self = shift;
    my $mucs = $self->{_BARNOWL_MUCS};
    return wantarray ? @$mucs : $mucs;
}


=head1 SEE ALSO

L<Net::Jabber::Client>, L<BarnOwl::Module::Jabber>

=cut

1;
