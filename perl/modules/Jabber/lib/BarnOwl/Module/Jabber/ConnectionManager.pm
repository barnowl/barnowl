use warnings;
use strict;

=head1 NAME

BarnOwl::Module::Jabber::ConnectionManager

=head1 DESCRIPTION

A class to keep track of all the active connection in the barnowl
jabber module

=cut

package BarnOwl::Module::Jabber::ConnectionManager;

sub new {
    my $class = shift;
    return bless { }, $class;
}

sub addConnection {
    my $self = shift;
    my $jidStr = shift;

    my $client = BarnOwl::Module::Jabber::Connection->new;

    $self->{$jidStr}->{Client} = $client;
    $self->{$jidStr}->{Roster} = $client->Roster();
    $self->{$jidStr}->{Status} = "available";
    return $client;
}

sub removeConnection {
    my $self = shift;
    my $jidStr = shift;
    return 0 unless exists $self->{$jidStr};

    BarnOwl::remove_dispatch($self->{$jidStr}->{Client}->{fileno}) if $self->{$jidStr}->{Client}->{fileno};
    $self->{$jidStr}->{Client}->Disconnect()
      if $self->{$jidStr}->{Client};
    delete $self->{$jidStr};

    return 1;
}

sub renameConnection {
    my $self = shift;
    my $oldJidStr = shift;
    my $newJidStr = shift;
    return 0 unless exists $self->{$oldJidStr};
    return 0 if $oldJidStr eq $newJidStr;

    $self->{$newJidStr} = $self->{$oldJidStr};
    delete $self->{$oldJidStr};
    return 1;
}

sub connected {
    my $self = shift;
    return scalar keys %{ $self };
}

sub getJIDs {
    my $self = shift;
    return keys %{ $self };
}

sub jidExists {
    my $self = shift;
    my $jidStr = shift;
    return exists $self->{$jidStr};
}

sub sidExists {
    my $self = shift;
    my $sid = shift || "";
    foreach my $c ( values %{ $self } ) {
        return 1 if ($c->{Client}->{SESSION}->{id} eq $sid);
    }
    return 0;
}

sub getConnectionFromSid {
    my $self = shift;
    my $sid = shift;
    foreach my $c (values %{ $self }) {
        return $c->{Client} if $c->{Client}->{SESSION}->{id} eq $sid;
    }
    return undef;
}

sub getConnectionFromJID {
    my $self = shift;
    my $jid = shift;
    $jid = $jid->GetJID('full') if UNIVERSAL::isa($jid, 'Net::XMPP::JID');
    return $self->{$jid}->{Client} if exists $self->{$jid};
}

sub getRosterFromSid {
    my $self = shift;
    my $sid = shift;
    foreach my $c (values %{ $self }) {
        return $c->{Roster}
          if $c->{Client}->{SESSION}->{id} eq $sid;
    }
    return undef;
}

sub getRosterFromJID {
    my $self = shift;
    my $jid = shift;
    $jid = $jid->GetJID('full') if UNIVERSAL::isa($jid, 'Net::XMPP::JID');
    return $self->{$jid}->{Roster} if exists $self->{$jid};
}


1;
