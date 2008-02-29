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

package Net::XMPP::Connection;

=head1 NAME

Net::XMPP::Connection - XMPP Connection Module

=head1 SYNOPSIS

  Net::XMPP::Connection is a private package that serves as a basis
  for anything wanting to open a socket connection to a server.

=head1 DESCRIPTION

  This module is not meant to be used directly.  You should be using
  either Net::XMPP::Client, or another package that inherits from
  Net::XMPP::Connection.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use Carp;
use base qw( Net::XMPP::Protocol );


sub new
{
    my $proto = shift;
    my $self = { };

    bless($self, $proto);
    
    $self->init(@_);
    
    $self->{SERVER}->{namespace} = "unknown";

    return $self;
}


##############################################################################
#
# init - do all of the heavy lifting for a generic connection.
#
##############################################################################
sub init
{
    my $self = shift;
    
    $self->{ARGS} = {};
    while($#_ >= 0) { $self->{ARGS}->{ lc(pop(@_)) } = pop(@_); }

    $self->{DEBUG} =
        Net::XMPP::Debug->new(level      => $self->_arg("debuglevel",-1),
                              file       => $self->_arg("debugfile","stdout"),
                              time       => $self->_arg("debugtime",0),
                              setdefault => 1,
                              header     => "XMPP::Conn"
                             );

    $self->{SERVER} = {};
    $self->{SERVER}->{hostname} = "localhost";
    $self->{SERVER}->{tls} = $self->_arg("tls",0);
    $self->{SERVER}->{ssl} = $self->_arg("ssl",0);
    $self->{SERVER}->{connectiontype} = $self->_arg("connectiontype","tcpip");

    $self->{CONNECTED} = 0;
    $self->{DISCONNECTED} = 0;

    $self->{STREAM} =
        XML::Stream->new(style      => "node",
                         debugfh    => $self->{DEBUG}->GetHandle(),
                         debuglevel => $self->{DEBUG}->GetLevel(),
                         debugtime  => $self->{DEBUG}->GetTime(),
                        );
    
    $self->{RCVDB}->{currentID} = 0;

    $self->callbackInit();

    return $self;
}


##############################################################################
#
# Connect - Takes a has and opens the connection to the specified server.
#           Registers CallBack as the main callback for all packets from
#           the server.
#
#           NOTE:  Need to add some error handling if the connection is
#           not made because the server hostname is wrong or whatnot.
#
##############################################################################
sub Connect
{
    my $self = shift;

    while($#_ >= 0) { $self->{SERVER}{ lc pop(@_) } = pop(@_); }

    $self->{SERVER}->{timeout} = 10 unless exists($self->{SERVER}->{timeout});

    $self->{DEBUG}->Log1("Connect: host($self->{SERVER}->{hostname}:$self->{SERVER}->{port}) namespace($self->{SERVER}->{namespace})");
    $self->{DEBUG}->Log1("Connect: timeout($self->{SERVER}->{timeout})");
    
    delete($self->{SESSION});
    $self->{SESSION} =
        $self->{STREAM}->
            Connect(hostname       => $self->{SERVER}->{hostname},
                    port           => $self->{SERVER}->{port},
                    namespace      => $self->{SERVER}->{namespace},
                    connectiontype => $self->{SERVER}->{connectiontype},
                    timeout        => $self->{SERVER}->{timeout},
                    ssl            => $self->{SERVER}->{ssl}, #LEGACY
                    (defined($self->{SERVER}->{componentname}) ?
                     (to => $self->{SERVER}->{componentname}) :
                     ()
                    ),
                   );

    if ($self->{SESSION})
    {
        $self->{DEBUG}->Log1("Connect: connection made");

        $self->{STREAM}->SetCallBacks(node=>sub{ $self->CallBack(@_) });
        $self->{CONNECTED} = 1;

        if (exists($self->{SESSION}->{version}) &&
            ($self->{SESSION}->{version} ne ""))
        {
            my $tls = $self->GetStreamFeature("xmpp-tls");
            if (defined($tls) && $self->{SERVER}->{tls})
            {
                $self->{SESSION} =
                    $self->{STREAM}->StartTLS(
                        $self->{SESSION}->{id},
                        $self->{SERVER}->{timeout},
                    );
            }
            elsif (defined($tls) && ($tls eq "required"))
            {
                $self->SetErrorCode("The server requires us to use TLS, but you did not specify that\nTLS was an option.");
                return;
            }
        }

        return 1;
    }
    else
    {
        $self->SetErrorCode($self->{STREAM}->GetErrorCode());
        return;
    }
}


##############################################################################
#
# Connected - returns 1 if the Transport is connected to the server, 0
#             otherwise.
#
##############################################################################
sub Connected
{
    my $self = shift;

    $self->{DEBUG}->Log1("Connected: ($self->{CONNECTED})");
    return $self->{CONNECTED};
}


##############################################################################
#
# Disconnect - Sends the string to close the connection cleanly.
#
##############################################################################
sub Disconnect
{
    my $self = shift;

    $self->{STREAM}->Disconnect($self->{SESSION}->{id})
        if ($self->{CONNECTED} == 1);
    $self->{STREAM}->SetCallBacks(node=>undef);    
    $self->{CONNECTED} = 0;
    $self->{DISCONNECTED} = 1;
    $self->{DEBUG}->Log1("Disconnect: bye bye");
    $self->{DEBUG}->GetHandle()->close();
}


##############################################################################
#
# Execute - generic inner loop to listen for incoming messages, stay
#           connected to the server, and do all the right things.  It
#           calls a couple of callbacks for the user to put hooks into
#           place if they choose to.
#
##############################################################################
sub Execute
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $args{connectiontype} = "tcpip" unless exists($args{connectiontype});
    $args{connectattempts} = -1 unless exists($args{connectattempts});
    $args{connectsleep} = 5 unless exists($args{connectsleep});
    $args{register} = 0 unless exists($args{register});

    my %connect = $self->_connect_args(%args);

    $self->{DEBUG}->Log1("Execute: begin");

    my $connectAttempt = $args{connectattempts};

    while(($connectAttempt == -1) || ($connectAttempt > 0))
    {

        $self->{DEBUG}->Log1("Execute: Attempt to connect ($connectAttempt)");

        my $status = $self->Connect(%connect);

        if (!(defined($status)))
        {
            $self->{DEBUG}->Log1("Execute: Server is not answering.  (".$self->GetErrorCode().")");
            $self->{CONNECTED} = 0;

            $connectAttempt-- unless ($connectAttempt == -1);
            sleep($args{connectsleep});
            next;
        }

        $self->{DEBUG}->Log1("Execute: Connected...");
        &{$self->{CB}->{onconnect}}() if exists($self->{CB}->{onconnect});

        my @result = $self->_auth(%args);

        if (@result && $result[0] ne "ok")
        {
            $self->{DEBUG}->Log1("Execute: Could not auth with server: ($result[0]: $result[1])");
            &{$self->{CB}->{onauthfail}}()
                if exists($self->{CB}->{onauthfail});
            
            if (!$self->{SERVER}->{allow_register} || $args{register} == 0)
            {
                $self->{DEBUG}->Log1("Execute: Register turned off.  Exiting.");
                $self->Disconnect();
                &{$self->{CB}->{ondisconnect}}()
                    if exists($self->{CB}->{ondisconnect});
                $connectAttempt = 0;
            }
            else
            {
                @result = $self->_register(%args);

                if ($result[0] ne "ok")
                {
                    $self->{DEBUG}->Log1("Execute: Register failed.  Exiting.");
                    &{$self->{CB}->{onregisterfail}}()
                        if exists($self->{CB}->{onregisterfail});
            
                    $self->Disconnect();
                    &{$self->{CB}->{ondisconnect}}()
                        if exists($self->{CB}->{ondisconnect});
                    $connectAttempt = 0;
                }
                else
                {
                    &{$self->{CB}->{onauth}}()
                        if exists($self->{CB}->{onauth});
                }
            }
        }
        else
        {
            &{$self->{CB}->{onauth}}()
                if exists($self->{CB}->{onauth});
        }
 
        while($self->Connected())
        {

            while(defined($status = $self->Process($args{processtimeout})))
            {
                &{$self->{CB}->{onprocess}}()
                    if exists($self->{CB}->{onprocess});
            }

            if (!defined($status))
            {
                $self->Disconnect();
                $self->{DEBUG}->Log1("Execute: Connection to server lost...");
                &{$self->{CB}->{ondisconnect}}()
                    if exists($self->{CB}->{ondisconnect});

                $connectAttempt = $args{connectattempts};
                next;
            }
        }

        last if $self->{DISCONNECTED};
    }

    $self->{DEBUG}->Log1("Execute: end");
    &{$self->{CB}->{onexit}}() if exists($self->{CB}->{onexit});
}


###############################################################################
#
#  Process - If a timeout value is specified then the function will wait
#            that long before returning.  This is useful for apps that
#            need to handle other processing while still waiting for
#            packets.  If no timeout is listed then the function waits
#            until a packet is returned.  Either way the function exits
#            as soon as a packet is returned.
#
###############################################################################
sub Process
{
    my $self = shift;
    my ($timeout) = @_;
    my %status;

    if (exists($self->{PROCESSERROR}) && ($self->{PROCESSERROR} == 1))
    {
        croak("There was an error in the last call to Process that you did not check for and\nhandle.  You should always check the output of the Process call.  If it was\nundef then there was a fatal error that you need to check.  There is an error\nin your program");
    }

    $self->{DEBUG}->Log5("Process: timeout($timeout)") if defined($timeout);

    if (!defined($timeout) || ($timeout eq ""))
    {
        while(1)
        {
            %status = $self->{STREAM}->Process();
            $self->{DEBUG}->Log1("Process: status($status{$self->{SESSION}->{id}})");
            last if ($status{$self->{SESSION}->{id}} != 0);
            select(undef,undef,undef,.25);
        }
        $self->{DEBUG}->Log1("Process: return($status{$self->{SESSION}->{id}})");
        if ($status{$self->{SESSION}->{id}} == -1)
        {
            $self->{PROCESSERROR} = 1;
            return;
        }
        else
        {
            return $status{$self->{SESSION}->{id}};
        }
    }
    else
    {
        %status = $self->{STREAM}->Process($timeout);
        if ($status{$self->{SESSION}->{id}} == -1)
        {
            $self->{PROCESSERROR} = 1;
            return;
        }
        else
        {
            return $status{$self->{SESSION}->{id}};
        }
    }
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Overloadable Methods
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# _auth - Overload this method to provide the authentication method for your
#         type of connection.
#
##############################################################################
sub _auth
{
    my $self = shift;
    croak("You must override the _auth method.");
}


##############################################################################
#
# _connect_args - The Connect function that the Execute loop uses needs
#                 certain args.  This method lets you map the Execute args
#                 into the Connect args for your Connection type.
#
##############################################################################
sub _connect_args
{
    my $self = shift;
    my (%args) = @_;

    return %args;
}


##############################################################################
#
# _register - overload this method if you need your connection to register
#             with the server.
#
##############################################################################
sub _register
{
    my $self = shift;
    return ( "ok" ,"" );
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Private Helpers
#|
#+----------------------------------------------------------------------------
##############################################################################

sub _arg
{
    my $self = shift;
    my $arg = shift;
    my $default = shift;

    return exists($self->{ARGS}->{$arg}) ? $self->{ARGS}->{$arg} : $default;
}


1;
