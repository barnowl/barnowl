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
#  Copyright (C) 1998-1999 The Jabber Team http://jabber.org/
#
##############################################################################

package Net::Jabber::Data;

=head1 NAME

Net::Jabber::Data - Jabber Data Library

=head1 SYNOPSIS

  Net::Jabber::Data is a companion to the Net::Jabber::XDB module. It
  provides the user a simple interface to set and retrieve all
  parts of a Jabber XDB Data.

=head1 DESCRIPTION

  Net::Jabber::Data differs from the other modules in that its behavior
  and available functions are based off of the XML namespace that is
  set in it.  The current list of supported namespaces is:

    jabber:iq:auth
    jabber:iq:auth:0k
    jabber:iq:register
    jabber:iq:roster

  For more information on what these namespaces are for, visit 
  http://www.jabber.org and browse the Jabber Programmers Guide.

  Each of these namespaces provide Net::Jabber::Data with the functions
  to access the data.  By using the AUTOLOAD function the functions for
  each namespace is used when that namespace is active.

  To access a Data object you must create an XDB object and use the
  access functions there to get to the Data.  To initialize the XDB with
  a Jabber <xdb/> you must pass it the XML::Stream hash from the
  Net::Jabber::Client module.

    my $xdb = new Net::Jabber::XDB(%hash);

  There has been a change from the old way of handling the callbacks.
  You no longer have to do the above yourself, a Net::Jabber::XDB
  object is passed to the callback function for the message.  Also,
  the first argument to the callback functions is the session ID from
  XML::Streams.  There are some cases where you might want this
  information, like if you created a Client that connects to two servers
  at once, or for writing a mini server.

    use Net::Jabber qw(Client);

    sub xdbCB {
      my ($sid,$XDB) = @_;
      my $data = $XDB->GetData();
      .
      .
      .
    }

  You now have access to all of the retrieval functions available for
  that namespace.

  To create a new xdb to send to the server:

    use Net::Jabber;

    my $xdb = new Net::Jabber::XDB();
    $data = $xdb->NewData("jabber:iq:auth");

  Now you can call the creation functions for the Data as defined in the
  proper namespaces.  See below for the general <data/> functions, and
  in each data module for those functions.

  For more information about the array format being passed to the
  CallBack please read the Net::Jabber::Client documentation.

=head1 METHODS

=head2 Retrieval functions

  GetXMLNS() - returns a string with the namespace of the data that
               the <xdb/> contains.

               $xmlns  = $XDB->GetXMLNS();

  GetData() - since the behavior of this module depends on the
               namespace, a Data object may contain Data objects.
               This helps to leverage code reuse by making children
               behave in the same manner.  More than likely this
               function will never be called.

               @data = GetData()

=head2 Creation functions

  SetXMLNS(string) - sets the xmlns of the <data/> to the string.

                     $data->SetXMLNS("jabber:xdb:roster");


In an effort to make maintaining this document easier, I am not going
to go into full detail on each of these functions.  Rather I will
present the functions in a list with a type in the first column to
show what they return, or take as arugments.  Here is the list of
types I will use:

  string  - just a string
  array   - array of strings
  flag    - this means that the specified child exists in the
            XML <child/> and acts like a flag.  get will return
            0 or 1.
  JID     - either a string or Net::Jabber::JID object.
  objects - creates new objects, or returns an array of
            objects.
  special - this is a special case kind of function.  Usually
            just by calling Set() with no arguments it will
            default the value to a special value, like OS or time.
            Sometimes it will modify the value you set, like
            in jabber:xdb:version SetVersion() the function
            adds on the Net::Jabber version to the string
            just for advertisement purposes. =)
  master  - this desribes a function that behaves like the
            SetMessage() function in Net::Jabber::Message.
            It takes a hash and sets all of the values defined,
            and the Set returns a hash with the values that
            are defined in the object.

=head1 jabber:iq:

  Type     Get               Set               Defined
  =======  ================  ================  ==================


=head1 jabber:iq:

  Type     Get               Set               Defined
  =======  ================  ================  ==================


=head1 jabber:iq:

  Type     Get               Set               Defined
  =======  ================  ================  ==================


=head1 jabber:iq:

  Type     Get               Set               Defined
  =======  ================  ================  ==================


=head1 jabber:iq:

  Type     Get               Set               Defined
  =======  ================  ================  ==================


=head1 CUSTOM NAMESPACES

  Part of the flexability of this module is that you can define your own
  namespace.  For more information on this topic, please read the
  Net::Jabber::Namespaces man page.

=head1 AUTHOR

By Ryan Eatmon in May of 2001 for http://jabber.org..

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.003;
use strict;
use Carp;
use vars qw($VERSION $AUTOLOAD %FUNCTIONS %NAMESPACES);

$VERSION = "2.0";

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = { };

    $self->{VERSION} = $VERSION;

    bless($self, $proto);

    $self->{DEBUGHEADER} = "Data";

    $self->{DATA} = {};
    $self->{CHILDREN} = {};

    $self->{TAG} = "data";

    if ("@_" ne (""))
    {
        if (ref($_[0]) eq "Net::Jabber::Data")
        {
            return $_[0];
        }
        else
        {
            $self->{TREE} = shift;
            $self->{TAG} = $self->{TREE}->get_tag();
            $self->ParseXMLNS();
            $self->ParseTree();
        }
    }

    return $self;
}


##############################################################################
#
# AUTOLOAD - This function calls the main AutoLoad function in Jabber.pm
#
##############################################################################
sub AUTOLOAD
{
    my $self = shift;
    &Net::Jabber::AutoLoad($self,$AUTOLOAD,@_);
}

$FUNCTIONS{XMLNS}->{Get}     = "xmlns";
$FUNCTIONS{XMLNS}->{Set}     = ["scalar","xmlns"];
$FUNCTIONS{XMLNS}->{Defined} = "xmlns";
$FUNCTIONS{XMLNS}->{Hash}    = "att";

$FUNCTIONS{Data}->{Get}     = "__netjabber__:children:data";
$FUNCTIONS{Data}->{Defined} = "__netjabber__:children:data";

#-----------------------------------------------------------------------------
# jabber:iq:auth
#-----------------------------------------------------------------------------
$NAMESPACES{"jabber:iq:auth"}->{Password}->{Get}     = "password";
$NAMESPACES{"jabber:iq:auth"}->{Password}->{Set}     = ["scalar","password"];
$NAMESPACES{"jabber:iq:auth"}->{Password}->{Defined} = "password";
$NAMESPACES{"jabber:iq:auth"}->{Password}->{Hash}    = "data";

$NAMESPACES{"jabber:iq:auth"}->{Auth}->{Get} = "__netjabber__:master";
$NAMESPACES{"jabber:iq:auth"}->{Auth}->{Set} = ["master"];

$NAMESPACES{"jabber:iq:auth"}->{"__netjabber__"}->{Tag} = "password";

#-----------------------------------------------------------------------------
# jabber:iq:auth:0k
#-----------------------------------------------------------------------------
$NAMESPACES{"jabber:iq:auth:0k"}->{Hash}->{Get}     = "hash";
$NAMESPACES{"jabber:iq:auth:0k"}->{Hash}->{Set}     = ["scalar","hash"];
$NAMESPACES{"jabber:iq:auth:0k"}->{Hash}->{Defined} = "hash";
$NAMESPACES{"jabber:iq:auth:0k"}->{Hash}->{Hash}    = "child-data";

$NAMESPACES{"jabber:iq:auth:0k"}->{Sequence}->{Get}     = "sequence";
$NAMESPACES{"jabber:iq:auth:0k"}->{Sequence}->{Set}     = ["scalar","sequence"];
$NAMESPACES{"jabber:iq:auth:0k"}->{Sequence}->{Defined} = "sequence";
$NAMESPACES{"jabber:iq:auth:0k"}->{Sequence}->{Hash}    = "child-data";

$NAMESPACES{"jabber:iq:auth:0k"}->{Token}->{Get}     = "token";
$NAMESPACES{"jabber:iq:auth:0k"}->{Token}->{Set}     = ["scalar","token"];
$NAMESPACES{"jabber:iq:auth:0k"}->{Token}->{Defined} = "token";
$NAMESPACES{"jabber:iq:auth:0k"}->{Token}->{Hash}    = "child-data";

$NAMESPACES{"jabber:iq:auth:0k"}->{ZeroK}->{Get} = "__netjabber__:master";
$NAMESPACES{"jabber:iq:auth:0k"}->{ZeroK}->{Set} = ["master"];

$NAMESPACES{"jabber:iq:auth:0k"}->{"__netjabber__"}->{Tag} = "zerok";

#-----------------------------------------------------------------------------
# jabber:iq:last
#-----------------------------------------------------------------------------
$NAMESPACES{"jabber:iq:last"}->{Message}->{Get}     = "message";
$NAMESPACES{"jabber:iq:last"}->{Message}->{Set}     = ["scalar","message"];
$NAMESPACES{"jabber:iq:last"}->{Message}->{Defined} = "message";
$NAMESPACES{"jabber:iq:last"}->{Message}->{Hash}    = "data";

$NAMESPACES{"jabber:iq:last"}->{Seconds}->{Get}     = "last";
$NAMESPACES{"jabber:iq:last"}->{Seconds}->{Set}     = ["scalar","last"];
$NAMESPACES{"jabber:iq:last"}->{Seconds}->{Defined} = "last";
$NAMESPACES{"jabber:iq:last"}->{Seconds}->{Hash}    = "att";

$NAMESPACES{"jabber:iq:last"}->{Last}->{Get} = "__netjabber__:master";
$NAMESPACES{"jabber:iq:last"}->{Last}->{Set} = ["master"];

$NAMESPACES{"jabber:iq:last"}->{"__netjabber__"}->{Tag} = "query";

#-----------------------------------------------------------------------------
# jabber:iq:roster
#-----------------------------------------------------------------------------
$NAMESPACES{"jabber:iq:roster"}->{Item}->{Get}     = "";
$NAMESPACES{"jabber:iq:roster"}->{Item}->{Set}     = ["add","Data","__netjabber__:iq:roster:item"];
$NAMESPACES{"jabber:iq:roster"}->{Item}->{Defined} = "__netjabber__:children:data";
$NAMESPACES{"jabber:iq:roster"}->{Item}->{Hash}    = "child-add";

$NAMESPACES{"jabber:iq:roster"}->{Item}->{Add} = ["Data","__netjabber__:iq:roster:item","Item","item"];

$NAMESPACES{"jabber:iq:roster"}->{Items}->{Get} = ["__netjabber__:children:data","__netjabber__:iq:roster:item"];

$NAMESPACES{"jabber:iq:roster"}->{"__netjabber__"}->{Tag} = "query";

#-----------------------------------------------------------------------------
# __netjabber__:iq:roster:item
#-----------------------------------------------------------------------------
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Ask}->{Get}     = "ask";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Ask}->{Set}     = ["scalar","ask"];
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Ask}->{Defined} = "ask";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Ask}->{Hash}    = "att";

$NAMESPACES{"__netjabber__:iq:roster:item"}->{Group}->{Get}     = "group";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Group}->{Set}     = ["array","group"];
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Group}->{Defined} = "group";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Group}->{Hash}    = "child-data";

$NAMESPACES{"__netjabber__:iq:roster:item"}->{JID}->{Get}     = "jid";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{JID}->{Set}     = ["jid","jid"];
$NAMESPACES{"__netjabber__:iq:roster:item"}->{JID}->{Defined} = "jid";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{JID}->{Hash}    = "att";

$NAMESPACES{"__netjabber__:iq:roster:item"}->{Name}->{Get}     = "name";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Name}->{Set}     = ["scalar","name"];
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Name}->{Defined} = "name";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Name}->{Hash}    = "att";

$NAMESPACES{"__netjabber__:iq:roster:item"}->{Subscription}->{Get}     = "subscription";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Subscription}->{Set}     = ["scalar","subscription"];
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Subscription}->{Defined} = "subscription";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Subscription}->{Hash}    = "att";

$NAMESPACES{"__netjabber__:iq:roster:item"}->{Item}->{Get} = "__netjabber__:master";
$NAMESPACES{"__netjabber__:iq:roster:item"}->{Item}->{Set} = ["master"];

#-----------------------------------------------------------------------------
# jabber:x:offline
#-----------------------------------------------------------------------------
$NAMESPACES{"jabber:x:offline"}->{Data}->{Get}     = "data";
$NAMESPACES{"jabber:x:offline"}->{Data}->{Set}     = ["scalar","data"];
$NAMESPACES{"jabber:x:offline"}->{Data}->{Defined} = "data";
$NAMESPACES{"jabber:x:offline"}->{Data}->{Hash}    = "data";

$NAMESPACES{"jabber:x:offline"}->{Offline}->{Get} = "__netjabber__:master";
$NAMESPACES{"jabber:x:offline"}->{Offline}->{Set} = ["master"];

$NAMESPACES{"jabber:x:offline"}->{"__netjabber__"}->{Tag} = "foo";

1;
