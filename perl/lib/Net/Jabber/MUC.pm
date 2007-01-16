package Net::Jabber::MUC;

=head1 NAME

Net::Jabber::Roster - Jabber Multi-User Chat Object

=head1 SYNOPSIS

  my $Client = Net::XMPP:Client->new(...);

  my $muc = Net::Jabber::MUC(connection => $Client,
                             room   => "jabber",
                             server => "conference.jabber.org",
                             nick   => "nick");
   or
  my $muc = Net::Jabber::MUC(connection => $Client,
                             jid    =>  'jabber@conference.jabber.org/nick');


  $muc->Join(Password => "secret", History => {MaxChars => 0});

  if( $muc->Contains($JID) ) { ... }
  my @jids = $muc->Presence();

  $muc->Leave();

=head1 DESCRIPTION

The MUC object seeks to provide a simple API for interfacing with
Jabber multi-user chats (as defined in XEP 0045). It automatically
registers callbacks with the connections to keep track of presence in
the MUC.


=cut

use strict;
use warnings;

sub new {
    my $class = shift;
    my $self = { };

    my %args;
    while($#_ >= 0) { $args{ lc(pop(@_)) } = pop(@_); }

    if (!exists($args{connection}) ||
        !$args{connection}->isa("Net::XMPP::Connection"))
    {
        croak("You must pass Net::Jabber::MUC a valid connection object.");
    }

    if($args{jid}) {
        my $jid = $args{jid};
        $jid = Net::Jabber::JID->new($jid) unless UNIVERSAL::isa($jid, 'Net::Jabber::JID');
        $args{jid} = $jid;
    } elsif($args{room} && $args{server} && $args{nick}) {
        $args{jid} = New::Jabber::JID->new($args{room}."@".$args{server}."/".$args{nick});
    } else {
        croak("You must specify either a jid or room,server,nick.");
    }

    $self->{CONNECTION} = $args{connection};
    $self->{JID} = $args{jid};
    $self->{PRESENCE} = { };

    bless($self, $class);

    $self->_init;

    return $self;
}

=head2 JID

Returns the Net::Jabber::JID object representing this MUC's JID
(room@host/nick)

=cut

sub JID {
    my $self = shift;
    return $self->{JID};
}

=head2 BaseJID

Returns the base JID of this MUC as a string

=cut

sub BaseJID {
    my $self = shift;
    return $self->JID->GetJID('base');
}


=head2 _init

Add callbacks to our connection to receive the appropriate packets.

=cut

sub _init {
    my $self = shift;

    $self->{CONNECTION}->SetXPathCallBacks('/presence' => sub { $self->_handler(@_) });
}

=head2 Join

Sends the appropriate presence packet to join us to the MUC.

=cut

sub Join {
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc(pop(@_)) } = pop(@_); }

    my $presence = Net::Jabber::Presence->new;
    $presence->SetTo($self->JID);
    my $x = $presence->NewChild('http://jabber.org/protocol/muc');
    if($args{password}) {
        $x->SetPassword($args{password});
    }
    if($args{history}) {
        my $h = $x->AddHistory();
        if($args{history}{MaxChars}) {
            $h->SetMaxChars($args{history}{MaxChars});
        } elsif($args{history}{MaxStanzas}) {
            $h->SetMaxStanzas($args{history}{MaxStanzas});
        } elsif($args{history}{Seconds}) {
            $h->SetSeconds($args{history}{Seconds});
        } elsif($args{history}{Since}) {
            $h->SetSince($args{history}{Since});
        }
    }
    $self->{CONNECTION}->Send($presence);
}

=head2 Leave

Leaves the MUC

=cut

sub Leave {
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc(pop(@_)) } = pop(@_); }

    $self->{CONNECTION}->PresenceSend(to => $self->JID, type => 'unavailable');
    $self->{PRESENCE} = {};
}


=head2 _handler

Central dispatch point for handling incoming packets.

=cut

sub _handler {
    my $self = shift;
    my $sid = shift;
    my $packet = shift;

    $self->_handlePresence($packet) if $packet->GetTag() eq "presence";
}

=head2 handlePresence

Handle an incoming presence packet.

=cut

sub _handlePresence {
    my $self = shift;
    my $presence = shift;

    my $type = $presence->GetType() || "available";
    my $from = Net::Jabber::JID->new($presence->GetFrom());

    return unless $from->GetJID('base') eq $self->BaseJID;

    if($type eq 'unavailable') {
        delete $self->{PRESENCE}->{$from->GetJID('full')};
    } elsif($type eq 'available') {
        $self->{PRESENCE}->{$from->GetJID('full')} = $from;
    }
}

=head2 Contains JID

Returns true iff the MUC contains the specified full JID

=cut

sub Contains {
    my $self = shift;
    my $jid = shift;

    $jid = $jid->GetJID('full') if UNIVERSAL::isa($jid, 'Net::Jabber::JID');

    return exists $self->{PRESENCE}->{$jid};
}

=head2 Presence

Returns a list of JIDS in the MUC, as Net::Jabber::JID objects

=cut

sub Presence {
    my $self = shift;
    return values %{$self->{PRESENCE}};
}

1;
