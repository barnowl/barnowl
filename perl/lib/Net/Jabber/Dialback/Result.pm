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

package Net::Jabber::Dialback::Result;

=head1 NAME

Net::Jabber::Dialback::Result - Jabber Dialback Result Module

=head1 SYNOPSIS

  Net::Jabber::Dialback::Result is a companion to the Net::Jabber::Dialback
  module.  It provides the user a simple interface to set and retrieve all
  parts of a Jabber Dialback Result.

=head1 DESCRIPTION

  To initialize the Result with a Jabber <db:*/> you must pass it
  the XML::Stream hash.  For example:

    my $dialback = new Net::Jabber::Dialback::Result(%hash);

  There has been a change from the old way of handling the callbacks.
  You no longer have to do the above yourself, a NJ::Dialback::Result
  object is passed to the callback function for the message.  Also,
  the first argument to the callback functions is the session ID from
  XML::Streams.  There are some cases where you might want this
  information, like if you created a Client that connects to two servers
  at once, or for writing a mini server.

    use Net::Jabber qw(Server);

    sub dialbackResult {
      my ($sid,$Result) = @_;
      .
      .
      .
    }

  You now have access to all of the retrieval functions available.

  To create a new dialback to send to the server:

    use Net::Jabber qw(Server);

    $Result = new Net::Jabber::Dialback::Result();

  Now you can call the creation functions below to populate the tag before
  sending it.

  For more information about the array format being passed to the CallBack
  please read the Net::Jabber::Client documentation.

=head2 Retrieval functions

    $to         = $Result->GetTo();
    $from       = $Result->GetFrom();
    $type       = $Result->GetType();

    $data       = $Result->GetData();

    $str        = $Result->GetXML();
    @dialback   = $Result->GetTree();

=head2 Creation functions

    $Result->SetResult(from=>"jabber.org",
		       to=>"jabber.com",
		       data=>key);
    $Result->SetTo("jabber.org");
    $Result->SetFrom("jabber.com");
    $Result->SetType("valid");
    $Result->SetData(key);

=head2 Test functions

    $test = $Result->DefinedTo();
    $test = $Result->DefinedFrom();
    $test = $Result->DefinedType();

=head1 METHODS

=head2 Retrieval functions

  GetTo() -  returns a string with server that the <db:result/> is being
             sent to.

  GetFrom() -  returns a string with server that the <db:result/> is being
               sent from.

  GetType() - returns a string with the type <db:result/> this is.

  GetData() - returns a string with the cdata of the <db:result/>.

  GetXML() - returns the XML string that represents the <db:result/>.
             This is used by the Send() function in Server.pm to send
             this object as a Jabber Dialback Result.

  GetTree() - returns an array that contains the <db:result/> tag
              in XML::Parser::Tree format.

=head2 Creation functions

  SetResult(to=>string,   - set multiple fields in the <db:result/>
            from=>string,   at one time.  This is a cumulative
            type=>string,   and over writing action.  If you set
            data=>string)   the "from" attribute twice, the second
                            setting is what is used.  If you set
                            the type, and then set the data
                            then both will be in the <db:result/>
                            tag.  For valid settings read the
                            specific Set functions below.

  SetTo(string) - sets the to attribute.

  SetFrom(string) - sets the from attribute.

  SetType(string) - sets the type attribute.  Valid settings are:

                    valid
                    invalid

  SetData(string) - sets the cdata of the <db:result/>.

=head2 Test functions

  DefinedTo() - returns 1 if the to attribute is defined in the 
                <db:result/>, 0 otherwise.

  DefinedFrom() - returns 1 if the from attribute is defined in the 
                  <db:result/>, 0 otherwise.

  DefinedType() - returns 1 if the type attribute is defined in the 
                  <db:result/>, 0 otherwise.

=head1 AUTHOR

By Ryan Eatmon in May of 2001 for http://jabber.org..

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.003;
use strict;
use Carp;
use vars qw($VERSION $AUTOLOAD %FUNCTIONS);

$VERSION = "2.0";

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = { };

    $self->{VERSION} = $VERSION;

    bless($self, $proto);

    $self->{DEBUGHEADER} = "DB:Result";

    $self->{DATA} = {};
    $self->{CHILDREN} = {};

    $self->{TAG} = "db:result";

    if ("@_" ne (""))
    {
        if (ref($_[0]) eq "Net::Jabber::Dialback::Result")
        {
            return $_[0];
        }
        else
        {
            $self->{TREE} = shift;
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

$FUNCTIONS{From}->{Get}        = "from";
$FUNCTIONS{From}->{Set}        = ["jid","from"];
$FUNCTIONS{From}->{Defined}    = "from";
$FUNCTIONS{From}->{Hash}       = "att";

$FUNCTIONS{Data}->{Get}        = "data";
$FUNCTIONS{Data}->{Set}        = ["scalar","data"];
$FUNCTIONS{Data}->{Defined}    = "data";
$FUNCTIONS{Data}->{Hash}       = "data";

$FUNCTIONS{To}->{Get}        = "to";
$FUNCTIONS{To}->{Set}        = ["jid","to"];
$FUNCTIONS{To}->{Defined}    = "to";
$FUNCTIONS{To}->{Hash}       = "att";

$FUNCTIONS{Type}->{Get}        = "type";
$FUNCTIONS{Type}->{Set}        = ["scalar","type"];
$FUNCTIONS{Type}->{Defined}    = "type";
$FUNCTIONS{Type}->{Hash}       = "att";

$FUNCTIONS{Result}->{Get} = "__netjabber__:master";
$FUNCTIONS{Result}->{Set} = ["master"];

1;

