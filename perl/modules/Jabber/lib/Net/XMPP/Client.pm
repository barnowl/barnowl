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

package Net::XMPP::Client;

=head1 NAME

Net::XMPP::Client - XMPP Client Module

=head1 SYNOPSIS

  Net::XMPP::Client is a module that provides a developer easy access
  to the Extensible Messaging and Presence Protocol (XMPP).

=head1 DESCRIPTION

  Client.pm uses Protocol.pm to provide enough high level APIs and
  automation of the low level APIs that writing an XMPP Client in
  Perl is trivial.  For those that wish to work with the low level
  you can do that too, but those functions are covered in the
  documentation for each module.

  Net::XMPP::Client provides functions to connect to an XMPP server,
  login, send and receive messages, set personal information, create
  a new user account, manage the roster, and disconnect.  You can use
  all or none of the functions, there is no requirement.

  For more information on how the details for how Net::XMPP is written
  please see the help for Net::XMPP itself.

  For a full list of high level functions available please see
  Net::XMPP::Protocol.

=head2 Basic Functions

    use Net::XMPP;

    $Con = Net::XMPP::Client->new();

    $Con->SetCallbacks(...);

    $Con->Execute(hostname=>"jabber.org",
                  username=>"bob",
                  password=>"XXXX",
                  resource=>"Work'
                 );

    #
    # For the list of available functions see Net::XMPP::Protocol.
    #

    $Con->Disconnect();

=head1 METHODS

=head2 Basic Functions

    new(debuglevel=>0|1|2, - creates the Client object.  debugfile
        debugfile=>string,   should be set to the path for the debug
        debugtime=>0|1)      log to be written.  If set to "stdout"
                             then the debug will go there.  debuglevel
                             controls the amount of debug.  For more
                             information about the valid setting for
                             debuglevel, debugfile, and debugtime see
                             Net::XMPP::Debug.

    Connect(hostname=>string,      - opens a connection to the server
            port=>integer,           listed in the hostname (default
            timeout=>int             localhost), on the port (default
            connectiontype=>string,  5222) listed, using the
            tls=>0|1)                connectiontype listed (default
                                     tcpip).  The two connection types
                                     available are:
                                       tcpip  standard TCP socket
                                       http   TCP socket, but with the
                                              headers needed to talk
                                              through a web proxy
                                     If you specify tls, then it TLS
                                     will be used if it is available
                                     as a feature.

    Execute(hostname=>string,       - Generic inner loop to handle
            port=>int,                connecting to the server, calling
            tls=>0|1,                 Process, and reconnecting if the
            username=>string,         connection is lost.  There are
            password=>string,         five callbacks available that are
            resource=>string,         called at various places:
            register=>0|1,              onconnect - when the client has
            connectiontype=>string,                 made a connection.
            connecttimeout=>string,     onauth - when the connection is
            connectattempts=>int,                made and user has been
            connectsleep=>int,                   authed.  Essentially,
            processtimeout=>int)                 this is when you can
                                                 start doing things
                                                 as a Client.  Like
                                                 send presence, get your
                                                 roster, etc...
                                        onprocess - this is the most
                                                    inner loop and so
                                                    gets called the most.
                                                    Be very very careful
                                                    what you put here
                                                    since it can
                                                    *DRASTICALLY* affect
                                                    performance.
                                        ondisconnect - when the client
                                                       disconnects from
                                                       the server.
                                        onexit - when the function gives
                                                 up trying to connect and
                                                 exits.
                                      The arguments are passed straight
                                      on to the Connect function, except
                                      for connectattempts and connectsleep.
                                      connectattempts is the number of
                                      times that the Component should try
                                      to connect before giving up.  -1
                                      means try forever.  The default is
                                      -1. connectsleep is the number of
                                      seconds to sleep between each
                                      connection attempt.

                                      If you specify register=>1, then the
                                      Client will attempt to register the
                                      sepecified account for you, if it
                                      does not exist.

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

    Disconnect() - closes the connection to the server.

    Connected() - returns 1 if the Transport is connected to the server,
                  and 0 if not.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use Carp;
use Net::XMPP::Connection;
use base qw( Net::XMPP::Connection );

sub new
{
    my $proto = shift;
    my $self = { };

    bless($self, $proto);
    $self->init(@_);

    $self->{SERVER}->{port} = 5222;
    $self->{SERVER}->{namespace} = "jabber:client";
    $self->{SERVER}->{allow_register} = 1;

    return $self;
}


sub _auth
{
    my $self = shift;

    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my %auth;
    $auth{username} = $args{username};
    $auth{password} = $args{password};
    $auth{resource} = $args{resource} if exists($args{resource});

    return $self->AuthSend(%auth);
}


sub _connection_args
{
    my $self = shift;
    my (%args) = @_;

    my %connect;
    $connect{hostname}       = $args{hostname};
    $connect{port}           = $args{port}           if exists($args{port});
    $connect{connectiontype} = $args{connectiontype} if exists($args{connectiontype});
    $connect{timeout}        = $args{connecttimeout} if exists($args{connecttimeout});
    $connect{tls}            = $args{tls}            if exists($args{tls});

    return %connect;
}


sub _register
{
    my $self = shift;

    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my %fields = $self->RegisterRequest();

    $fields{username} = $args{username};
    $fields{password} = $args{password};

    $self->RegisterSend(%fields);

    return $self->_auth(%args);
}

1;
