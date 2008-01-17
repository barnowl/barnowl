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
#  Jabber
#  Copyright (C) 1998-2004 Jabber Software Foundation http://jabber.org/
#
##############################################################################

package XML::Stream;

=head1 NAME

XML::Stream - Creates and XML Stream connection and parses return data

=head1 SYNOPSIS

  XML::Stream is an attempt at solidifying the use of XML via streaming.

=head1 DESCRIPTION

  This module provides the user with methods to connect to a remote
  server, send a stream of XML to the server, and receive/parse an XML
  stream from the server.  It is primarily based work for the Etherx XML
  router developed by the Jabber Development Team.  For more information
  about this project visit http://etherx.jabber.org/stream/.

  XML::Stream gives the user the ability to define a central callback
  that will be used to handle the tags received from the server.  These
  tags are passed in the format defined at instantiation time.
  the closing tag of an object is seen, the tree is finished and passed
  to the call back function.  What the user does with it from there is up
  to them.

  For a detailed description of how this module works, and about the data
  structure that it returns, please view the source of Stream.pm and
  look at the detailed description at the end of the file.


  NOTE: The parser that XML::Stream::Parser provides, as are most Perl
  parsers, is synchronous.  If you are in the middle of parsing a
  packet and call a user defined callback, the Parser is blocked until
  your callback finishes.  This means you cannot be operating on a
  packet, send out another packet and wait for a response to that packet.
  It will never get to you.  Threading might solve this, but as we all
  know threading in Perl is not quite up to par yet.  This issue will be
  revisted in the future.



=head1 METHODS

  new(debug=>string,       - creates the XML::Stream object.  debug
      debugfh=>FileHandle,   should be set to the path for the debug log
      debuglevel=>0|1|N,     to be written.  If set to "stdout" then the
      debugtime=>0|1,        debug will go there.   Also, you can specify
      style=>string)         a filehandle that already exists byt using
                             debugfh.  debuglevel determines the amount
                             of debug to generate.  0 is the least, 1 is
                             a little more, N is the limit you want.
                             debugtime determines wether a timestamp
                             should be preappended to the entry.  style
                             defines the way the data structure is
                             returned.  The two available styles are:

                               tree - XML::Parser Tree format
                               node - XML::Stream::Node format

                             For more information see the respective man
                             pages.

  Connect(hostname=>string,       - opens a tcp connection to the
          port=>integer,            specified server and sends the proper
          to=>string,               opening XML Stream tag.  hostname,
          from=>string,             port, and namespace are required.
          myhostname=>string,       namespaces allows you to use
          namespace=>string,        XML::Stream::Namespace objects.
          namespaces=>array,        to is needed if you want the stream
          connectiontype=>string,   to attribute to be something other
          ssl=>0|1,                 than the hostname you are connecting
          srv=>string)              to.  from is needed if you want the
                                    stream from attribute to be something
                                    other than the hostname you are
                                    connecting from.  myhostname should
                                    not be needed but if the module
                                    cannot determine your hostname
                                    properly (check the debug log), set
                                    this to the correct value, or if you
                                    want the other side of the  stream to
                                    think that you are someone else.  The
                                    type determines the kind of
                                    connection that is made:
                                      "tcpip"    - TCP/IP (default)
                                      "stdinout" - STDIN/STDOUT
                                      "http"     - HTTP
                                    HTTP recognizes proxies if the ENV
                                    variables http_proxy or https_proxy
                                    are set.  ssl specifies if an SLL
                                    socket should be used for encrypted
                                    communications.  This function
                                    returns the same hash from GetRoot()
                                    below. Make sure you get the SID
                                    (Session ID) since you have to use it
                                    to call most other functions in here.

                                    If srv is specified AND Net::DNS is
                                    installed and can be loaded, then
                                    an SRV query is sent to srv.hostname
                                    and the results processed to replace
                                    the hostname and port.  If the lookup
                                    fails, or Net::DNS cannot be loaded,
                                    then hostname and port are left alone
                                    as the defaults.


  OpenFile(string) - opens a filehandle to the argument specified, and
                     pretends that it is a stream.  It will ignore the
                     outer tag, and not check if it was a
                     <stream:stream/>. This is useful for writing a
                     program that has to parse any XML file that is
                     basically made up of small packets (like RDF).

  Disconnect(sid) - sends the proper closing XML tag and closes the
                    specified socket down.

  Process(integer) - waits for data to be available on the socket.  If
                     a timeout is specified then the Process function
                     waits that period of time before returning nothing.
                     If a timeout period is not specified then the
                     function blocks until data is received.  The
                     function returns a hash with session ids as the key,
                     and status values or data as the hash values.

  SetCallBacks(node=>function,   - sets the callback that should be
               update=>function)   called in various situations.  node
                                   is used to handle the data structures
                                   that are built for each top level tag.
                                   Update is used for when Process is
                                   blocking waiting for data, but you
                                   want your original code to be updated.

  GetRoot(sid) - returns the attributes that the stream:stream tag sent
                 by the other end listed in a hash for the specified
                 session.

  GetSock(sid) - returns a pointer to the IO::Socket object for the
                 specified session.

  Send(sid,    - sends the string over the specified connection as is.
       string)   This does no checking if valid XML was sent or not.
                 Best behavior when sending information.

  GetErrorCode(sid) - returns a string for the specified session that
                      will hopefully contain some useful information
                      about why Process or Connect returned an undef
                      to you.

  XPath(node,path) - returns an array of results that match the xpath.
                     node can be any of the three types (Tree, Node).

=head1 VARIABLES

  $NONBLOCKING - tells the Parser to enter into a nonblocking state.  This
                 might cause some funky behavior since you can get nested
                 callbacks while things are waiting.  1=on, 0=off(default).

=head1 EXAMPLES

  ##########################
  # simple example

  use XML::Stream qw( Tree );

  $stream = new XML::Stream;

  my $status = $stream->Connect(hostname => "jabber.org",
                                port => 5222,
                                namespace => "jabber:client");

  if (!defined($status)) {
    print "ERROR: Could not connect to server\n";
    print "       (",$stream->GetErrorCode(),")\n";
    exit(0);
  }

  while($node = $stream->Process()) {
    # do something with $node
  }

  $stream->Disconnect();


  ###########################
  # example using a handler

  use XML::Stream qw( Tree );

  $stream = new XML::Stream;
  $stream->SetCallBacks(node=>\&noder);
  $stream->Connect(hostname => "jabber.org",
		   port => 5222,
		   namespace => "jabber:client",
		   timeout => undef) || die $!;

  # Blocks here forever, noder is called for incoming
  # packets when they arrive.
  while(defined($stream->Process())) { }

  print "ERROR: Stream died (",$stream->GetErrorCode(),")\n";

  sub noder
  {
    my $sid = shift;
    my $node = shift;
    # do something with $node
  }

=head1 AUTHOR

Tweaked, tuned, and brightness changes by Ryan Eatmon, reatmon@ti.com
in May of 2000.
Colorized, and Dolby Surround sound added by Thomas Charron,
tcharron@jabber.org
By Jeremie in October of 1999 for http://etherx.jabber.org/streams/

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use 5.006_001;
use strict;
use Sys::Hostname;
use IO::Socket;
use IO::Select;
use FileHandle;
use Carp;
use POSIX;
use Authen::SASL;
use MIME::Base64;
use utf8;
use Encode;

$SIG{PIPE} = "IGNORE";

use vars qw($VERSION $PAC $SSL $NONBLOCKING %HANDLERS $NETDNS %XMLNS );

##############################################################################
# Define the namespaces in an easy/constant manner.
#-----------------------------------------------------------------------------
# 0.9
#-----------------------------------------------------------------------------
$XMLNS{'stream'}        = "http://etherx.jabber.org/streams";

#-----------------------------------------------------------------------------
# 1.0
#-----------------------------------------------------------------------------
$XMLNS{'xmppstreams'}   = "urn:ietf:params:xml:ns:xmpp-streams";
$XMLNS{'xmpp-bind'}     = "urn:ietf:params:xml:ns:xmpp-bind";
$XMLNS{'xmpp-sasl'}     = "urn:ietf:params:xml:ns:xmpp-sasl";
$XMLNS{'xmpp-session'}  = "urn:ietf:params:xml:ns:xmpp-session";
$XMLNS{'xmpp-tls'}      = "urn:ietf:params:xml:ns:xmpp-tls";
##############################################################################


if (eval "require Net::DNS;" )
{
    require Net::DNS;
    import Net::DNS;
    $NETDNS = 1;
}
else
{
    $NETDNS = 0;
}


$VERSION = "1.22";
$NONBLOCKING = 0;

use XML::Stream::Namespace;
use XML::Stream::Parser;
use XML::Stream::XPath;

##############################################################################
#
# Setup the exportable objects
#
##############################################################################
require Exporter;
my @ISA = qw(Exporter);
my @EXPORT_OK = qw(Tree Node);

sub import
{
    my $class = shift;

    foreach my $module (@_)
    {
        eval "use XML::Stream::$module;";
        die($@) if ($@);

        my $lc = lc($module);
        
        eval("\$HANDLERS{\$lc}->{startElement} = \\&XML::Stream::${module}::_handle_element;");
        eval("\$HANDLERS{\$lc}->{endElement}   = \\&XML::Stream::${module}::_handle_close;");
        eval("\$HANDLERS{\$lc}->{characters}   = \\&XML::Stream::${module}::_handle_cdata;");
    }
}


sub new
{
    my $proto = shift;
    my $self = { };

    bless($self,$proto);

    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $self->{DATASTYLE} = "tree";
    $self->{DATASTYLE} = delete($args{style}) if exists($args{style});

    if ((($self->{DATASTYLE} eq "tree") && !defined($XML::Stream::Tree::LOADED)) ||
        (($self->{DATASTYLE} eq "node") && !defined($XML::Stream::Node::LOADED))
       )
    {
        croak("The style that you have chosen was not defined when you \"use\"d the module.\n");
    }

    $self->{DEBUGARGS} = \%args;

    $self->{DEBUGTIME} = 0;
    $self->{DEBUGTIME} = $args{debugtime} if exists($args{debugtime});

    $self->{DEBUGLEVEL} = 0;
    $self->{DEBUGLEVEL} = $args{debuglevel} if exists($args{debuglevel});

    $self->{DEBUGFILE} = "";

    if (exists($args{debugfh}) && ($args{debugfh} ne ""))
    {
        $self->{DEBUGFILE} = $args{debugfh};
        $self->{DEBUG} = 1;
    }
    if ((exists($args{debugfh}) && ($args{debugfh} eq "")) ||
        (exists($args{debug}) && ($args{debug} ne "")))
    {
        $self->{DEBUG} = 1;
        if (lc($args{debug}) eq "stdout")
        {
            $self->{DEBUGFILE} = new FileHandle(">&STDERR");
            $self->{DEBUGFILE}->autoflush(1);
        }
        else
        {
            if (-e $args{debug})
            {
                if (-w $args{debug})
                {
                    $self->{DEBUGFILE} = new FileHandle(">$args{debug}");
                    $self->{DEBUGFILE}->autoflush(1);
                }
                else
                {
                    print "WARNING: debug file ($args{debug}) is not writable by you\n";
                    print "         No debug information being saved.\n";
                    $self->{DEBUG} = 0;
                }
            }
            else
            {
                $self->{DEBUGFILE} = new FileHandle(">$args{debug}");
                if (defined($self->{DEBUGFILE}))
                {
                    $self->{DEBUGFILE}->autoflush(1);
                }
                else
                {
                    print "WARNING: debug file ($args{debug}) does not exist \n";
                    print "         and is not writable by you.\n";
                    print "         No debug information being saved.\n";
                    $self->{DEBUG} = 0;
                }
            }
        }
    }

    my $hostname = hostname();
    my $address = gethostbyname($hostname) ||
        die("Cannot resolve $hostname: $!");
    my $fullname = gethostbyaddr($address,AF_INET) || $hostname;

    $self->debug(1,"new: hostname = ($fullname)");

    #---------------------------------------------------------------------------
    # Setup the defaults that the module will work with.
    #---------------------------------------------------------------------------
    $self->{SIDS}->{default}->{hostname} = "";
    $self->{SIDS}->{default}->{port} = "";
    $self->{SIDS}->{default}->{sock} = 0;
    $self->{SIDS}->{default}->{ssl} = (exists($args{ssl}) ? $args{ssl} : 0);
    $self->{SIDS}->{default}->{namespace} = "";
    $self->{SIDS}->{default}->{myhostname} = $fullname;
    $self->{SIDS}->{default}->{derivedhostname} = $fullname;
    $self->{SIDS}->{default}->{id} = "";

    #---------------------------------------------------------------------------
    # We are only going to use one callback, let the user call other callbacks
    # on his own.
    #---------------------------------------------------------------------------
    $self->SetCallBacks(node=>sub { $self->_node(@_) });

    $self->{IDCOUNT} = 0;

    return $self;
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Incoming Connection Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# Listen - starts the stream by listening on a port for someone to connect,
#          and send the opening stream tag, and then sending a response based
#          on if the received header was correct for this stream.  Server
#          name, port, and namespace are required otherwise we don't know
#          where to listen and what namespace to accept.
#
##############################################################################
sub Listen
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $serverid = "server$args{port}";

    return if exists($self->{SIDS}->{$serverid});

    push(@{$self->{SIDS}->{server}},$serverid);

    foreach my $key (keys(%{$self->{SIDS}->{default}}))
    {
        $self->{SIDS}->{$serverid}->{$key} = $self->{SIDS}->{default}->{$key};
    }

    foreach my $key (keys(%args))
    {
        $self->{SIDS}->{$serverid}->{$key} = $args{$key};
    }

    $self->debug(1,"Listen: start");

    if ($self->{SIDS}->{$serverid}->{namespace} eq "")
    {
        $self->SetErrorCode($serverid,"Namespace not specified");
        return;
    }

    #---------------------------------------------------------------------------
    # Check some things that we have to know in order get the connection up
    # and running.  Server hostname, port number, namespace, etc...
    #---------------------------------------------------------------------------
    if ($self->{SIDS}->{$serverid}->{hostname} eq "")
    {
        $self->SetErrorCode("$serverid","Server hostname not specified");
        return;
    }
    if ($self->{SIDS}->{$serverid}->{port} eq "")
    {
        $self->SetErrorCode("$serverid","Server port not specified");
        return;
    }
    if ($self->{SIDS}->{$serverid}->{myhostname} eq "")
    {
        $self->{SIDS}->{$serverid}->{myhostname} = $self->{SIDS}->{$serverid}->{derivedhostname};
    }

    #-------------------------------------------------------------------------
    # Open the connection to the listed server and port.  If that fails then
    # abort ourselves and let the user check $! on his own.
    #-------------------------------------------------------------------------

    while($self->{SIDS}->{$serverid}->{sock} == 0)
    {
        $self->{SIDS}->{$serverid}->{sock} =
            new IO::Socket::INET(LocalHost=>$self->{SIDS}->{$serverid}->{hostname},
                                 LocalPort=>$self->{SIDS}->{$serverid}->{port},
                                 Reuse=>1,
                                 Listen=>10,
                                 Proto=>'tcp');
        select(undef,undef,undef,.1);
    }
    $self->{SIDS}->{$serverid}->{status} = 1;
    $self->nonblock($self->{SIDS}->{$serverid}->{sock});
    $self->{SIDS}->{$serverid}->{sock}->autoflush(1);

    $self->{SELECT} =
        new IO::Select($self->{SIDS}->{$serverid}->{sock});
    $self->{SIDS}->{$serverid}->{select} =
        new IO::Select($self->{SIDS}->{$serverid}->{sock});

    $self->{SOCKETS}->{$self->{SIDS}->{$serverid}->{sock}} = "$serverid";

    return $serverid;
}


##############################################################################
#
# ConnectionAccept - accept an incoming connection.
#
##############################################################################
sub ConnectionAccept
{
    my $self = shift;
    my $serverid = shift;

    my $sid = $self->NewSID();

    $self->debug(1,"ConnectionAccept: sid($sid)");

    $self->{SIDS}->{$sid}->{sock} = $self->{SIDS}->{$serverid}->{sock}->accept();

    $self->nonblock($self->{SIDS}->{$sid}->{sock});
    $self->{SIDS}->{$sid}->{sock}->autoflush(1);

    $self->debug(3,"ConnectionAccept: sid($sid) client($self->{SIDS}->{$sid}->{sock}) server($self->{SIDS}->{$serverid}->{sock})");

    $self->{SELECT}->add($self->{SIDS}->{$sid}->{sock});

    #-------------------------------------------------------------------------
    # Create the XML::Stream::Parser and register our callbacks
    #-------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{parser} =
        new XML::Stream::Parser(%{$self->{DEBUGARGS}},
                                nonblocking=>$NONBLOCKING,
                                sid=>$sid,
                                style=>$self->{DATASTYLE},
                                Handlers=>{
                                    startElement=>sub{ $self->_handle_root(@_) },
                                    endElement=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{endElement}}($self,@_) },
                                    characters=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{characters}}($self,@_) },
                                }
                               );

    $self->{SIDS}->{$sid}->{select} =
        new IO::Select($self->{SIDS}->{$sid}->{sock});
    $self->{SIDS}->{$sid}->{connectiontype} = "tcpip";
    $self->{SOCKETS}->{$self->{SIDS}->{$sid}->{sock}} = $sid;

    $self->InitConnection($sid,$serverid);

    #---------------------------------------------------------------------------
    # Grab the init time so that we can check if we get data in the timeout
    # period or not.
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{activitytimeout} = time;

    return $sid;
}


##############################################################################
#
# Respond - If this is a listening socket then we need to respond to the
#           opening <stream:stream/>.
#
##############################################################################
sub Respond
{
    my $self = shift;
    my $sid = shift;
    my $serverid = $self->{SIDS}->{$sid}->{serverid};

    my $root = $self->GetRoot($sid);
    
    if ($root->{xmlns} ne $self->{SIDS}->{$serverid}->{namespace})
    {
        my $error = $self->StreamError($sid,"invalid-namespace","Invalid namespace specified");
        $self->Send($sid,$error);

        $self->{SIDS}->{$sid}->{sock}->flush();
        select(undef,undef,undef,1);
        $self->Disconnect($sid);
    }

    #---------------------------------------------------------------------------
    # Next, we build the opening handshake.
    #---------------------------------------------------------------------------
    my %stream_args;

    $stream_args{from} =
        (exists($self->{SIDS}->{$serverid}->{from}) ?
         $self->{SIDS}->{$serverid}->{from} :
         $self->{SIDS}->{$serverid}->{hostname}
        );

    $stream_args{to} = $self->GetRoot($sid)->{from};
    $stream_args{id} = $sid;
    $stream_args{namespaces} = $self->{SIDS}->{$serverid}->{namespaces};

    my $stream =
        $self->StreamHeader(
            xmlns=>$self->{SIDS}->{$serverid}->{namespace},
            xmllang=>"en",
            %stream_args
        );

    #---------------------------------------------------------------------------
    # Then we send the opening handshake.
    #---------------------------------------------------------------------------
    $self->Send($sid,$stream);
    delete($self->{SIDS}->{$sid}->{activitytimeout});
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Outgoing Connection Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# Connect - starts the stream by connecting to the server, sending the opening
#           stream tag, and then waiting for a response and verifying that it
#           is correct for this stream.  Server name, port, and namespace are
#           required otherwise we don't know where to send the stream to...
#
##############################################################################
sub Connect
{
    my $self = shift;

    foreach my $key (keys(%{$self->{SIDS}->{default}}))
    {
        $self->{SIDS}->{newconnection}->{$key} = $self->{SIDS}->{default}->{$key};
    }
    while($#_ >= 0) { $self->{SIDS}->{newconnection}->{ lc pop(@_) } = pop(@_); }
    
    my $timeout = exists($self->{SIDS}->{newconnection}->{timeout}) ?
                  delete($self->{SIDS}->{newconnection}->{timeout}) :
                  "";

    $self->debug(4,"Connect: timeout($timeout)");
    

    if (exists($self->{SIDS}->{newconnection}->{srv}))
    {
        $self->debug(1,"Connect: srv requested");
        if ($NETDNS)
        {
            my $res = Net::DNS::Resolver->new();
            my $query = $res->query($self->{SIDS}->{newconnection}->{srv}.".".$self->{SIDS}->{newconnection}->{hostname},"SRV");
            
            if ($query)
            { 
                $self->{SIDS}->{newconnection}->{hostname} = ($query->answer)[0]->target();
                $self->{SIDS}->{newconnection}->{port} = ($query->answer)[0]->port();
                $self->debug(1,"Connect: srv host: $self->{SIDS}->{newconnection}->{hostname}");
                $self->debug(1,"Connect: srv post: $self->{SIDS}->{newconnection}->{port}");
            }
            else
            {
                $self->debug(1,"Connect: srv query failed");
            }
        }
        else
        {
            $self->debug(1,"Connect: srv query failed");
        }
        delete($self->{SIDS}->{newconnection}->{srv});
    }

    $self->{SIDS}->{newconnection}->{connectiontype} = "tcpip"
        unless exists($self->{SIDS}->{newconnection}->{connectiontype});

    $self->debug(1,"Connect: type($self->{SIDS}->{newconnection}->{connectiontype})");

    if ($self->{SIDS}->{newconnection}->{namespace} eq "")
    {
        $self->SetErrorCode("newconnection","Namespace not specified");
        return;
    }

    #---------------------------------------------------------------------------
    # TCP/IP
    #---------------------------------------------------------------------------
    if ($self->{SIDS}->{newconnection}->{connectiontype} eq "tcpip")
    {
        #-----------------------------------------------------------------------
        # Check some things that we have to know in order get the connection up
        # and running.  Server hostname, port number, namespace, etc...
        #-----------------------------------------------------------------------
        if ($self->{SIDS}->{newconnection}->{hostname} eq "")
        {
            $self->SetErrorCode("newconnection","Server hostname not specified");
            return;
        }
        if ($self->{SIDS}->{newconnection}->{port} eq "")
        {
            $self->SetErrorCode("newconnection","Server port not specified");
            return;
        }
        if ($self->{SIDS}->{newconnection}->{myhostname} eq "")
        {
            $self->{SIDS}->{newconnection}->{myhostname} = $self->{SIDS}->{newconnection}->{derivedhostname};
        }

        #-----------------------------------------------------------------------
        # Open the connection to the listed server and port.  If that fails then
        # abort ourselves and let the user check $! on his own.
        #-----------------------------------------------------------------------
        $self->{SIDS}->{newconnection}->{sock} =
            new IO::Socket::INET(PeerAddr=>$self->{SIDS}->{newconnection}->{hostname},
                                 PeerPort=>$self->{SIDS}->{newconnection}->{port},
                                 Proto=>"tcp",
                                 (($timeout ne "") ? ( Timeout=>$timeout ) : ()),
                                );
        return unless $self->{SIDS}->{newconnection}->{sock};

        if ($self->{SIDS}->{newconnection}->{ssl} == 1)
        {
            $self->debug(1,"Connect: Convert normal socket to SSL");
            $self->debug(1,"Connect: sock($self->{SIDS}->{newconnection}->{sock})");
            $self->LoadSSL();
            $self->{SIDS}->{newconnection}->{sock} =
                IO::Socket::SSL::socketToSSL($self->{SIDS}->{newconnection}->{sock},
                                             {SSL_verify_mode=>0x00});
            $self->debug(1,"Connect: ssl_sock($self->{SIDS}->{newconnection}->{sock})");
            $self->debug(1,"Connect: SSL: We are secure") if ($self->{SIDS}->{newconnection}->{sock});
        }
        return unless $self->{SIDS}->{newconnection}->{sock};
    }

    #---------------------------------------------------------------------------
    # STDIN/OUT
    #---------------------------------------------------------------------------
    if ($self->{SIDS}->{newconnection}->{connectiontype} eq "stdinout")
    {
        $self->{SIDS}->{newconnection}->{sock} =
            new FileHandle(">&STDOUT");
    }  

    #---------------------------------------------------------------------------
    # HTTP
    #---------------------------------------------------------------------------
    if ($self->{SIDS}->{newconnection}->{connectiontype} eq "http")
    {
        #-----------------------------------------------------------------------
        # Check some things that we have to know in order get the connection up
        # and running.  Server hostname, port number, namespace, etc...
        #-----------------------------------------------------------------------
        if ($self->{SIDS}->{newconnection}->{hostname} eq "")
        {
            $self->SetErrorCode("newconnection","Server hostname not specified");
            return;
        }
        if ($self->{SIDS}->{newconnection}->{port} eq "")
        {
            $self->SetErrorCode("newconnection","Server port not specified");
            return;
        }
        if ($self->{SIDS}->{newconnection}->{myhostname} eq "")
        {
            $self->{SIDS}->{newconnection}->{myhostname} = $self->{SIDS}->{newconnection}->{derivedhostname};
        }

        if (!defined($PAC))
        {
            eval("use HTTP::ProxyAutoConfig;");
            if ($@)
            {
                $PAC = 0;
            }
            else
            {
                require HTTP::ProxyAutoConfig;
                $PAC = new HTTP::ProxyAutoConfig();
            }
        }

        if ($PAC eq "0") {
            if (exists($ENV{"http_proxy"}))
            {
                my($host,$port) = ($ENV{"http_proxy"} =~ /^(\S+)\:(\d+)$/);
                $self->{SIDS}->{newconnection}->{httpproxyhostname} = $host;
                $self->{SIDS}->{newconnection}->{httpproxyport} = $port;
                $self->{SIDS}->{newconnection}->{httpproxyhostname} =~ s/^http\:\/\///;
            }
            if (exists($ENV{"https_proxy"}))
            {
                my($host,$port) = ($ENV{"https_proxy"} =~ /^(\S+)\:(\d+)$/);
                $self->{SIDS}->{newconnection}->{httpsproxyhostname} = $host;
                $self->{SIDS}->{newconnection}->{httpsproxyport} = $port;
                $self->{SIDS}->{newconnection}->{httpsproxyhostname} =~ s/^https?\:\/\///;
            }
        }
        else
        {
            my $proxy = $PAC->FindProxy("http://".$self->{SIDS}->{newconnection}->{hostname});
            if ($proxy ne "DIRECT")
            {
                ($self->{SIDS}->{newconnection}->{httpproxyhostname},$self->{SIDS}->{newconnection}->{httpproxyport}) = ($proxy =~ /^PROXY ([^:]+):(\d+)$/);
            }

            $proxy = $PAC->FindProxy("https://".$self->{SIDS}->{newconnection}->{hostname});

            if ($proxy ne "DIRECT")
            {
                ($self->{SIDS}->{newconnection}->{httpsproxyhostname},$self->{SIDS}->{newconnection}->{httpsproxyport}) = ($proxy =~ /^PROXY ([^:]+):(\d+)$/);
            }
        }

        $self->debug(1,"Connect: http_proxy($self->{SIDS}->{newconnection}->{httpproxyhostname}:$self->{SIDS}->{newconnection}->{httpproxyport})")
            if (exists($self->{SIDS}->{newconnection}->{httpproxyhostname}) &&
                defined($self->{SIDS}->{newconnection}->{httpproxyhostname}) &&
                exists($self->{SIDS}->{newconnection}->{httpproxyport}) &&
                defined($self->{SIDS}->{newconnection}->{httpproxyport}));
        $self->debug(1,"Connect: https_proxy($self->{SIDS}->{newconnection}->{httpsproxyhostname}:$self->{SIDS}->{newconnection}->{httpsproxyport})")
            if (exists($self->{SIDS}->{newconnection}->{httpsproxyhostname}) &&
                defined($self->{SIDS}->{newconnection}->{httpsproxyhostname}) &&
                exists($self->{SIDS}->{newconnection}->{httpsproxyport}) &&
                defined($self->{SIDS}->{newconnection}->{httpsproxyport}));

        #-----------------------------------------------------------------------
        # Open the connection to the listed server and port.  If that fails then
        # abort ourselves and let the user check $! on his own.
        #-----------------------------------------------------------------------
        my $connect = "CONNECT $self->{SIDS}->{newconnection}->{hostname}:$self->{SIDS}->{newconnection}->{port} HTTP/1.1\r\nHost: $self->{SIDS}->{newconnection}->{hostname}\r\n\r\n";
        my $put = "PUT http://$self->{SIDS}->{newconnection}->{hostname}:$self->{SIDS}->{newconnection}->{port} HTTP/1.1\r\nHost: $self->{SIDS}->{newconnection}->{hostname}\r\nProxy-Connection: Keep-Alive\r\n\r\n";

        my $connected = 0;
        #-----------------------------------------------------------------------
        # Combo #0 - The user didn't specify a proxy
        #-----------------------------------------------------------------------
        if (!exists($self->{SIDS}->{newconnection}->{httpproxyhostname}) &&
            !exists($self->{SIDS}->{newconnection}->{httpsproxyhostname}))
        {

            $self->debug(1,"Connect: Combo #0: User did not specify a proxy... connecting DIRECT");

            $self->debug(1,"Connect: Combo #0: Create normal socket");
            $self->{SIDS}->{newconnection}->{sock} =
            new IO::Socket::INET(PeerAddr=>$self->{SIDS}->{newconnection}->{hostname},
                                 PeerPort=>$self->{SIDS}->{newconnection}->{port},
                                 Proto=>"tcp",
                                 (($timeout ne "") ? ( Timeout=>$timeout ) : ()),
                                );
            $connected = defined($self->{SIDS}->{newconnection}->{sock});
            $self->debug(1,"Connect: Combo #0: connected($connected)");
            #            if ($connected)
            #            {
            #                $self->{SIDS}->{newconnection}->{sock}->syswrite($put,length($put),0);
            #                my $buff;
            #                $self->{SIDS}->{newconnection}->{sock}->sysread($buff,4*POSIX::BUFSIZ);
            #                my ($code) = ($buff =~ /^\S+\s+(\S+)\s+/);
            #                $self->debug(1,"Connect: Combo #1: buff($buff)");
            #                $connected = 0 if ($code !~ /2\d\d/);
            #            }
            #            $self->debug(1,"Connect: Combo #0: connected($connected)");
          }

        #-----------------------------------------------------------------------
        # Combo #1 - PUT through http_proxy
        #-----------------------------------------------------------------------
        if (!$connected &&
            exists($self->{SIDS}->{newconnection}->{httpproxyhostname}) &&
            ($self->{SIDS}->{newconnection}->{ssl} == 0))
        {

            $self->debug(1,"Connect: Combo #1: PUT through http_proxy");
            $self->{SIDS}->{newconnection}->{sock} =
                new IO::Socket::INET(PeerAddr=>$self->{SIDS}->{newconnection}->{httpproxyhostname},
                                     PeerPort=>$self->{SIDS}->{newconnection}->{httpproxyport},
                                     Proto=>"tcp",
                                     (($timeout ne "") ? ( Timeout=>$timeout ) : ()),
                                    );
            $connected = defined($self->{SIDS}->{newconnection}->{sock});
            $self->debug(1,"Connect: Combo #1: connected($connected)");
            if ($connected)
            {
                $self->debug(1,"Connect: Combo #1: send($put)");
                $self->{SIDS}->{newconnection}->{sock}->syswrite($put,length($put),0);
                my $buff;
                $self->{SIDS}->{newconnection}->{sock}->sysread($buff,4*POSIX::BUFSIZ);
                my ($code) = ($buff =~ /^\S+\s+(\S+)\s+/);
                $self->debug(1,"Connect: Combo #1: buff($buff)");
                $connected = 0 if ($code !~ /2\d\d/);
            }
            $self->debug(1,"Connect: Combo #1: connected($connected)");
        }
        #-----------------------------------------------------------------------
        # Combo #2 - CONNECT through http_proxy
        #-----------------------------------------------------------------------
        if (!$connected &&
            exists($self->{SIDS}->{newconnection}->{httpproxyhostname}) &&
            ($self->{SIDS}->{newconnection}->{ssl} == 0))
        {

            $self->debug(1,"Connect: Combo #2: CONNECT through http_proxy");
            $self->{SIDS}->{newconnection}->{sock} =
                new IO::Socket::INET(PeerAddr=>$self->{SIDS}->{newconnection}->{httpproxyhostname},
                                     PeerPort=>$self->{SIDS}->{newconnection}->{httpproxyport},
                                     Proto=>"tcp",
                                     (($timeout ne "") ? ( Timeout=>$timeout ) : ()),
                                    );
            $connected = defined($self->{SIDS}->{newconnection}->{sock});
            $self->debug(1,"Connect: Combo #2: connected($connected)");
            if ($connected)
            {
                $self->{SIDS}->{newconnection}->{sock}->syswrite($connect,length($connect),0);
                my $buff;
                $self->{SIDS}->{newconnection}->{sock}->sysread($buff,4*POSIX::BUFSIZ);
                my ($code) = ($buff =~ /^\S+\s+(\S+)\s+/);
                $self->debug(1,"Connect: Combo #2: buff($buff)");
                $connected = 0 if ($code !~ /2\d\d/);
            }
            $self->debug(1,"Connect: Combo #2: connected($connected)");
        }

        #-----------------------------------------------------------------------
        # Combo #3 - CONNECT through https_proxy
        #-----------------------------------------------------------------------
        if (!$connected &&
            exists($self->{SIDS}->{newconnection}->{httpsproxyhostname}))
        {
            $self->debug(1,"Connect: Combo #3: CONNECT through https_proxy");
            $self->{SIDS}->{newconnection}->{sock} =
                new IO::Socket::INET(PeerAddr=>$self->{SIDS}->{newconnection}->{httpsproxyhostname},
                                     PeerPort=>$self->{SIDS}->{newconnection}->{httpsproxyport},
                                     Proto=>"tcp");
            $connected = defined($self->{SIDS}->{newconnection}->{sock});
            $self->debug(1,"Connect: Combo #3: connected($connected)");
            if ($connected)
            {
                $self->{SIDS}->{newconnection}->{sock}->syswrite($connect,length($connect),0);
                my $buff;
                $self->{SIDS}->{newconnection}->{sock}->sysread($buff,4*POSIX::BUFSIZ);
                my ($code) = ($buff =~ /^\S+\s+(\S+)\s+/);
                $self->debug(1,"Connect: Combo #3: buff($buff)");
                $connected = 0 if ($code !~ /2\d\d/);
            }
            $self->debug(1,"Connect: Combo #3: connected($connected)");
        }

        #-----------------------------------------------------------------------
        # We have failed
        #-----------------------------------------------------------------------
        if (!$connected)
        {
            $self->debug(1,"Connect: No connection... I have failed... I.. must... end it all...");
            $self->SetErrorCode("newconnection","Unable to open a connection to destination.  Please check your http_proxy and/or https_proxy environment variables.");
            return;
        }

        return unless $self->{SIDS}->{newconnection}->{sock};

        $self->debug(1,"Connect: We are connected");

        if (($self->{SIDS}->{newconnection}->{ssl} == 1) &&
            (ref($self->{SIDS}->{newconnection}->{sock}) eq "IO::Socket::INET"))
        {
            $self->debug(1,"Connect: Convert normal socket to SSL");
            $self->debug(1,"Connect: sock($self->{SIDS}->{newconnection}->{sock})");
            $self->LoadSSL();
            $self->{SIDS}->{newconnection}->{sock} =
                IO::Socket::SSL::socketToSSL($self->{SIDS}->{newconnection}->{sock},
                                             {SSL_verify_mode=>0x00});
            $self->debug(1,"Connect: ssl_sock($self->{SIDS}->{newconnection}->{sock})");
            $self->debug(1,"Connect: SSL: We are secure") if ($self->{SIDS}->{newconnection}->{sock});
        }
        return unless $self->{SIDS}->{newconnection}->{sock};
    }

    $self->debug(1,"Connect: Got a connection");

    $self->{SIDS}->{newconnection}->{sock}->autoflush(1);

    return $self->OpenStream("newconnection",$timeout);
}


##############################################################################
#
# OpenStream - Send the opening stream and save the root element info.
#
##############################################################################
sub OpenStream
{
    my $self = shift;
    my $currsid = shift;
    my $timeout = shift;
    $timeout = "" unless defined($timeout);

    $self->InitConnection($currsid,$currsid);

    #---------------------------------------------------------------------------
    # Next, we build the opening handshake.
    #---------------------------------------------------------------------------
    my %stream_args;
    
    if (($self->{SIDS}->{$currsid}->{connectiontype} eq "tcpip") ||
        ($self->{SIDS}->{$currsid}->{connectiontype} eq "http"))
    {
        $stream_args{to}= $self->{SIDS}->{$currsid}->{hostname}
            unless exists($self->{SIDS}->{$currsid}->{to});
        
        $stream_args{to} = $self->{SIDS}->{$currsid}->{to}
            if exists($self->{SIDS}->{$currsid}->{to});

        $stream_args{from} = $self->{SIDS}->{$currsid}->{myhostname}
            if (!exists($self->{SIDS}->{$currsid}->{from}) &&
                ($self->{SIDS}->{$currsid}->{myhostname} ne "")
               );
        
        $stream_args{from} = $self->{SIDS}->{$currsid}->{from}
            if exists($self->{SIDS}->{$currsid}->{from});
        
        $stream_args{id} = $self->{SIDS}->{$currsid}->{id}
            if (exists($self->{SIDS}->{$currsid}->{id}) &&
                ($self->{SIDS}->{$currsid}->{id} ne "")
               );

        $stream_args{namespaces} = $self->{SIDS}->{$currsid}->{namespaces};
    }
    
    my $stream =
        $self->StreamHeader(
            xmlns=>$self->{SIDS}->{$currsid}->{namespace},
            xmllang=>"en",
            %stream_args
        );

    #---------------------------------------------------------------------------
    # Create the XML::Stream::Parser and register our callbacks
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$currsid}->{parser} =
        new XML::Stream::Parser(%{$self->{DEBUGARGS}},
                                nonblocking=>$NONBLOCKING,
                                sid=>$currsid,
                                style=>$self->{DATASTYLE},
                                Handlers=>{
                                    startElement=>sub{ $self->_handle_root(@_) },
                                    endElement=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{endElement}}($self,@_) },
                                    characters=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{characters}}($self,@_) },
                                }
                               );

    $self->{SIDS}->{$currsid}->{select} =
        new IO::Select($self->{SIDS}->{$currsid}->{sock});

    if (($self->{SIDS}->{$currsid}->{connectiontype} eq "tcpip") ||
            ($self->{SIDS}->{$currsid}->{connectiontype} eq "http"))
    {
        $self->{SELECT} = new IO::Select($self->{SIDS}->{$currsid}->{sock});
        $self->{SOCKETS}->{$self->{SIDS}->{$currsid}->{sock}} = "newconnection";
    }

    if ($self->{SIDS}->{$currsid}->{connectiontype} eq "stdinout")
    {
        $self->{SELECT} = new IO::Select(*STDIN);
        $self->{SOCKETS}->{$self->{SIDS}->{$currsid}->{sock}} = $currsid;
        $self->{SOCKETS}->{*STDIN} = $currsid;
        $self->{SIDS}->{$currsid}->{select}->add(*STDIN);
    }

    $self->{SIDS}->{$currsid}->{status} = 0;

    #---------------------------------------------------------------------------
    # Then we send the opening handshake.
    #---------------------------------------------------------------------------
    $self->Send($currsid,$stream) || return;

    #---------------------------------------------------------------------------
    # Before going on let's make sure that the server responded with a valid
    # root tag and that the stream is open.
    #---------------------------------------------------------------------------
    my $buff = "";
    my $timeEnd = ($timeout eq "") ? "" : time + $timeout;
    while($self->{SIDS}->{$currsid}->{status} == 0)
    {
        my $now = time;
        my $wait = (($timeEnd eq "") || ($timeEnd - $now > 10)) ? 10 :
                    $timeEnd - $now;

        $self->debug(5,"Connect: can_read(",join(",",$self->{SIDS}->{$currsid}->{select}->can_read(0)),")");
        if ($self->{SIDS}->{$currsid}->{select}->can_read($wait))
        {
            $self->{SIDS}->{$currsid}->{status} = -1
                unless defined($buff = $self->Read($currsid));
            return unless($self->{SIDS}->{$currsid}->{status} == 0);
            return unless($self->ParseStream($currsid,$buff) == 1);
        }
        else
        {
            if ($timeout ne "")
            {
                if (time >= $timeEnd)
                {
                    $self->SetErrorCode($currsid,"Timeout limit reached");
                    return;
                }
            }
        }

        return if($self->{SIDS}->{$currsid}->{select}->has_exception(0));
    }
    return if($self->{SIDS}->{$currsid}->{status} != 1);

    $self->debug(3,"Connect: status($self->{SIDS}->{$currsid}->{status})");

    my $sid = $self->GetRoot($currsid)->{id};
    $| = 1;
    foreach my $key (keys(%{$self->{SIDS}->{$currsid}}))
    {
        $self->{SIDS}->{$sid}->{$key} = $self->{SIDS}->{$currsid}->{$key};
    }
    $self->{SIDS}->{$sid}->{parser}->setSID($sid);

    if (($self->{SIDS}->{$sid}->{connectiontype} eq "tcpip") ||
        ($self->{SIDS}->{$sid}->{connectiontype} eq "http"))
    {
        $self->{SOCKETS}->{$self->{SIDS}->{$currsid}->{sock}} = $sid;
    }

    if ($self->{SIDS}->{$sid}->{connectiontype} eq "stdinout")
    {
        $self->{SOCKETS}->{$self->{SIDS}->{$currsid}->{sock}} = $sid;
        $self->{SOCKETS}->{*STDIN} = $sid;
    }

    # 08.04.05(Fri) slipstream@yandex.ru for compapility with ejabberd since it reuses stream id
    delete($self->{SIDS}->{$currsid}) unless ($currsid eq $sid);

    if (exists($self->GetRoot($sid)->{version}) &&
        ($self->GetRoot($sid)->{version} ne ""))
    {
        while(!$self->ReceivedStreamFeatures($sid))
        {
            $self->Process(1);
        }
    }
        
    return $self->GetRoot($sid);
}


##############################################################################
#
# OpenFile - starts the stream by opening a file and setting it up so that
#            Process reads from the filehandle to get the incoming stream.
#
##############################################################################
sub OpenFile
{
    my $self = shift;
    my $file = shift;

    $self->debug(1,"OpenFile: file($file)");

    $self->{SIDS}->{newconnection}->{connectiontype} = "file";

    $self->{SIDS}->{newconnection}->{sock} = new FileHandle($file);
    $self->{SIDS}->{newconnection}->{sock}->autoflush(1);

    $self->RegisterPrefix("newconnection",&ConstXMLNS("stream"),"stream");

    #---------------------------------------------------------------------------
    # Create the XML::Stream::Parser and register our callbacks
    #---------------------------------------------------------------------------
    $self->{SIDS}->{newconnection}->{parser} =
        new XML::Stream::Parser(%{$self->{DEBUGARGS}},
                    nonblocking=>$NONBLOCKING,
                    sid=>"newconnection",
                    style=>$self->{DATASTYLE},
                    Handlers=>{
                         startElement=>sub{ $self->_handle_root(@_) },
                         endElement=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{endElement}}($self,@_) },
                         characters=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{characters}}($self,@_) },
                        }
                 );

    $self->{SIDS}->{newconnection}->{select} =
        new IO::Select($self->{SIDS}->{newconnection}->{sock});

    $self->{SELECT} = new IO::Select($self->{SIDS}->{newconnection}->{sock});

    $self->{SIDS}->{newconnection}->{status} = 0;

    my $buff = "";
    while($self->{SIDS}->{newconnection}->{status} == 0)
    {
        $self->debug(5,"OpenFile: can_read(",join(",",$self->{SIDS}->{newconnection}->{select}->can_read(0)),")");
        if ($self->{SIDS}->{newconnection}->{select}->can_read(0))
        {
            $self->{SIDS}->{newconnection}->{status} = -1
                unless defined($buff = $self->Read("newconnection"));
            return unless($self->{SIDS}->{newconnection}->{status} == 0);
            return unless($self->ParseStream("newconnection",$buff) == 1);
        }

        return if($self->{SIDS}->{newconnection}->{select}->has_exception(0) &&
                  $self->{SIDS}->{newconnection}->{sock}->error());
    }
    return if($self->{SIDS}->{newconnection}->{status} != 1);


    my $sid = $self->NewSID();
    foreach my $key (keys(%{$self->{SIDS}->{newconnection}}))
    {
        $self->{SIDS}->{$sid}->{$key} = $self->{SIDS}->{newconnection}->{$key};
    }
    $self->{SIDS}->{$sid}->{parser}->setSID($sid);

    $self->{SOCKETS}->{$self->{SIDS}->{newconnection}->{sock}} = $sid;

    delete($self->{SIDS}->{newconnection});

    return $sid;
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Common Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# Disconnect - sends the closing XML tag and shuts down the socket.
#
##############################################################################
sub Disconnect
{
    my $self = shift;
    my $sid = shift;

    $self->Send($sid,"</stream:stream>");
    close($self->{SIDS}->{$sid}->{sock})
        if (($self->{SIDS}->{$sid}->{connectiontype} eq "tcpip") ||
    ($self->{SIDS}->{$sid}->{connectiontype} eq "http"));
    delete($self->{SOCKETS}->{$self->{SIDS}->{$sid}->{sock}});
    foreach my $key (keys(%{$self->{SIDS}->{$sid}}))
    {
        delete($self->{SIDS}->{$sid}->{$key});
    }
    delete($self->{SIDS}->{$sid});
}


##############################################################################
#
# InitConnection - Initialize the connection data structure
#
##############################################################################
sub InitConnection
{
    my $self = shift;
    my $sid = shift;
    my $serverid = shift;

    #---------------------------------------------------------------------------
    # Set the default STATUS so that we can keep track of it throughout the
    # session.
    #   1 = no errors
    #   0 = no data has been received yet
    #  -1 = error from handlers
    #  -2 = error but keep the connection alive so that we can send some info.
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{status} = 0;

    #---------------------------------------------------------------------------
    # A storage place for when we don't have a callback registered and we need
    # to stockpile the nodes we receive until Process is called and we return
    # them.
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{nodes} = ();

    #---------------------------------------------------------------------------
    # If there is an error on the stream, then we need a place to indicate that.
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{streamerror} = {};

    #---------------------------------------------------------------------------
    # Grab the init time so that we can keep the connection alive by sending " "
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{keepalive} = time;

    #---------------------------------------------------------------------------
    # Keep track of the "server" we are connected to so we can check stuff
    # later.
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{serverid} = $serverid;

    #---------------------------------------------------------------------------
    # Mark the stream:features as MIA.
    #---------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{streamfeatures}->{received} = 0;
    
    #---------------------------------------------------------------------------
    # First acitivty is the connection... duh. =)
    #---------------------------------------------------------------------------
    $self->MarkActivity($sid);
}


##############################################################################
#
# ParseStream - takes the incoming stream and makes sure that only full
#               XML tags gets passed to the parser.  If a full tag has not
#               read yet, then the Stream saves the incomplete part and
#               sends the rest to the parser.
#
##############################################################################
sub ParseStream
{
    my $self = shift;
    my $sid = shift;
    my $stream = shift;

    $stream = "" unless defined($stream);

    $self->debug(3,"ParseStream: sid($sid) stream($stream)");

    $self->{SIDS}->{$sid}->{parser}->parse($stream);

    if (exists($self->{SIDS}->{$sid}->{streamerror}->{type}))
    {
        $self->debug(3,"ParseStream: ERROR($self->{SIDS}->{$sid}->{streamerror}->{type})");
        $self->SetErrorCode($sid,$self->{SIDS}->{$sid}->{streamerror});
        return 0;
    }

    return 1;
}


##############################################################################
#
# Process - checks for data on the socket and returns a status code depending
#           on if there was data or not.  If a timeout is not defined in the
#           call then the timeout defined in Connect() is used.  If a timeout
#           of 0 is used then the call blocks until it gets some data,
#           otherwise it returns after the timeout period.
#
##############################################################################
sub Process
{
    my $self = shift;
    my $timeout = shift;
    $timeout = "" unless defined($timeout);

    $self->debug(4,"Process: timeout($timeout)");
    #---------------------------------------------------------------------------
    # We need to keep track of what's going on in the function and tell the
    # outside world about it so let's return something useful.  We track this
    # information based on sid:
    #    -1    connection closed and error
    #     0    connection open but no data received.
    #     1    connection open and data received.
    #   array  connection open and the data that has been collected
    #          over time (No CallBack specified)
    #---------------------------------------------------------------------------
    my %status;
    foreach my $sid (keys(%{$self->{SIDS}}))
    {
        next if ($sid eq "default");
        $self->debug(5,"Process: initialize sid($sid) status to 0");
        $status{$sid} = 0;
    }

    #---------------------------------------------------------------------------
    # Either block until there is data and we have parsed it all, or wait a
    # certain period of time and then return control to the user.
    #---------------------------------------------------------------------------
    my $block = 1;
    my $timeEnd = ($timeout eq "") ? "" : time + $timeout;
    while($block == 1)
    {
        $self->debug(4,"Process: let's wait for data");

        my $now = time;
        my $wait = (($timeEnd eq "") || ($timeEnd - $now > 10)) ? 10 :
                    $timeEnd - $now;

        foreach my $connection ($self->{SELECT}->can_read($wait))
        {
            $self->debug(4,"Process: connection($connection)");
            $self->debug(4,"Process: sid($self->{SOCKETS}->{$connection})");
            $self->debug(4,"Process: connection_status($self->{SIDS}->{$self->{SOCKETS}->{$connection}}->{status})");

            next unless (($self->{SIDS}->{$self->{SOCKETS}->{$connection}}->{status} == 1) ||
                         exists($self->{SIDS}->{$self->{SOCKETS}->{$connection}}->{activitytimeout}));

            my $processit = 1;
            if (exists($self->{SIDS}->{server}))
            {
                foreach my $serverid (@{$self->{SIDS}->{server}})
                {
                    if (exists($self->{SIDS}->{$serverid}->{sock}) &&
                        ($connection == $self->{SIDS}->{$serverid}->{sock}))
                    {
                        my $sid = $self->ConnectionAccept($serverid);
                        $status{$sid} = 0;
                        $processit = 0;
                        last;
                    }
                }
            }
            if ($processit == 1)
            {
                my $sid = $self->{SOCKETS}->{$connection};
                $self->debug(4,"Process: there's something to read");
                $self->debug(4,"Process: connection($connection) sid($sid)");
                my $buff;
                $self->debug(4,"Process: read");
                $status{$sid} = 1;
                $self->{SIDS}->{$sid}->{status} = -1
                    if (!defined($buff = $self->Read($sid)));
                $buff = "" unless defined($buff);
                $self->debug(4,"Process: connection_status($self->{SIDS}->{$sid}->{status})");
                $status{$sid} = -1 unless($self->{SIDS}->{$sid}->{status} == 1);
                $self->debug(4,"Process: parse($buff)");
                $status{$sid} = -1 unless($self->ParseStream($sid,$buff) == 1);
            }
            $block = 0;
        }

        if ($timeout ne "")
        {
            if (time >= $timeEnd)
            {
                $self->debug(4,"Process: Everyone out of the pool! Time to stop blocking.");
                $block = 0;
            }
        }

        $self->debug(4,"Process: timeout($timeout)");

        if (exists($self->{CB}->{update}))
        {
            $self->debug(4,"Process: Calling user defined update function");
            &{$self->{CB}->{update}}();
        }

        $block = 1 if $self->{SELECT}->can_read(0);

        #---------------------------------------------------------------------
        # Check for connections that need to be kept alive
        #---------------------------------------------------------------------
        $self->debug(4,"Process: check for keepalives");
        foreach my $sid (keys(%{$self->{SIDS}}))
        {
            next if ($sid eq "default");
            next if ($sid =~ /^server/);
            next if ($status{$sid} == -1);
            if ((time - $self->{SIDS}->{$sid}->{keepalive}) > 10)
            {
                $self->IgnoreActivity($sid,1);
                $self->{SIDS}->{$sid}->{status} = -1
                    if !defined($self->Send($sid," "));
                $status{$sid} = -1 unless($self->{SIDS}->{$sid}->{status} == 1);
                if ($status{$sid} == -1)
                {
                    $self->debug(2,"Process: Keep-Alive failed.  What the hell happened?!?!");
                    $self->debug(2,"Process: connection_status($self->{SIDS}->{$sid}->{status})");
                }
                $self->IgnoreActivity($sid,0);
            }
        }
        #---------------------------------------------------------------------
        # Check for connections that have timed out.
        #---------------------------------------------------------------------
        $self->debug(4,"Process: check for timeouts");
        foreach my $sid (keys(%{$self->{SIDS}}))
        {
            next if ($sid eq "default");
            next if ($sid =~ /^server/);

            if (exists($self->{SIDS}->{$sid}->{activitytimeout}))
            {
                $self->debug(4,"Process: sid($sid) time(",time,") timeout($self->{SIDS}->{$sid}->{activitytimeout})");
            }
            else
            {
                $self->debug(4,"Process: sid($sid) time(",time,") timeout(undef)");
            }
            
            $self->Respond($sid)
                if (exists($self->{SIDS}->{$sid}->{activitytimeout}) &&
                    defined($self->GetRoot($sid)));
            $self->Disconnect($sid)
                if (exists($self->{SIDS}->{$sid}->{activitytimeout}) &&
                    ((time - $self->{SIDS}->{$sid}->{activitytimeout}) > 10) &&
                     ($self->{SIDS}->{$sid}->{status} != 1));
        }


        #---------------------------------------------------------------------
        # If any of the connections have status == -1 then return so that the
        # user can handle it.
        #---------------------------------------------------------------------
        foreach my $sid (keys(%status))
        {
            if ($status{$sid} == -1)
            {
                $self->debug(4,"Process: sid($sid) is broken... let's tell someone and watch it hit the fan... =)");
                $block = 0;
            }
        }

        $self->debug(2,"Process: block($block)");
    }

    #---------------------------------------------------------------------------
    # If the Select has an error then shut this party down.
    #---------------------------------------------------------------------------
    foreach my $connection ($self->{SELECT}->has_exception(0))
    {
        $self->debug(4,"Process: has_exception sid($self->{SOCKETS}->{$connection})");
        $status{$self->{SOCKETS}->{$connection}} = -1;
    }

    #---------------------------------------------------------------------------
    # If there are data structures that have not been collected return
    # those, otherwise return the status which indicates if nodes were read or
    # not.
    #---------------------------------------------------------------------------
    foreach my $sid (keys(%status))
    {
        $status{$sid} = $self->{SIDS}->{$sid}->{nodes}
            if (($status{$sid} == 1) &&
                ($#{$self->{SIDS}->{$sid}->{nodes}} > -1));
    }

    return %status;
}


##############################################################################
#
# Read - Takes the data from the server and returns a string
#
##############################################################################
sub Read
{
    my $self = shift;
    my $sid = shift;
    my $buff;
    my $status = 1;

    $self->debug(3,"Read: sid($sid)");
    $self->debug(3,"Read: connectionType($self->{SIDS}->{$sid}->{connectiontype})");
    $self->debug(3,"Read: socket($self->{SIDS}->{$sid}->{sock})");

    return if ($self->{SIDS}->{$sid}->{status} == -1);

    if (!defined($self->{SIDS}->{$sid}->{sock}))
    {
        $self->{SIDS}->{$sid}->{status} = -1;
        $self->SetErrorCode($sid,"Socket does not defined.");
        return;
    }

    $self->{SIDS}->{$sid}->{sock}->flush();

    $status = $self->{SIDS}->{$sid}->{sock}->sysread($buff,4*POSIX::BUFSIZ)
        if (($self->{SIDS}->{$sid}->{connectiontype} eq "tcpip") ||
    ($self->{SIDS}->{$sid}->{connectiontype} eq "http") ||
    ($self->{SIDS}->{$sid}->{connectiontype} eq "file"));
    $status = sysread(STDIN,$buff,1024)
        if ($self->{SIDS}->{$sid}->{connectiontype} eq "stdinout");

    $buff =~ s/^HTTP[\S\s]+\n\n// if ($self->{SIDS}->{$sid}->{connectiontype} eq "http");
    $self->debug(1,"Read: buff($buff)");
    $self->debug(3,"Read: status($status)") if defined($status);
    $self->debug(3,"Read: status(undef)") unless defined($status);
    $self->{SIDS}->{$sid}->{keepalive} = time
        unless (($buff eq "") || !defined($status) || ($status == 0));
    if (defined($status) && ($status != 0))
    {
        $buff = Encode::decode_utf8($buff);
        return $buff;
    }
    #return $buff unless (!defined($status) || ($status == 0));
    $self->debug(1,"Read: ERROR");
    return;
}


##############################################################################
#
# Send - Takes the data string and sends it to the server
#
##############################################################################
sub Send
{
    my $self = shift;
    my $sid = shift;
    $self->debug(1,"Send: (@_)");
    $self->debug(3,"Send: sid($sid)");
    $self->debug(3,"Send: status($self->{SIDS}->{$sid}->{status})");
    
    $self->{SIDS}->{$sid}->{keepalive} = time;

    return if ($self->{SIDS}->{$sid}->{status} == -1);

    if (!defined($self->{SIDS}->{$sid}->{sock}))
    {
        $self->debug(3,"Send: socket not defined");
        $self->{SIDS}->{$sid}->{status} = -1;
        $self->SetErrorCode($sid,"Socket not defined.");
        return;
    }
    else
    {
        $self->debug(3,"Send: socket($self->{SIDS}->{$sid}->{sock})");
    }

    $self->{SIDS}->{$sid}->{sock}->flush();

    if ($self->{SIDS}->{$sid}->{select}->can_write(0))
    {
        $self->debug(3,"Send: can_write");

        $self->{SENDSTRING} = Encode::encode_utf8(join("",@_));

        $self->{SENDWRITTEN} = 0;
        $self->{SENDOFFSET} = 0;
        $self->{SENDLENGTH} = length($self->{SENDSTRING});
        while ($self->{SENDLENGTH})
        {
            $self->{SENDWRITTEN} = $self->{SIDS}->{$sid}->{sock}->syswrite($self->{SENDSTRING},$self->{SENDLENGTH},$self->{SENDOFFSET});

            if (!defined($self->{SENDWRITTEN}))
            {
                $self->debug(4,"Send: SENDWRITTEN(undef)");
                $self->debug(4,"Send: Ok... what happened?  Did we lose the connection?");
                $self->{SIDS}->{$sid}->{status} = -1;
                $self->SetErrorCode($sid,"Socket died for an unknown reason.");
                return;
            }
            
            $self->debug(4,"Send: SENDWRITTEN($self->{SENDWRITTEN})");

            $self->{SENDLENGTH} -= $self->{SENDWRITTEN};
            $self->{SENDOFFSET} += $self->{SENDWRITTEN};
        }
    }
    else
    {
        $self->debug(3,"Send: can't write...");
    }

    return if($self->{SIDS}->{$sid}->{select}->has_exception(0));

    $self->debug(3,"Send: no exceptions");

    $self->{SIDS}->{$sid}->{keepalive} = time;

    $self->MarkActivity($sid);

    return 1;
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Feature Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# ProcessStreamFeatures - process the <stream:featutres/> block.
#
##############################################################################
sub ProcessStreamFeatures
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    $self->{SIDS}->{$sid}->{streamfeatures}->{received} = 1;

    #-------------------------------------------------------------------------
    # SASL - 1.0
    #-------------------------------------------------------------------------
    my @sasl = &XPath($node,'*[@xmlns="'.&ConstXMLNS('xmpp-sasl').'"]');
    if ($#sasl > -1)
    {
        if (&XPath($sasl[0],"name()") eq "mechanisms")
        {
            my @mechanisms = &XPath($sasl[0],"mechanism/text()");
            $self->{SIDS}->{$sid}->{streamfeatures}->{'xmpp-sasl'} = \@mechanisms;
        }
    }
    
    #-------------------------------------------------------------------------
    # XMPP-TLS - 1.0
    #-------------------------------------------------------------------------
    my @tls = &XPath($node,'*[@xmlns="'.&ConstXMLNS('xmpp-tls').'"]');
    if ($#tls > -1)
    {
        if (&XPath($tls[0],"name()") eq "starttls")
        {
            $self->{SIDS}->{$sid}->{streamfeatures}->{'xmpp-tls'} = 1;
            my @required = &XPath($tls[0],"required");
            if ($#required > -1)
            {
                $self->{SIDS}->{$sid}->{streamfeatures}->{'xmpp-tls'} = "required";
            }
        }
    }
    
    #-------------------------------------------------------------------------
    # XMPP-Bind - 1.0
    #-------------------------------------------------------------------------
    my @bind = &XPath($node,'*[@xmlns="'.&ConstXMLNS('xmpp-bind').'"]');
    if ($#bind > -1)
    {
        $self->{SIDS}->{$sid}->{streamfeatures}->{'xmpp-bind'} = 1;
    }
    
    #-------------------------------------------------------------------------
    # XMPP-Session - 1.0
    #-------------------------------------------------------------------------
    my @session = &XPath($node,'*[@xmlns="'.&ConstXMLNS('xmpp-session').'"]');
    if ($#session > -1)
    {
        $self->{SIDS}->{$sid}->{streamfeatures}->{'xmpp-session'} = 1;
    }
    
}


##############################################################################
#
# GetStreamFeature - Return the value of the stream feature (if any).
#
##############################################################################
sub GetStreamFeature
{
    my $self = shift;
    my $sid = shift;
    my $feature = shift;

    return unless exists($self->{SIDS}->{$sid}->{streamfeatures}->{$feature});
    return $self->{SIDS}->{$sid}->{streamfeatures}->{$feature};
}


##############################################################################
#
# ReceivedStreamFeatures - Have we received the stream:features yet?
#
##############################################################################
sub ReceivedStreamFeatures
{
    my $self = shift;
    my $sid = shift;
    my $feature = shift;

    return $self->{SIDS}->{$sid}->{streamfeatures}->{received};
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| TLS Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# ProcessTLSPacket - process a TLS based packet.
#
##############################################################################
sub ProcessTLSPacket
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    my $tag = &XPath($node,"name()");

    if ($tag eq "failure")
    {
        $self->TLSClientFailure($sid,$node);
    }
    
    if ($tag eq "proceed")
    {
        $self->TLSClientProceed($sid,$node);
    }
}


##############################################################################
#
# StartTLS - client function to have the socket start TLS.
#
##############################################################################
sub StartTLS
{
    my $self = shift;
    my $sid = shift;
    my $timeout = shift;
    $timeout = 120 unless defined($timeout);
    $timeout = 120 if ($timeout eq "");
    
    $self->TLSStartTLS($sid);

    my $endTime = time + $timeout;
    while(!$self->TLSClientDone($sid) && ($endTime >= time))
    {
        $self->Process(1);
    }

    if (!$self->TLSClientSecure($sid))
    {
        return;
    }

    return $self->OpenStream($sid,$timeout);
}


##############################################################################
#
# TLSStartTLS - send a <starttls/> in the TLS namespace.
#
##############################################################################
sub TLSStartTLS
{
    my $self = shift;
    my $sid = shift;

    $self->Send($sid,"<starttls xmlns='".&ConstXMLNS('xmpp-tls')."'/>");
}


##############################################################################
#
# TLSClientProceed - handle a <proceed/> packet.
#
##############################################################################
sub TLSClientProceed
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    $self->debug(1,"TLSClientProceed: Convert normal socket to SSL");
    $self->debug(1,"TLSClientProceed: sock($self->{SIDS}->{$sid}->{sock})");
    if (!$self->LoadSSL())
    {
        $self->{SIDS}->{$sid}->{tls}->{error} = "Could not load IO::Socket::SSL.";
        $self->{SIDS}->{$sid}->{tls}->{done} = 1;
        return;
    }
    
    IO::Socket::SSL->start_SSL($self->{SIDS}->{$sid}->{sock},{SSL_verify_mode=>0x00});

    $self->debug(1,"TLSClientProceed: ssl_sock($self->{SIDS}->{$sid}->{sock})");
    $self->debug(1,"TLSClientProceed: SSL: We are secure")
        if ($self->{SIDS}->{$sid}->{sock});
    
    $self->{SIDS}->{$sid}->{tls}->{done} = 1;
    $self->{SIDS}->{$sid}->{tls}->{secure} = 1;
}


##############################################################################
#
# TLSClientSecure - return 1 if the socket is secure, 0 otherwise.
#
##############################################################################
sub TLSClientSecure
{
    my $self = shift;
    my $sid = shift;
    
    return $self->{SIDS}->{$sid}->{tls}->{secure};
}


##############################################################################
#
# TLSClientDone - return 1 if the TLS process is done
#
##############################################################################
sub TLSClientDone
{
    my $self = shift;
    my $sid = shift;
    
    return $self->{SIDS}->{$sid}->{tls}->{done};
}


##############################################################################
#
# TLSClientError - return the TLS error if any
#
##############################################################################
sub TLSClientError
{
    my $self = shift;
    my $sid = shift;
    
    return $self->{SIDS}->{$sid}->{tls}->{error};
}


##############################################################################
#
# TLSClientFailure - handle a <failure/>
#
##############################################################################
sub TLSClientFailure
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;
    
    my $type = &XPath($node,"*/name()");

    $self->{SIDS}->{$sid}->{tls}->{error} = $type;
    $self->{SIDS}->{$sid}->{tls}->{done} = 1;
}


##############################################################################
#
# TLSFailure - Send a <failure/> in the TLS namespace
#
##############################################################################
sub TLSFailure
{
    my $self = shift;
    my $sid = shift;
    my $type = shift;
    
    $self->Send($sid,"<failure xmlns='".&ConstXMLNS('xmpp-tls')."'><${type}/></failure>");
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| SASL Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# ProcessSASLPacket - process a SASL based packet.
#
##############################################################################
sub ProcessSASLPacket
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    my $tag = &XPath($node,"name()");

    if ($tag eq "challenge")
    {
        $self->SASLAnswerChallenge($sid,$node);
    }
    
    if ($tag eq "failure")
    {
        $self->SASLClientFailure($sid,$node);
    }
    
    if ($tag eq "success")
    {
        $self->SASLClientSuccess($sid,$node);
    }
}


##############################################################################
#
# SASLAnswerChallenge - when we get a <challenge/> we need to do the grunt
#                       work to return a <response/>.
#
##############################################################################
sub SASLAnswerChallenge
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    my $challenge64 = &XPath($node,"text()");
    my $challenge = MIME::Base64::decode_base64($challenge64);
    
    #-------------------------------------------------------------------------
    # As far as I can tell, if the challenge contains rspauth, then we authed.
    # If you try to send that to Authen::SASL, it will spew warnings about
    # the missing qop, nonce, etc...  However, in order for jabberd2 to think
    # that you answered, you have to send back an empty response.  Not sure
    # which approach is right... So let's hack for now.
    #-------------------------------------------------------------------------
    my $response = "";
    if ($challenge !~ /rspauth\=/)
    {
        $response = $self->{SIDS}->{$sid}->{sasl}->{client}->client_step($challenge);
    }

    my $response64 = MIME::Base64::encode_base64($response,"");
    $self->SASLResponse($sid,$response64);
}


##############################################################################
#
# SASLAuth - send an <auth/> in the SASL namespace
#
##############################################################################
sub SASLAuth
{
    my $self = shift;
    my $sid = shift;

    my $first_step = $self->{SIDS}->{$sid}->{sasl}->{client}->client_start();
    my $first_step64 = MIME::Base64::encode_base64($first_step,"");

    $self->Send($sid,"<auth xmlns='".&ConstXMLNS('xmpp-sasl')."' mechanism='".$self->{SIDS}->{$sid}->{sasl}->{client}->mechanism()."'>".$first_step64."</auth>");
}


##############################################################################
#
# SASLChallenge - Send a <challenge/> in the SASL namespace
#
##############################################################################
sub SASLChallenge
{
    my $self = shift;
    my $sid = shift;
    my $challenge = shift;

    $self->Send($sid,"<challenge xmlns='".&ConstXMLNS('xmpp-sasl')."'>${challenge}</challenge>");
}


###############################################################################
#
# SASLClient - This is a helper function to perform all of the required steps
#              for doing SASL with the server.
#
###############################################################################
sub SASLClient
{
    my $self = shift;
    my $sid = shift;
    my $username = shift;
    my $password = shift;

    my $mechanisms = $self->GetStreamFeature($sid,"xmpp-sasl");

    return unless defined($mechanisms);
    
    my $sasl = new Authen::SASL(mechanism=>join(" ",@{$mechanisms}),
                                callback=>{
#                                           authname => $username."@".$self->{SIDS}->{$sid}->{hostname},
                                           user     => $username,
                                           pass     => $password
                                          }
                               );

    $self->{SIDS}->{$sid}->{sasl}->{client} = $sasl->client_new('xmpp', $self->{SIDS}->{$sid}->{hostname});
    $self->{SIDS}->{$sid}->{sasl}->{username} = $username;
    $self->{SIDS}->{$sid}->{sasl}->{password} = $password;
    $self->{SIDS}->{$sid}->{sasl}->{authed} = 0;
    $self->{SIDS}->{$sid}->{sasl}->{done} = 0;

    $self->SASLAuth($sid);
}


##############################################################################
#
# SASLClientAuthed - return 1 if we authed via SASL, 0 otherwise
#
##############################################################################
sub SASLClientAuthed
{
    my $self = shift;
    my $sid = shift;
    
    return $self->{SIDS}->{$sid}->{sasl}->{authed};
}


##############################################################################
#
# SASLClientDone - return 1 if the SASL process is finished
#
##############################################################################
sub SASLClientDone
{
    my $self = shift;
    my $sid = shift;
    
    return $self->{SIDS}->{$sid}->{sasl}->{done};
}


##############################################################################
#
# SASLClientError - return the error if any
#
##############################################################################
sub SASLClientError
{
    my $self = shift;
    my $sid = shift;
    
    return $self->{SIDS}->{$sid}->{sasl}->{error};
}


##############################################################################
#
# SASLClientFailure - handle a received <failure/>
#
##############################################################################
sub SASLClientFailure
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;
    
    my $type = &XPath($node,"*/name()");

    $self->{SIDS}->{$sid}->{sasl}->{error} = $type;
    $self->{SIDS}->{$sid}->{sasl}->{done} = 1;
}


##############################################################################
#
# SASLClientSuccess - handle a received <success/>
#
##############################################################################
sub SASLClientSuccess
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;
    
    $self->{SIDS}->{$sid}->{sasl}->{authed} = 1;
    $self->{SIDS}->{$sid}->{sasl}->{done} = 1;
}


##############################################################################
#
# SASLFailure - Send a <failure/> tag in the SASL namespace
#
##############################################################################
sub SASLFailure
{
    my $self = shift;
    my $sid = shift;
    my $type = shift;
    
    $self->Send($sid,"<failure xmlns='".&ConstXMLNS('xmpp-sasl')."'><${type}/></failure>");
}


##############################################################################
#
# SASLResponse - Send a <response/> tag in the SASL namespace
#
##############################################################################
sub SASLResponse
{
    my $self = shift;
    my $sid = shift;
    my $response = shift;

    $self->Send($sid,"<response xmlns='".&ConstXMLNS('xmpp-sasl')."'>${response}</response>");
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Packet Handlers
#|
#+----------------------------------------------------------------------------
##############################################################################


##############################################################################
#
# ProcessStreamPacket - process the <stream:XXXX/> packet
#
##############################################################################
sub ProcessStreamPacket
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    my $tag = &XPath($node,"name()");
    my $stream_prefix = $self->StreamPrefix($sid);
    my ($type) = ($tag =~ /^${stream_prefix}\:(.+)$/);

    $self->ProcessStreamError($sid,$node) if ($type eq "error");
    $self->ProcessStreamFeatures($sid,$node) if ($type eq "features");
}


##############################################################################
#
# _handle_root - handles a root tag and checks that it is a stream:stream tag
#                with the proper namespace.  If not then it sets the STATUS
#                to -1 and let's the outer code know that an error occurred.
#                Then it changes the Start tag handlers to the methond listed
#                in $self->{DATASTYLE}
#
##############################################################################
sub _handle_root
{
    my $self = shift;
    my ($sax, $tag, %att) = @_;
    my $sid = $sax->getSID();

    $self->debug(2,"_handle_root: sid($sid) sax($sax) tag($tag) att(",%att,")");

    $self->{SIDS}->{$sid}->{rootTag} = $tag;

    if ($self->{SIDS}->{$sid}->{connectiontype} ne "file")
    {
        #---------------------------------------------------------------------
        # Make sure we are receiving a valid stream on the same namespace.
        #---------------------------------------------------------------------
        
        $self->debug(3,"_handle_root: ($self->{SIDS}->{$self->{SIDS}->{$sid}->{serverid}}->{namespace})");
        $self->{SIDS}->{$sid}->{status} =
            ((($tag eq "stream:stream") &&
               exists($att{'xmlns'}) &&
               ($att{'xmlns'} eq $self->{SIDS}->{$self->{SIDS}->{$sid}->{serverid}}->{namespace})
              ) ?
              1 :
              -1
            );
        $self->debug(3,"_handle_root: status($self->{SIDS}->{$sid}->{status})");
    }
    else
    {
        $self->{SIDS}->{$sid}->{status} = 1;
    }

    #-------------------------------------------------------------------------
    # Get the root tag attributes and save them for later.  You never know when
    # you'll need to check the namespace or the from attributes sent by the
    # server.
    #-------------------------------------------------------------------------
    $self->{SIDS}->{$sid}->{root} = \%att;

    #-------------------------------------------------------------------------
    # Run through the various xmlns:*** attributes and register the namespace
    # to prefix map.
    #-------------------------------------------------------------------------
    foreach my $key (keys(%att))
    {
        if ($key =~ /^xmlns\:(.+?)$/)
        {
            $self->debug(5,"_handle_root: RegisterPrefix: prefix($att{$key}) ns($1)");
            $self->RegisterPrefix($sid,$att{$key},$1);
        }
    }
    
    #-------------------------------------------------------------------------
    # Sometimes we will get an error, so let's parse the tag assuming that we
    # got a stream:error
    #-------------------------------------------------------------------------
    my $stream_prefix = $self->StreamPrefix($sid);
    $self->debug(5,"_handle_root: stream_prefix($stream_prefix)");
    
    if ($tag eq $stream_prefix.":error")
    {
        &XML::Stream::Tree::_handle_element($self,$sax,$tag,%att)
            if ($self->{DATASTYLE} eq "tree");
        &XML::Stream::Node::_handle_element($self,$sax,$tag,%att)
            if ($self->{DATASTYLE} eq "node");
    }

    #---------------------------------------------------------------------------
    # Now that we have gotten a root tag, let's look for the tags that make up
    # the stream.  Change the handler for a Start tag to another function.
    #---------------------------------------------------------------------------
    $sax->setHandlers(startElement=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{startElement}}($self,@_) },
                endElement=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{endElement}}($self,@_) },
                characters=>sub{ &{$HANDLERS{$self->{DATASTYLE}}->{characters}}($self,@_) },
             );
}


##############################################################################
#
# _node - internal callback for nodes.  All it does is place the nodes in a
#         list so that Process() can return them later.
#
##############################################################################
sub _node
{
    my $self = shift;
    my $sid = shift;
    my @node = shift;

    if (ref($node[0]) eq "XML::Stream::Node")
    {
        push(@{$self->{SIDS}->{$sid}->{nodes}},$node[0]);
    }
    else
    {
        push(@{$self->{SIDS}->{$sid}->{nodes}},\@node);
    }
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Error Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# GetErrorCode - if you are returned an undef, you can call this function
#                and hopefully learn more information about the problem.
#
##############################################################################
sub GetErrorCode
{
    my $self = shift;
    my $sid = shift;

    $sid = "newconnection" unless defined($sid);

    $self->debug(3,"GetErrorCode: sid($sid)");
    return ((exists($self->{SIDS}->{$sid}->{errorcode}) &&
             (ref($self->{SIDS}->{$sid}->{errorcode}) eq "HASH")) ?
            $self->{SIDS}->{$sid}->{errorcode} :
            { type=>"system",
              text=>$!,
            }
           );
}


##############################################################################
#
# SetErrorCode - sets the error code so that the caller can find out more
#                information about the problem
#
##############################################################################
sub SetErrorCode
{
    my $self = shift;
    my $sid = shift;
    my $errorcode = shift;

    $self->{SIDS}->{$sid}->{errorcode} = $errorcode;
}


##############################################################################
#
# ProcessStreamError - Take the XML packet and extract out the error.
#
##############################################################################
sub ProcessStreamError
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    $self->{SIDS}->{$sid}->{streamerror}->{type} = "unknown";
    $self->{SIDS}->{$sid}->{streamerror}->{node} = $node;
    
    #-------------------------------------------------------------------------
    # Check for older 0.9 streams and handle the errors for them.
    #-------------------------------------------------------------------------
    if (!exists($self->{SIDS}->{$sid}->{root}->{version}) ||
        ($self->{SIDS}->{$sid}->{root}->{version} eq "") ||
        ($self->{SIDS}->{$sid}->{root}->{version} < 1.0)
       )
    {
        $self->{SIDS}->{$sid}->{streamerror}->{text} =
            &XPath($node,"text()");
        return;
    }

    #-------------------------------------------------------------------------
    # Otherwise we are in XMPP land with real stream errors.
    #-------------------------------------------------------------------------
    my @errors = &XPath($node,'*[@xmlns="'.&ConstXMLNS("xmppstreams").'"]');

    my $type;
    my $text;
    foreach my $error (@errors)
    {
        if (&XPath($error,"name()") eq "text")
        {
            $self->{SIDS}->{$sid}->{streamerror}->{text} =
                &XPath($error,"text()");
        }
        else
        {
            $self->{SIDS}->{$sid}->{streamerror}->{type} =
                &XPath($error,"name()");
        }
    }
}


##############################################################################
#
# StreamError - Given a type and text, generate a <stream:error/> packet to
#               send back to the other side.
#
##############################################################################
sub StreamError
{
    my $self = shift;
    my $sid = shift;
    my $type = shift;
    my $text = shift;

    my $root = $self->GetRoot($sid);
    my $stream_base = $self->StreamPrefix($sid);
    my $error = "<${stream_base}:error>";

    if (exists($root->{version}) && ($root->{version} ne ""))
    {
        $error .= "<${type} xmlns='".&ConstXMLNS('xmppstreams')."'/>";
        if (defined($text))
        {
            $error .= "<text xmlns='".&ConstXMLNS('xmppstreams')."'>";
            $error .= $text;
            $error .= "</text>";
        }
    }
    else
    {
        $error .= $text;
    }

    $error .= "</${stream_base}:error>";

    return $error;
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Activity Monitoring Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# IgnoreActivity - Set the flag that will ignore the activity monitor.
#
##############################################################################
sub IgnoreActivity
{
    my $self = shift;
    my $sid = shift;
    my $ignoreActivity = shift;
    $ignoreActivity = 1 unless defined($ignoreActivity);

    $self->debug(3,"IgnoreActivity: ignoreActivity($ignoreActivity)");
    $self->debug(4,"IgnoreActivity: sid($sid)");

    $self->{SIDS}->{$sid}->{ignoreActivity} = $ignoreActivity;
}


##############################################################################
#
# LastActivity - Return the time of the last activity.
#
##############################################################################
sub LastActivity
{
    my $self = shift;
    my $sid = shift;

    $self->debug(3,"LastActivity: sid($sid)");
    $self->debug(1,"LastActivity: lastActivity($self->{SIDS}->{$sid}->{lastActivity})");

    return $self->{SIDS}->{$sid}->{lastActivity};
}


##############################################################################
#
# MarkActivity - Record the current time for this sid.
#
##############################################################################
sub MarkActivity
{
    my $self = shift;
    my $sid = shift;

    return if (exists($self->{SIDS}->{$sid}->{ignoreActivity}) &&
               ($self->{SIDS}->{$sid}->{ignoreActivity} == 1));

    $self->debug(3,"MarkActivity: sid($sid)");

    $self->{SIDS}->{$sid}->{lastActivity} = time;
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| XML Node Interface functions
#|
#|   These are generic wrappers around the Tree and Node data types.  The
#| problem being that the Tree class cannot support methods.
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# SetXMLData - takes a host of arguments and sets a portion of the specified
#              data strucure with that data.  The function works in two
#              modes "single" or "multiple".  "single" denotes that the
#              function should locate the current tag that matches this
#              data and overwrite it's contents with data passed in.
#              "multiple" denotes that a new tag should be created even if
#              others exist.
#
#              type    - single or multiple
#              XMLTree - pointer to XML::Stream data object (tree or node)
#              tag     - name of tag to create/modify (if blank assumes
#                        working with top level tag)
#              data    - CDATA to set for tag
#              attribs - attributes to ADD to tag
#
##############################################################################
sub SetXMLData
{
    return &XML::Stream::Node::SetXMLData(@_) if (ref($_[1]) eq "XML::Stream::Node");
    return &XML::Stream::Tree::SetXMLData(@_) if (ref($_[1]) eq "ARRAY");
}


##############################################################################
#
# GetXMLData - takes a host of arguments and returns various data structures
#              that match them.
#
#              type - "existence" - returns 1 or 0 if the tag exists in the
#                                   top level.
#                     "value" - returns either the CDATA of the tag, or the
#                               value of the attribute depending on which is
#                               sought.  This ignores any mark ups to the data
#                               and just returns the raw CDATA.
#                     "value array" - returns an array of strings representing
#                                     all of the CDATA in the specified tag.
#                                     This ignores any mark ups to the data
#                                     and just returns the raw CDATA.
#                     "tree" - returns a data structure that represents the
#                              XML with the specified tag as the root tag.
#                              Depends on the format that you are working with.
#                     "tree array" - returns an array of data structures each
#                                    with the specified tag as the root tag.
#                     "child array" - returns a list of all children nodes
#                                     not including CDATA nodes.
#                     "attribs" - returns a hash with the attributes, and
#                                 their values, for the things that match
#                                 the parameters
#                     "count" - returns the number of things that match
#                               the arguments
#                     "tag" - returns the root tag of this tree
#              XMLTree - pointer to XML::Stream data structure
#              tag     - tag to pull data from.  If blank then the top level
#                        tag is accessed.
#              attrib  - attribute value to retrieve.  Ignored for types
#                        "value array", "tree", "tree array".  If paired
#                        with value can be used to filter tags based on
#                        attributes and values.
#              value   - only valid if an attribute is supplied.  Used to
#                        filter for tags that only contain this attribute.
#                        Useful to search through multiple tags that all
#                        reference different name spaces.
#
##############################################################################
sub GetXMLData
{
    return &XML::Stream::Node::GetXMLData(@_) if (ref($_[1]) eq "XML::Stream::Node");
    return &XML::Stream::Tree::GetXMLData(@_) if (ref($_[1]) eq "ARRAY");
}


##############################################################################
#
# XPath - run an xpath query on a node and return back the result.
#
##############################################################################
sub XPath
{
    my $tree = shift;
    my $path = shift;
    
    my $query = new XML::Stream::XPath::Query($path);
    my $result = $query->execute($tree);
    if ($result->check())
    {
        my %attribs = $result->getAttribs();
        return %attribs if (scalar(keys(%attribs)) > 0);
        
        my @values = $result->getValues();
        @values = $result->getList() unless ($#values > -1);
        return @values if wantarray;
        return $values[0];
    }
    return;
}


##############################################################################
#
# XPathCheck - run an xpath query on a node and return 1 or 0 if the path is
#              valid.
#
##############################################################################
sub XPathCheck
{
    my $tree = shift;
    my $path = shift;
    
    my $query = new XML::Stream::XPath::Query($path);
    my $result = $query->execute($tree);
    return $result->check();
}


##############################################################################
#
# XML2Config - takes an XML data tree and turns it into a hash of hashes.
#              This only works for certain kinds of XML trees like this:
#
#                <foo>
#                  <bar>1</bar>
#                  <x>
#                    <y>foo</y>
#                  </x>
#                  <z>5</z>
#                  <z>6</z>
#                </foo>
#
#              The resulting hash would be:
#
#                $hash{bar} = 1;
#                $hash{x}->{y} = "foo";
#                $hash{z}->[0] = 5;
#                $hash{z}->[1] = 6;
#
#              Good for config files.
#
##############################################################################
sub XML2Config
{
    return &XML::Stream::Node::XML2Config(@_) if (ref($_[0]) eq "XML::Stream::Node");
    return &XML::Stream::Tree::XML2Config(@_) if (ref($_[0]) eq "ARRAY");
}


##############################################################################
#
# Config2XML - takes a hash and produces an XML string from it.  If the hash
#              looks like this:
#
#                $hash{bar} = 1;
#                $hash{x}->{y} = "foo";
#                $hash{z}->[0] = 5;
#                $hash{z}->[1] = 6;
#
#              The resulting xml would be:
#
#                <foo>
#                  <bar>1</bar>
#                  <x>
#                    <y>foo</y>
#                  </x>
#                  <z>5</z>
#                  <z>6</z>
#                </foo>
#
#              Good for config files.
#
##############################################################################
sub Config2XML
{
    my ($tag,$hash,$indent) = @_;
    $indent = "" unless defined($indent);

    my $xml;

    if (ref($hash) eq "ARRAY")
    {
        foreach my $item (@{$hash})
        {
            $xml .= &XML::Stream::Config2XML($tag,$item,$indent);
        }
    }
    else
    {
        if ((ref($hash) eq "HASH") && ((scalar keys(%{$hash})) == 0))
        {
            $xml .= "$indent<$tag/>\n";
        }
        else
        {
            if (ref($hash) eq "")
            {
                if ($hash eq "")
                {
                    return "$indent<$tag/>\n";
                }
                else
                {
                    return "$indent<$tag>$hash</$tag>\n";
                }
            }
            else
            {
                $xml .= "$indent<$tag>\n";
                foreach my $item (sort {$a cmp $b} keys(%{$hash}))
                {
                    $xml .= &XML::Stream::Config2XML($item,$hash->{$item},"  $indent");
                }
                $xml .= "$indent</$tag>\n";
            }
        }
    }
    return $xml;
}


##############################################################################
#
# EscapeXML - Simple function to make sure that no bad characters make it into
#             in the XML string that might cause the string to be
#             misinterpreted.
#
##############################################################################
sub EscapeXML
{
    my $data = shift;

    if (defined($data))
    {
        $data =~ s/&/&amp;/g;
        $data =~ s/</&lt;/g;
        $data =~ s/>/&gt;/g;
        $data =~ s/\"/&quot;/g;
        $data =~ s/\'/&apos;/g;
    }

    return $data;
}


##############################################################################
#
# UnescapeXML - Simple function to take an escaped string and return it to
#               normal.
#
##############################################################################
sub UnescapeXML
{
    my $data = shift;

    if (defined($data))
    {
        $data =~ s/&amp;/&/g;
        $data =~ s/&lt;/</g;
        $data =~ s/&gt;/>/g;
        $data =~ s/&quot;/\"/g;
        $data =~ s/&apos;/\'/g;
    }

    return $data;
}


##############################################################################
#
# BuildXML - takes one of the data formats that XML::Stream supports and call
#            the proper BuildXML_xxx function on it.
#
##############################################################################
sub BuildXML
{
    return &XML::Stream::Node::BuildXML(@_) if (ref($_[0]) eq "XML::Stream::Node");
    return &XML::Stream::Tree::BuildXML(@_) if (ref($_[0]) eq "ARRAY");
    return &XML::Stream::Tree::BuildXML(@_) if (ref($_[1]) eq "ARRAY");
}



##############################################################################
#+----------------------------------------------------------------------------
#|
#| Namespace/Prefix Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# ConstXMLNS - Return the namespace from the constant string.
#
##############################################################################
sub ConstXMLNS
{
    my $const = shift;
    
    return $XMLNS{$const};
}


##############################################################################
#
# StreamPrefix - Return the prefix of the <stream:stream/>
#
##############################################################################
sub StreamPrefix
{
    my $self = shift;
    my $sid = shift;
    
    return $self->ns2prefix($sid,&ConstXMLNS("stream"));
}


##############################################################################
#
# RegisterPrefix - setup the map for namespace to prefix
#
##############################################################################
sub RegisterPrefix
{
    my $self = shift;
    my $sid = shift;
    my $ns = shift;
    my $prefix = shift;

    $self->{SIDS}->{$sid}->{ns2prefix}->{$ns} = $prefix;
}


##############################################################################
#
# ns2prefix - for a stream, return the prefix for the given namespace
#
##############################################################################
sub ns2prefix
{
    my $self = shift;
    my $sid = shift;
    my $ns = shift;

    return $self->{SIDS}->{$sid}->{ns2prefix}->{$ns};
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Helper Functions
#|
#+----------------------------------------------------------------------------
##############################################################################

##############################################################################
#
# GetRoot - returns the hash of attributes for the root <stream:stream/> tag
#           so that any attributes returned can be accessed.  from and any
#           xmlns:foobar might be important.
#
##############################################################################
sub GetRoot
{
    my $self = shift;
    my $sid = shift;
    return unless exists($self->{SIDS}->{$sid}->{root});
    return $self->{SIDS}->{$sid}->{root};
}


##############################################################################
#
# GetSock - returns the Socket so that an outside function can access it if
#           desired.
#
##############################################################################
sub GetSock
{
    my $self = shift;
    my $sid = shift;
    return $self->{SIDS}->{$sid}->{sock};
}


##############################################################################
#
# LoadSSL - simple call to set everything up for SSL one time.
#
##############################################################################
sub LoadSSL
{
    my $self = shift;

    $self->debug(1,"LoadSSL: Load the IO::Socket::SSL module");
    
    if (defined($SSL) && ($SSL == 1))
    {
        $self->debug(1,"LoadSSL: Success");
        return 1;
    }
    
    if (defined($SSL) && ($SSL == 0))
    {
        $self->debug(1,"LoadSSL: Failure");
        return;
    }

    my $SSL_Version = "0.81";
    eval "use IO::Socket::SSL $SSL_Version";
    if ($@)
    {
        croak("You requested that XML::Stream turn the socket into an SSL socket, but you don't have the correct version of IO::Socket::SSL v$SSL_Version.");
    }
    IO::Socket::SSL::context_init({SSL_verify_mode=>0x00});
    $SSL = 1;

    $self->debug(1,"LoadSSL: Success");
    return 1;
}


##############################################################################
#
# Host2SID - For a server this allows you to lookup the SID of a stream server
#            based on the hostname that is is listening on.
#
##############################################################################
sub Host2SID
{
    my $self = shift;
    my $hostname = shift;

    foreach my $sid (keys(%{$self->{SIDS}}))
    {
        next if ($sid eq "default");
        next if ($sid =~ /^server/);

        return $sid if ($self->{SIDS}->{$sid}->{hostname} eq $hostname);
    }
    return;
}


##############################################################################
#
# NewSID - returns a session ID to send to an incoming stream in the return
#          header.  By default it just increments a counter and returns that,
#          or you can define a function and set it using the SetCallBacks
#          function.
#
##############################################################################
sub NewSID
{
    my $self = shift;
    return &{$self->{CB}->{sid}}() if (exists($self->{CB}->{sid}) &&
                       defined($self->{CB}->{sid}));
    return $$.time.$self->{IDCOUNT}++;
}


###########################################################################
#
# SetCallBacks - Takes a hash with top level tags to look for as the keys
#                and pointers to functions as the values.
#
###########################################################################
sub SetCallBacks
{
    my $self = shift;
    while($#_ >= 0) {
        my $func = pop(@_);
        my $tag = pop(@_);
        if (($tag eq "node") && !defined($func))
        {
            $self->SetCallBacks(node=>sub { $self->_node(@_) });
        }
        else
        {
            $self->debug(1,"SetCallBacks: tag($tag) func($func)");
            $self->{CB}->{$tag} = $func;
        }
    }
}


##############################################################################
#
# StreamHeader - Given the arguments, return the opening stream header.
#
##############################################################################
sub StreamHeader
{
    my $self = shift;
    my (%args) = @_;

    my $stream;
    $stream .= "<?xml version='1.0'?>";
    $stream .= "<stream:stream ";
    $stream .= "version='1.0' ";
    $stream .= "xmlns:stream='".&ConstXMLNS("stream")."' ";
    $stream .= "xmlns='$args{xmlns}' ";
    $stream .= "to='$args{to}' " if exists($args{to});
    $stream .= "from='$args{from}' " if exists($args{from});
    $stream .= "xml:lang='$args{xmllang}' " if exists($args{xmllang});

    foreach my $ns (@{$args{namespaces}})
    {
        $stream .= " ".$ns->GetStream();
    }
    
    $stream .= ">";

    return $stream;
}


###########################################################################
#
# debug - prints the arguments to the debug log if debug is turned on.
#
###########################################################################
sub debug
{
    return if ($_[1] > $_[0]->{DEBUGLEVEL});
    my $self = shift;
    my ($limit,@args) = @_;
    return if ($self->{DEBUGFILE} eq "");
    my $fh = $self->{DEBUGFILE};
    if ($self->{DEBUGTIME} == 1)
    {
        my ($sec,$min,$hour) = localtime(time);
        print $fh sprintf("[%02d:%02d:%02d] ",$hour,$min,$sec);
    }
    print $fh "XML::Stream: @args\n";
}


##############################################################################
#
# nonblock - set the socket to be non-blocking.
#
##############################################################################
sub nonblock
{
    my $self = shift;
    my $socket = shift;

    #--------------------------------------------------------------------------
    # Code copied from POE::Wheel::SocketFactory...
    # Win32 does things one way...
    #--------------------------------------------------------------------------
    if ($^O eq "MSWin32")
    {
        ioctl( $socket, 0x80000000 | (4 << 16) | (ord('f') << 8) | 126, 1) ||
            croak("Can't make socket nonblocking (win32): $!");
        return;
    }

    #--------------------------------------------------------------------------
    # And UNIX does them another
    #--------------------------------------------------------------------------
    my $flags = fcntl($socket, F_GETFL, 0)
        or die "Can't get flags for socket: $!\n";
    fcntl($socket, F_SETFL, $flags | O_NONBLOCK)
        or die "Can't make socket nonblocking: $!\n";
}


##############################################################################
#
# printData - debugging function to print out any data structure in an
#             organized manner.  Very useful for debugging XML::Parser::Tree
#             objects.  This is a private function that will only exist in
#             in the development version.
#
##############################################################################
sub printData
{
    print &sprintData(@_);
}


##############################################################################
#
# sprintData - debugging function to build a string out of any data structure
#              in an organized manner.  Very useful for debugging
#              XML::Parser::Tree objects and perl hashes of hashes.
#
#              This is a private function.
#
##############################################################################
sub sprintData
{
    my ($preString,$data) = @_;

    my $outString = "";

    if (ref($data) eq "HASH")
    {
        my $key;
        foreach $key (sort { $a cmp $b } keys(%{$data}))
        {
            if (ref($$data{$key}) eq "")
            {
                my $value = defined($$data{$key}) ? $$data{$key} : "";
                $outString .= $preString."{'$key'} = \"".$value."\";\n";
            }
            else
            {
                if (ref($$data{$key}) =~ /Net::Jabber/)
                {
                    $outString .= $preString."{'$key'} = ".ref($$data{$key}).";\n";
                }
                else
                {
                    $outString .= $preString."{'$key'};\n";
                    $outString .= &sprintData($preString."{'$key'}->",$$data{$key});
                }
            }
        }
    }
    else
    {
        if (ref($data) eq "ARRAY")
        {
            my $index;
            foreach $index (0..$#{$data})
            {
                if (ref($$data[$index]) eq "")
                {
                    $outString .= $preString."[$index] = \"$$data[$index]\";\n";
                }
                else
                {
                    if (ref($$data[$index]) =~ /Net::Jabber/)
                    {
                        $outString .= $preString."[$index] = ".ref($$data[$index]).";\n";
                    }
                    else
                    {
                        $outString .= $preString."[$index];\n";
                        $outString .= &sprintData($preString."[$index]->",$$data[$index]);
                    }
                }
            }
        }
        else
        {
            if (ref($data) eq "REF")
            {
                $outString .= &sprintData($preString."->",$$data);
            }
            else
            {
                if (ref($data) eq "")
                {
                    $outString .= $preString." = \"$data\";\n";
                }
                else
                {
                     $outString .= $preString." = ".ref($data).";\n";
                }
            }
        }
    }

    return $outString;
}


1;
