use warnings;
use strict;
use utf8;

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
    $muc->{ARGS} = @_; # Save these for later
    $muc->Join(@_);

    # Add MUC to list of MUCs, unless we're just changing nicks.
    push @{$self->MUCs}, $muc unless grep {$_->BaseJID eq $muc->BaseJID} $self->MUCs;
}

=head2 MUCLeave ARGS

Leave a MUC. The MUC is specified in the same form as L</FindMUC>

Returns true if successful, false if this connection was not in the
named MUC.

=cut

sub MUCLeave {
    my $self = shift;
    my $muc = $self->FindMUC(@_);
    return unless $muc;

    $muc->Leave();
    $self->{_BARNOWL_MUCS} = [grep {$_->BaseJID ne $muc->BaseJID} $self->MUCs];
    return 1;
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


=head2 getSID

Returns the StreamID for this connection.

=cut

sub getStreamID {
    my $self = shift;
    return $self->{SESSION}->{id} || "";
}

=head2 getSocket

Returns the IO::Socket for this connection.

=cut

sub getSocket {
    my $self = shift;
    my $sid = getStreamID($self);
    return $self->{STREAM}->GetSock($sid) || -1;
}

=head2 OwlProcess

Non-blocking connection processing. For use in a select loop.

=cut

sub OwlProcess {
    my $self = shift;
    my $jid = shift || $self->{SESSION}->{FULLJID};
    my $status = $self->Process(0);
    if ( !defined($status) ) {
        $BarnOwl::Module::Jabber::conn->scheduleReconnect($jid);
    }
}

=head2 Disconnect

Work around a bug in Net::Jabber::Client where Process' return status
is not cleared on disconnect.

=cut

sub Disconnect {
    my $self = shift;
    delete $self->{PROCESSERROR};
    return $self->SUPER::Disconnect(@_);
}

=head2 OnConnect

Actions to perform on connecting and reconnecting.

=cut

sub onConnect {
    my $self = shift;
    my $conn = shift;
    my $jidStr = shift;

    my $fullJid = $self->{SESSION}->{FULLJID} || $jidStr;
    my $roster = $conn->getRosterFromJID($jidStr);

    $roster->fetch();
    $self->PresenceSend( priority => 1 );

    $conn->renameConnection($jidStr, $fullJid);
    BarnOwl::admin_message('Jabber', "Connected to jabber as $fullJid");
    # The remove_io_dispatch() method is called from the
    # ConnectionManager's removeConnection() method.
    $self->{fileno} = $self->getSocket()->fileno();
    BarnOwl::add_io_dispatch($self->{fileno}, 'r', sub { $self->OwlProcess($fullJid) });

    # populate completion from roster.
    for my $buddy ( $roster->jids('all') ) {
        my %jq  = $roster->query($buddy);
        my $name = $jq{name} || $buddy->GetUserID();
        $BarnOwl::Module::Jabber::completion_jids{$name} = 1;
        $BarnOwl::Module::Jabber::completion_jids{$buddy->GetJID()} = 1;
    }
    $BarnOwl::Module::Jabber::vars{idletime} |= BarnOwl::getidletime();
    unless (exists $BarnOwl::Module::Jabber::vars{keepAliveTimer}) {
        $BarnOwl::Module::Jabber::vars{keepAliveTimer} =
            BarnOwl::Timer->new({
                'after' => 5,
                'interval' => 5,
                'cb' => sub { BarnOwl::Module::Jabber::do_keep_alive_and_auto_away(@_) }
                                });
    }
}

=head1 SEE ALSO

L<Net::Jabber::Client>, L<BarnOwl::Module::Jabber>

=cut

1;
