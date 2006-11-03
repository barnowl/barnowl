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

package Net::Jabber::XDB;

=head1 NAME

Net::Jabber::XDB - Jabber XDB Library

=head1 SYNOPSIS

  Net::Jabber::XDB is a companion to the Net::Jabber module. It
  provides the user a simple interface to set and retrieve all
  parts of a Jabber XDB.

=head1 DESCRIPTION

  Net::Jabber::XDB differs from the other Net::Jabber::* modules in that
  the XMLNS of the data is split out into more submodules under
  XDB.  For specifics on each module please view the documentation
  for each Net::Jabber::Data::* module.  To see the list of avilable
  namspaces and modules see Net::Jabber::Data.

  To initialize the XDB with a Jabber <xdb/> you must pass it the 
  XML::Parser Tree array.  For example:

    my $xdb = new Net::Jabber::XDB(@tree);

  There has been a change from the old way of handling the callbacks.
  You no longer have to do the above, a Net::Jabber::XDB object is passed
  to the callback function for the xdb:

    use Net::Jabber qw(Component);

    sub xdb {
      my ($XDB) = @_;
      .
      .
      .
    }

  You now have access to all of the retrieval functions available.

  To create a new xdb to send to the server:

    use Net::Jabber;

    $XDB = new Net::Jabber::XDB();
    $XDBType = $XDB->NewData( type );
    $XDBType->SetXXXXX("yyyyy");

  Now you can call the creation functions for the XDB, and for the <data/>
  on the new Data object itself.  See below for the <xdb/> functions, and
  in each data module for those functions.

  For more information about the array format being passed to the CallBack
  please read the Net::Jabber::Client documentation.

=head1 METHODS

=head2 Retrieval functions

  GetTo()      - returns either a string with the Jabber Identifier,
  GetTo("jid")   or a Net::Jabber::JID object for the person who is
                 going to receive the <xdb/>.  To get the JID
                 object set the string to "jid", otherwise leave
                 blank for the text string.

                 $to    = $XDB->GetTo();
                 $toJID = $XDB->GetTo("jid");

  GetFrom()      -  returns either a string with the Jabber Identifier,
  GetFrom("jid")    or a Net::Jabber::JID object for the person who
                    sent the <xdb/>.  To get the JID object set
                    the string to "jid", otherwise leave blank for the
                    text string.

                    $from    = $XDB->GetFrom();
                    $fromJID = $XDB->GetFrom("jid");

  GetType() - returns a string with the type <xdb/> this is.

              $type = $XDB->GetType();

  GetID() - returns an integer with the id of the <xdb/>.

            $id = $XDB->GetID();

  GetAction() - returns a string with the action <xdb/> this is.

              $action = $XDB->GetAction();

  GetMatch() - returns a string with the match <xdb/> this is.

              $match = $XDB->GetMatch();

  GetError() - returns a string with the text description of the error.

               $error = $XDB->GetError();

  GetErrorCode() - returns a string with the code of error.

                   $errorCode = $XDB->GetErrorCode();

  GetData() - returns a Net::Jabber::Data object that contains the data
              in the <data/> of the <xdb/>.

              $dataTag = $XDB->GetData();

  GetDataXMLNS() - returns a string with the namespace of the data
                   for this <xdb/>, if one exists.

                   $xmlns = $XDB->GetDataXMLNS();

=head2 Creation functions

  SetXDB(to=>string|JID,    - set multiple fields in the <xdb/> at one
        from=>string|JID,     time.  This is a cumulative and over
        id=>string,           writing action.  If you set the "to"
        type=>string,         attribute twice, the second setting is
        action=>string,       what is used.  If you set the status, and
        match=>string)        then set the priority then both will be in
        errorcode=>string,    the <xdb/> tag.  For valid settings read the
        error=>string)        specific Set functions below.

                              $XDB->SetXDB(type=>"get",
 		   			   to=>"bob\@jabber.org",
	  				   data=>"info");

                              $XDB->SetXDB(to=>"bob\@jabber.org",
			 		   errorcode=>403,
					   error=>"Permission Denied");

  SetTo(string) - sets the to attribute.  You can either pass a string
  SetTo(JID)      or a JID object.  They must be a valid Jabber
                  Identifiers or the server will return an error message.
                  (ie.  jabber:bob@jabber.org, etc...)

                 $XDB->SetTo("bob\@jabber.org");

  SetFrom(string) - sets the from attribute.  You can either pass a string
  SetFrom(JID)      or a JID object.  They must be a valid Jabber
                    Identifiers or the server will return an error message.
                    (ie.  jabber:bob@jabber.org, etc...)

                    $XDB->SetFrom("me\@jabber.org");

  SetType(string) - sets the type attribute.  Valid settings are:

                    get      request information
                    set      set information
                    result   results of a get
                    error    there was an error

                    $XDB->SetType("set");

  SetAction(string) - sets the error code of the <xdb/>.

                      $XDB->SetAction("foo");

  SetMatch(string) - sets the error code of the <xdb/>.

                     $XDB->SetMatch("foo");

  SetErrorCode(string) - sets the error code of the <xdb/>.

                         $XDB->SetErrorCode(403);

  SetError(string) - sets the error string of the <xdb/>.

                     $XDB->SetError("Permission Denied");

  NewData(string) - creates a new Net::Jabber::Data object with the
                     namespace in the string.  In order for this function
                     to work with a custom namespace, you must define and
                     register that namespace with the XDB module.  For more
                     information please read the documentation for
                     Net::Jabber::Data.

                     $dataObj = $XDB->NewData("jabber:xdb:auth");
                     $dataObj = $XDB->NewData("jabber:xdb:roster");

  Reply(hash) - creates a new XDB object and populates the to/from
                fields.  If you specify a hash the same as with SetXDB
                then those values will override the Reply values.

                $xdbReply = $XDB->Reply();
                $xdbReply = $XDB->Reply(type=>"result");

=head2 Test functions

  DefinedTo() - returns 1 if the to attribute is defined in the <xdb/>,
                0 otherwise.

                $test = $XDB->DefinedTo();

  DefinedFrom() - returns 1 if the from attribute is defined in the <xdb/>,
                  0 otherwise.

                  $test = $XDB->DefinedFrom();

  DefinedID() - returns 1 if the id attribute is defined in the <xdb/>,
                0 otherwise.

                $test = $XDB->DefinedID();

  DefinedType() - returns 1 if the type attribute is defined in the <xdb/>,
                  0 otherwise.

                  $test = $XDB->DefinedType();

  DefinedAction() - returns 1 if the action attribute is defined in the <xdb/>,
                   0 otherwise.

                   $test = $XDB->DefinedAction();

  DefinedMatch() - returns 1 if the match attribute is defined in the <xdb/>,
                   0 otherwise.

                   $test = $XDB->DefinedMatch();

  DefinedError() - returns 1 if <error/> is defined in the <xdb/>,
                   0 otherwise.

                   $test = $XDB->DefinedError();

  DefinedErrorCode() - returns 1 if the code attribute is defined in
                       <error/>, 0 otherwise.

                       $test = $XDB->DefinedErrorCode();

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

    $self->{DEBUGHEADER} = "XDB";

    $self->{DATA} = {};
    $self->{CHILDREN} = {};

    $self->{TAG} = "xdb";

    if ("@_" ne (""))
    {
        if (ref($_[0]) eq "Net::Jabber::XDB")
        {
            return $_[0];
        }
        else
        {
            $self->{TREE} = shift;
            $self->ParseTree();
        }
    }
    else
    {
        $self->{TREE} = new XML::Stream::Node($self->{TAG});
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

$FUNCTIONS{Action}->{Get}           = "action";
$FUNCTIONS{Action}->{Set}           = ["scalar","action"];
$FUNCTIONS{Action}->{Defined}       = "action";
$FUNCTIONS{Action}->{Hash}          = "att";
$FUNCTIONS{Action}->{XPath}->{Type} = 'scalar';
$FUNCTIONS{Action}->{XPath}->{Path} = '@action';

$FUNCTIONS{Error}->{Get}           = "error";
$FUNCTIONS{Error}->{Set}           = ["scalar","error"];
$FUNCTIONS{Error}->{Defined}       = "error";
$FUNCTIONS{Error}->{Hash}          = "child-data";
$FUNCTIONS{Error}->{XPath}->{Type} = 'scalar';
$FUNCTIONS{Error}->{XPath}->{Path} = 'error/text()';

$FUNCTIONS{ErrorCode}->{Get}           = "errorcode";
$FUNCTIONS{ErrorCode}->{Set}           = ["scalar","errorcode"];
$FUNCTIONS{ErrorCode}->{Defined}       = "errorcode";
$FUNCTIONS{ErrorCode}->{Hash}          = "att-error-code";
$FUNCTIONS{ErrorCode}->{XPath}->{Type} = 'scalar';
$FUNCTIONS{ErrorCode}->{XPath}->{Path} = 'error/@code';

$FUNCTIONS{From}->{Get}           = "from";
$FUNCTIONS{From}->{Set}           = ["jid","from"];
$FUNCTIONS{From}->{Defined}       = "from";
$FUNCTIONS{From}->{Hash}          = "att";
$FUNCTIONS{From}->{XPath}->{Type} = 'jid';
$FUNCTIONS{From}->{XPath}->{Path} = '@from';

$FUNCTIONS{Match}->{Get}           = "match";
$FUNCTIONS{Match}->{Set}           = ["scalar","match"];
$FUNCTIONS{Match}->{Defined}       = "match";
$FUNCTIONS{Match}->{Hash}          = "att";
$FUNCTIONS{Match}->{XPath}->{Type} = 'scalar';
$FUNCTIONS{Match}->{XPath}->{Path} = '@match';

$FUNCTIONS{NS}->{Get}           = "ns";
$FUNCTIONS{NS}->{Set}           = ["scalar","ns"];
$FUNCTIONS{NS}->{Defined}       = "ns";
$FUNCTIONS{NS}->{Hash}          = "att";
$FUNCTIONS{NS}->{XPath}->{Type} = 'scalar';
$FUNCTIONS{NS}->{XPath}->{Path} = '@ns';

$FUNCTIONS{ID}->{Get}           = "id";
$FUNCTIONS{ID}->{Set}           = ["scalar","id"];
$FUNCTIONS{ID}->{Defined}       = "id";
$FUNCTIONS{ID}->{Hash}          = "att";
$FUNCTIONS{ID}->{XPath}->{Type} = 'scalar';
$FUNCTIONS{ID}->{XPath}->{Path} = '@id';

$FUNCTIONS{To}->{Get}           = "to";
$FUNCTIONS{To}->{Set}           = ["jid","to"];
$FUNCTIONS{To}->{Defined}       = "to";
$FUNCTIONS{To}->{Hash}          = "att";
$FUNCTIONS{To}->{XPath}->{Type} = 'jid';
$FUNCTIONS{To}->{XPath}->{Path} = '@to';

$FUNCTIONS{Type}->{Get}           = "type";
$FUNCTIONS{Type}->{Set}           = ["scalar","type"];
$FUNCTIONS{Type}->{Defined}       = "type";
$FUNCTIONS{Type}->{Hash}          = "att";
$FUNCTIONS{Type}->{XPath}->{Type} = 'scalar';
$FUNCTIONS{Type}->{XPath}->{Path} = '@type';

$FUNCTIONS{Data}->{Get}           = "__netjabber__:children:data";
$FUNCTIONS{Data}->{Defined}       = "__netjabber__:children:data";
$FUNCTIONS{Data}->{XPath}->{Type} = 'node';
$FUNCTIONS{Data}->{XPath}->{Path} = '*[@xmlns]';

$FUNCTIONS{X}->{Get}           = "__netjabber__:children:x";
$FUNCTIONS{X}->{Defined}       = "__netjabber__:children:x";
$FUNCTIONS{X}->{XPath}->{Type} = 'node';
$FUNCTIONS{X}->{XPath}->{Path} = '*[@xmlns]';

$FUNCTIONS{XDB}->{Get} = "__netjabber__:master";
$FUNCTIONS{XDB}->{Set} = ["master"];


##############################################################################
#
# GetDataXMLNS - returns the xmlns of the <data/> tag
#
##############################################################################
sub GetDataXMLNS
{
    my $self = shift;
    #XXX fix this
    return $self->{CHILDREN}->{data}->[0]->GetXMLNS() if exists($self->{CHILDREN}->{data});
}


##############################################################################
#
# Reply - returns a Net::Jabber::XDB object with the proper fields
#         already populated for you.
#
##############################################################################
sub Reply
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $reply = new Net::Jabber::XDB();

    $reply->SetID($self->GetID()) if ($self->GetID() ne "");
    $reply->SetType("result");

    if ($self->DefinedData())
    {
        my $selfData = $self->GetData();
        $reply->NewData($selfData->GetXMLNS());
    }

    $reply->SetXDB(to=>$self->GetFrom(),
                   from=>$self->GetTo()
                  );

    $reply->SetXDB(%args);

    return $reply;
}


1;
