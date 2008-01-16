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

package Net::XMPP::Message;

=head1 NAME

Net::XMPP::Message - XMPP Message Module

=head1 SYNOPSIS

  Net::XMPP::Message is a companion to the Net::XMPP module.
  It provides the user a simple interface to set and retrieve all
  parts of an XMPP Message.

=head1 DESCRIPTION

  A Net::XMPP::Message object is passed to the callback function for
  the message.  Also, the first argument to the callback functions is
  the session ID from XML::Stream.  There are some cases where you
  might want thisinformation, like if you created a Client that
  connects to two servers at once, or for writing a mini server.

    use Net::XMPP;

    sub message {
      my ($sid,$Mess) = @_;
      .
      .
      .
    }

  You now have access to all of the retrieval functions available.

  To create a new message to send to the server:

    use Net::XMPP;

    $Mess = Net::XMPP::Message->new();

  Now you can call the creation functions below to populate the tag
  before sending it.

=head1 METHODS

=head2 Retrieval functions

  GetTo()      - returns the value in the to='' attribute for the
  GetTo("jid")   <message/>.  If you specify "jid" as an argument
                 then a Net::XMPP::JID object is returned and
                 you can easily parse the parts of the JID.

                 $to    = $Mess->GetTo();
                 $toJID = $Mess->GetTo("jid");

  GetFrom()      - returns the value in the from='' attribute for the
  GetFrom("jid")   <message/>.  If you specify "jid" as an argument
                   then a Net::XMPP::JID object is returned and
                   you can easily parse the parts of the JID.

                   $from    = $Mess->GetFrom();
                   $fromJID = $Mess->GetFrom("jid");

  GetType() - returns the type='' attribute of the <message/>.  Each
              message is one of four types:

                normal        regular message (default if type is blank)
                chat          one on one chat
                groupchat     multi-person chat
                headline      headline
                error         error message

              $type = $Mess->GetType();

  GetSubject() - returns the data in the <subject/> tag.

                 $subject = $Mess->GetSubject();

  GetBody() - returns the data in the <body/> tag.

              $body = $Mess->GetBody();

  GetThread() - returns the data in the <thread/> tag.

                $thread = $Mess->GetThread();

  GetError() - returns a string with the data of the <error/> tag.

               $error = $Mess->GetError();

  GetErrorCode() - returns a string with the code='' attribute of the
                   <error/> tag.

                   $errCode = $Mess->GetErrorCode();

  GetTimeStamp() - returns a string that represents the time this
                   message object was created (and probably received)
                   for sending to the client.  If there is a
                   jabber:x:delay tag then that time is used to show
                   when the message was sent.

                   $date = $Mess->GetTimeStamp();


=head2 Creation functions

  SetMessage(to=>string|JID,    - set multiple fields in the <message/>
             from=>string|JID,    at one time.  This is a cumulative
             type=>string,        and over writing action.  If you set
             subject=>string,     the "to" attribute twice, the second
             body=>string,        setting is what is used.  If you set
             thread=>string,      the subject, and then set the body
             errorcode=>string,   then both will be in the <message/>
             error=>string)       tag.  For valid settings read the
                                  specific Set functions below.

                            $Mess->SetMessage(TO=>"bob\@jabber.org",
                                              Subject=>"Lunch",
                                              Body=>"Let's do lunch!");
                            $Mess->SetMessage(to=>"bob\@jabber.org",
                                              from=>"jabber.org",
                                              errorcode=>404,
                                              error=>"Not found");

  SetTo(string) - sets the to='' attribute.  You can either pass
  SetTo(JID)      a string or a JID object.  They must be valid JIDs
                  or the server will return an error message.
                  (ie.  bob@jabber.org/Work)

                  $Mess->SetTo("test\@jabber.org");

  SetFrom(string) - sets the from='' attribute.  You can either pass
  SetFrom(JID)      a string or a JID object.  They must be valid JIDs
                    or the server will return an error message. (ie.
                    jabber:bob@jabber.org/Work) This field is not
                    required if you are writing a Client since the
                    server will put the JID of your connection in
                    there to prevent spamming.

                    $Mess->SetFrom("me\@jabber.org");

  SetType(string) - sets the type attribute.  Valid settings are:

                      normal         regular message (default if blank)
                      chat           one one one chat style message
                      groupchat      multi-person chatroom message
                      headline       news headline, stock ticker, etc...
                      error          error message

                    $Mess->SetType("groupchat");

  SetSubject(string) - sets the subject of the <message/>.

                       $Mess->SetSubject("This is a test");

  SetBody(string) - sets the body of the <message/>.

                    $Mess->SetBody("To be or not to be...");

  SetThread(string) - sets the thread of the <message/>.  You should
                      copy this out of the message being replied to so
                      that the thread is maintained.

                      $Mess->SetThread("AE912B3");

  SetErrorCode(string) - sets the error code of the <message/>.

                         $Mess->SetErrorCode(403);

  SetError(string) - sets the error string of the <message/>.

                     $Mess->SetError("Permission Denied");

  Reply(hash) - creates a new Message object and populates the
                to/from, and the subject by putting "re: " in
                front.  If you specify a hash the same as with
                SetMessage then those values will override the
                Reply values.

                $Reply = $Mess->Reply();
                $Reply = $Mess->Reply(type=>"chat");

=head2 Removal functions

  RemoveTo() - removes the to attribute from the <message/>.

               $Mess->RemoveTo();
  
  RemoveFrom() - removes the from attribute from the <message/>.

                 $Mess->RemoveFrom();
  
  RemoveType() - removes the type attribute from the <message/>.

                 $Mess->RemoveType();
  
  RemoveSubject() - removes the <subject/> element from the
                    <message/>.

                    $Mess->RemoveSubject();

  RemoveBody() - removes the <body/> element from the
                 <message/>.
                  
                 $Mess->RemoveBody();

  RemoveThread() - removes the <thread/> element from the <message/>.

                   $Mess->RemoveThread();
  
  RemoveError() - removes the <error/> element from the <message/>.

                  $Mess->RemoveError();
  
  RemoveErrorCode() - removes the code attribute from the <error/>
                      element in the <message/>.

                      $Mess->RemoveErrorCode();

=head2 Test functions

  DefinedTo() - returns 1 if the to attribute is defined in the
                <message/>, 0 otherwise.

                $test = $Mess->DefinedTo();

  DefinedFrom() - returns 1 if the from attribute is defined in the
                  <message/>, 0 otherwise.

                  $test = $Mess->DefinedFrom();

  DefinedType() - returns 1 if the type attribute is defined in the
                  <message/>, 0 otherwise.

                  $test = $Mess->DefinedType();

  DefinedSubject() - returns 1 if <subject/> is defined in the
                     <message/>, 0 otherwise.

                     $test = $Mess->DefinedSubject();

  DefinedBody() - returns 1 if <body/> is defined in the <message/>,
                  0 otherwise.

                  $test = $Mess->DefinedBody();

  DefinedThread() - returns 1 if <thread/> is defined in the <message/>,
                    0 otherwise.

                    $test = $Mess->DefinedThread();

  DefinedErrorCode() - returns 1 if <error/> is defined in the
                       <message/>, 0 otherwise.

                       $test = $Mess->DefinedErrorCode();

  DefinedError() - returns 1 if the code attribute is defined in the
                   <error/>, 0 otherwise.

                   $test = $Mess->DefinedError();

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software, you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.003;
use strict;
use Carp;
use vars qw( %FUNCTIONS );
use Net::XMPP::Stanza;
use base qw( Net::XMPP::Stanza );

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = {};

    bless($self, $proto);

    $self->{DEBUGHEADER} = "Message";
    $self->{TAG} = "message";
    $self->{TIMESTAMP} = &Net::XMPP::GetTimeStamp("local");

    $self->{FUNCS} = \%FUNCTIONS;

    $self->_init(@_);

    return $self;
}

sub _message { my $self = shift; return Net::XMPP::Message->new(); }


$FUNCTIONS{Body}->{path}  = 'body/text()';

$FUNCTIONS{Error}->{path} = 'error/text()';

$FUNCTIONS{ErrorCode}->{path} = 'error/@code';

$FUNCTIONS{From}->{type} = 'jid';
$FUNCTIONS{From}->{path} = '@from';

$FUNCTIONS{ID}->{path} = '@id';

$FUNCTIONS{Subject}->{path} = 'subject/text()';

$FUNCTIONS{Thread}->{path} = 'thread/text()';

$FUNCTIONS{To}->{type} = 'jid';
$FUNCTIONS{To}->{path} = '@to';

$FUNCTIONS{Type}->{path} = '@type';

$FUNCTIONS{XMLNS}->{path} = '@xmlns';

$FUNCTIONS{Message}->{type}  = 'master';

$FUNCTIONS{Child}->{type} = 'child';
$FUNCTIONS{Child}->{path} = '*[@xmlns]';
$FUNCTIONS{Child}->{child} = {};

##############################################################################
#
# GetTimeStamp - returns a string with the time stamp of when this object
#                was created.
#
##############################################################################
sub GetTimeStamp
{
    my $self = shift;

    if ($self->DefinedX("jabber:x:delay"))
    {
        my @xTags = $self->GetX("jabber:x:delay");
        my $xTag = $xTags[0];
        $self->{TIMESTAMP} = &Net::XMPP::GetTimeStamp("utcdelaylocal",$xTag->GetStamp());
    }

    return $self->{TIMESTAMP};
}


##############################################################################
#
# Reply - returns a Net::XMPP::Message object with the proper fields
#         already populated for you.
#
##############################################################################
sub Reply
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $reply = $self->_message();

    if (($self->GetType() eq "") || ($self->GetType() eq "normal"))
    {
        my $subject = $self->GetSubject();
        $subject =~ s/re\:\s+//i;
        $reply->SetSubject("re: $subject");
    }
    $reply->SetThread($self->GetThread()) if ($self->GetThread() ne "");
    $reply->SetID($self->GetID()) if ($self->GetID() ne "");
    $reply->SetType($self->GetType()) if ($self->GetType() ne "");
    $reply->SetMessage((($self->GetFrom() ne "") ?
                         (to=>$self->GetFrom()) :
                         ()
                        ),
                        (($self->GetTo() ne "") ?
                         (from=>$self->GetTo()) :
                         ()
                        ),
                       );
    $reply->SetMessage(%args);

    return $reply;
}


1;
