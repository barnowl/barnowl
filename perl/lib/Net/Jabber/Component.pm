##############################################################################
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the
#  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA  02111-1307, USA.
#
#  Copyright (C) 1998-2004 Jabber Software Foundation http://jabber.org/
#
##############################################################################

package Net::Jabber::Component;

=head1 NAME

Net::Jabber::Component - Jabber Component Library

=head1 SYNOPSIS

  Net::Jabber::Component is a module that provides a developer easy
  access to developing server components in the Jabber Instant Messaging
  protocol.

=head1 DESCRIPTION

  Component.pm seeks to provide enough high level APIs and automation of
  the low level APIs that writing a Jabber Component in Perl is trivial.
  For those that wish to work with the low level you can do that too,
  but those functions are covered in the documentation for each module.

  Net::Jabber::Component provides functions to connect to a Jabber
  server, login, send and receive messages, operate as a server side
  component, and disconnect.  You can use all or none of the functions,
  there is no requirement.

  For more information on how the details for how Net::Jabber is written
  please see the help for Net::Jabber itself.

  For a full list of high level functions available please see
  Net::Jabber::Protocol and Net::XMPP::Protocol.

=head2 Basic Functions

    use Net::Jabber;

    $Con = new Net::Jabber::Component();

    $Con->Execute(hostname=>"jabber.org",
                  componentname=>"service.jabber.org",
                  secret=>"XXXX"
                 );

    #
    # For the list of available functions see Net::XMPP::Protocol.
    #

    $Con->Disconnect();

=head1 METHODS

=head2 Basic Functions

    new(debuglevel=>0|1|2, - creates the Component object.  debugfile
        debugfile=>string,   should be set to the path for the debug
        debugtime=>0|1)      log to be written.  If set to "stdout"
                             then the debug will go there.  debuglevel
                             controls the amount of debug.  For more
                             information about the valid setting for
                             debuglevel, debugfile, and debugtime see
                             Net::Jabber::Debug.

    AuthSend(secret=>string) - Perform the handshake and authenticate
                               with the server.

    Connect(hostname=>string,       - opens a connection to the server
	        port=>integer,            based on the value of
	        componentname=>string,    connectiontype.  The only valid
	        connectiontype=>string)   setting is:
	                                    accept - TCP/IP remote connection
                                      In the future this might be used
                                      again by offering new features.
                                      If accept then it connects to the
                                      server listed in the hostname
                                      value, on the port listed.  The
                                      defaults for the two are localhost
                                      and 5269.
                                      
                                      Note: A change from previous
                                      versions is that Component now
                                      shares its core with Client.  To
                                      that end, the secret should no
                                      longer be used.  Call AuthSend
                                      after connecting.  Better yet,
                                      use Execute.

    Connected() - returns 1 if the Component is connected to the server,
                  and 0 if not.

    Disconnect() - closes the connection to the server.

    Execute(hostname=>string,       - Generic inner loop to handle
	        port=>int,                connecting to the server, calling
	        secret=>string,           Process, and reconnecting if the
	        componentname=>string,    connection is lost.  There are four
	        connectiontype=>string,   callbacks available that are called
            connectattempts=>int,     at various places in the loop.
            connectsleep=>int)          onconnect - when the component
                                                    connects to the
                                                    server.
                                        onauth - when the component has
                                                 completed its handshake
                                                 with the server this
                                                 will be called.
                                        onprocess - this is the most
                                                    inner loop and so
                                                    gets called the most.
                                                    Be very very careful
                                                    what you put here
                                                    since it can
                                                    *DRASTICALLY* affect
                                                    performance.
                                        ondisconnect - when connection is
                                                       lost.
                                        onexit - when the function gives
                                                 up trying to connect and
                                                 exits.
                                      The arguments are passed straight
                                      on to the Connect function, except
                                      for connectattempts and
                                      connectsleep.  connectattempts is
                                      the number of time that the
                                      Component should try to connect
                                      before giving up.  -1 means try
                                      forever.  The default is -1.
                                      connectsleep is the number of
                                      seconds to sleep between each
                                      connection attempt.

    Process(integer) - takes the timeout period as an argument.  If no
                       timeout is listed then the function blocks until
                       a packet is received.  Otherwise it waits that
                       number of seconds and then exits so your program
                       can continue doing useful things.  NOTE: This is
                       important for GUIs.  You need to leave time to
                       process GUI commands even if you are waiting for
                       packets.  The following are the possible return
                       values, and what they mean:

                           1   - Status ok, data received.
                           0   - Status ok, no data received.
                         undef - Status not ok, stop processing.
                       
                       IMPORTANT: You need to check the output of every
                       Process.  If you get an undef then the connection
                       died and you should behave accordingly.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use Carp;
use Net::XMPP::Connection;
use Net::Jabber::Protocol;
use base qw( Net::XMPP::Connection Net::Jabber::Protocol );
use vars qw( $VERSION );

$VERSION = "2.0";

use Net::Jabber::XDB;

sub new
{
    srand( time() ^ ($$ + ($$ << 15)));

    my $proto = shift;
    my $self = { };

    bless($self, $proto);
    $self->init(@_);
    
    $self->{SERVER}->{port} = 5269;
    $self->{SERVER}->{namespace} = "jabber:component:accept";
    $self->{SERVER}->{allow_register} = 0;
    
    return $self;
}


sub AuthSend
{
    my $self = shift;

    $self->_auth(@_);
}


sub _auth
{
    my $self = shift;
    my (%args) = @_;
    
    $self->{STREAM}->SetCallBacks(node=>undef);

    $self->Send("<handshake>".Digest::SHA1::sha1_hex($self->{SESSION}->{id}.$args{secret})."</handshake>");
    my $handshake = $self->Process();

    if (!defined($handshake) ||
        ($#{$handshake} == -1) ||
            (ref($handshake->[0]) ne "XML::Stream::Node") ||
                ($handshake->[0]->get_tag() ne "handshake"))
    {
        $self->SetErrorCode("Bad handshake.");
        return ("fail","Bad handshake.");
    }
    shift(@{$handshake});

    foreach my $node (@{$handshake})
    {
        $self->CallBack($self->{SESSION}->{id},$node);
    }

    $self->{STREAM}->SetCallBacks(node=>sub{ $self->CallBack(@_) });

    return ("ok","");
}


sub _connection_args
{
    my $self = shift;
    my (%args) = @_;
    
    my %connect;
    $connect{componentname}  = $args{componentname};
    $connect{hostname}       = $args{hostname};
    $connect{port}           = $args{port}           if exists($args{port});
    $connect{connectiontype} = $args{connectiontype} if exists($args{connectiontype});
    $connect{timeout}        = $args{connecttimeout} if exists($args{connecttimeout});
    $connect{tls}            = $args{tls}            if exists($args{tls});

    
    return %connect;
}


1;
