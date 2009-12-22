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

    BarnOwl::remove_io_dispatch($self->{$jidStr}->{Client}->{fileno}) if $self->{$jidStr}->{Client}->{fileno};
    $self->{$jidStr}->{Client}->Disconnect()
      if $self->{$jidStr}->{Client};
    delete $self->{$jidStr};

    return 1;
}

sub scheduleReconnect {
    my $self = shift;
    my $jidStr = shift;
    return 0 unless exists $self->{$jidStr};
    BarnOwl::admin_message(Jabber => "Disconnected from jabber account $jidStr");

    unless (BarnOwl::getvar('jabber:reconnect') eq 'on') {
        return $self->removeConnection($jidStr);
    }

    BarnOwl::remove_io_dispatch($self->{$jidStr}->{Client}->{fileno}) if $self->{$jidStr}->{Client}->{fileno};
    $self->{$jidStr}->{Client}->Disconnect()
      if $self->{$jidStr}->{Client};

    $self->{$jidStr}->{Status} = "reconnecting";
    $self->{$jidStr}->{ReconnectBackoff} = 5;
    $self->{$jidStr}->{ReconnectAt} = time + $self->{$jidStr}->{ReconnectBackoff};
    return 1;
}

sub setAuth {
    my $self = shift;
    my $jidStr = shift;
    $self->{$jidStr}->{Auth} = shift;
}

sub tryReconnect {
    my $self = shift;
    my $jidStr = shift;
    my $force = shift;

    return 0 unless exists $self->{$jidStr};
    return 0 unless $self->{$jidStr}{Status} eq "reconnecting";
    return 0 unless $force or (time > $self->{$jidStr}{ReconnectAt});

    $self->{$jidStr}->{ReconnectBackoff} *= 2;
    $self->{$jidStr}->{ReconnectBackoff} = 60*5
        if $self->{$jidStr}->{ReconnectBackoff} > 60*5;
    $self->{$jidStr}->{ReconnectAt} = time + $self->{$jidStr}->{ReconnectBackoff};

    my $client = $self->{$jidStr}->{Client};
    my $status = $client->Connect;
    return 0 unless $status;

    my @result = $client->AuthSend( %{ $self->{$jidStr}->{Auth} } );
    if ( !@result || $result[0] ne 'ok' ) {
        $self->removeConnection($jidStr);
        BarnOwl::error( "Error in jabber reconnect: " . join( " ", @result ) );
        return 0;
    }
    $self->{$jidStr}->{Roster} = $client->Roster();
    $self->{$jidStr}->{Status} = "available";
    $client->onConnect($self, $jidStr);
    foreach my $muc ($client->MUCs()) {
        $muc->Join($muc->{ARGS});
    }

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

sub baseJIDExists {
    my $self = shift;
    my $jidStr = shift;
    my $jid = new Net::Jabber::JID;
    $jid->SetJID($jidStr);
    my $baseJID = $jid->GetJID('base');

    foreach my $cjidStr ( keys %{ $self } ) {
        my $cJID = new Net::Jabber::JID;
        $cJID->SetJID($cjidStr);
        return $cjidStr if ($cJID->GetJID('base') eq $baseJID);
    }
    return 0;
}

sub jidActive {
    my $self = shift;
    my $jidStr = shift;
    return(exists $self->{$jidStr} and $self->{$jidStr}{Status} eq "available");
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
    return $self->{$jid}->{Client} if $self->jidActive($jid);
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
    return $self->{$jid}->{Roster} if $self->jidExists($jid);
}


1;
