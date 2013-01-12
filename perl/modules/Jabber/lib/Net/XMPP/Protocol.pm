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

package Net::XMPP::Protocol;

=head1 NAME

Net::XMPP::Protocol - XMPP Protocol Module

=head1 SYNOPSIS

  Net::XMPP::Protocol is a module that provides a developer easy
  access to the XMPP Instant Messaging protocol.  It provides high
  level functions to the Net::XMPP Client object.  These functions are
  inherited by that modules.

=head1 DESCRIPTION

  Protocol.pm seeks to provide enough high level APIs and automation of
  the low level APIs that writing a XMPP Client in Perl is trivial.  For
  those that wish to work with the low level you can do that too, but
  those functions are covered in the documentation for each module.

  Net::XMPP::Protocol provides functions to login, send and receive
  messages, set personal information, create a new user account, manage
  the roster, and disconnect.  You can use all or none of the functions,
  there is no requirement.

  For more information on how the details for how Net::XMPP is written
  please see the help for Net::XMPP itself.

  For more information on writing a Client see Net::XMPP::Client.

=head2 Modes

  Several of the functions take a mode argument that let you specify how
  the function should behave:

    block - send the packet with an ID, and then block until an answer
            comes back.  You can optionally specify a timeout so that
            you do not block forever.

    nonblock - send the packet with an ID, but then return that id and
               control to the master program.  Net::XMPP is still
               tracking this packet, so you must use the CheckID function
               to tell when it comes in.  (This might not be very
               useful...)

    passthru - send the packet with an ID, but do NOT register it with
               Net::XMPP, then return the ID.  This is useful when
               combined with the XPath function because you can register
               a one shot function tied to the id you get back.


=head2 Basic Functions

    use Net::XMPP qw( Client );
    $Con = Net::XMPP::Client->new();                  # From
    $status = $Con->Connect(hostname=>"jabber.org"); # Net::XMPP::Client

    $Con->SetCallBacks(send=>\&sendCallBack,
                       receive=>\&receiveCallBack,
                       message=>\&messageCallBack,
                       iq=>\&handleTheIQTag);

    $Con->SetMessageCallBacks(normal=>\&messageNormalCB,
                              chat=>\&messageChatCB);

    $Con->SetPresenceCallBacks(available=>\&presenceAvailableCB,
                               unavailable=>\&presenceUnavailableCB);

    $Con->SetIQCallBacks("custom-namespace"=>
                                             {
                                                 get=>\&iqCustomGetCB,
                                                 set=>\&iqCustomSetCB,
                                                 result=>\&iqCustomResultCB,
                                             },
                                             etc...
                                            );

    $Con->SetXPathCallBacks("/message[@type='chat']"=>&messageChatCB,
                            "/message[@type='chat']"=>&otherMessageChatCB,
                            ...
                           );

    $Con->RemoveXPathCallBacks("/message[@type='chat']"=>&otherMessageChatCB);

    $error = $Con->GetErrorCode();
    $Con->SetErrorCode("Timeout limit reached");

    $status = $Con->Process();
    $status = $Con->Process(5);

    $Con->Send($object);
    $Con->Send("<tag>XML</tag>");

    $Con->Send($object,1);
    $Con->Send("<tag>XML</tag>",1);

    $Con->Disconnect();

=head2 ID Functions

    $id         = $Con->SendWithID($sendObj);
    $id         = $Con->SendWithID("<tag>XML</tag>");
    $receiveObj = $Con->SendAndReceiveWithID($sendObj);
    $receiveObj = $Con->SendAndReceiveWithID($sendObj,
                                             10);
    $receiveObj = $Con->SendAndReceiveWithID("<tag>XML</tag>");
    $receiveObj = $Con->SendAndReceiveWithID("<tag>XML</tag>",
                                             5);
    $yesno      = $Con->ReceivedID($id);
    $receiveObj = $Con->GetID($id);
    $receiveObj = $Con->WaitForID($id);
    $receiveObj = $Con->WaitForID($id,
                                  20);

=head2 Namespace Functions

    $Con->AddNamespace(ns=>"foo:bar",
                       tag=>"myfoo",
                       xpath=>{Foo=>{ path=> "foo/text()" },
                               Bar=>{ path=> "bar/text()" },
                               FooBar=>{ type=> "master" },
                              }
                      );

=head2 Message Functions

    $Con->MessageSend(to=>"bob@jabber.org",
                      subject=>"Lunch",
                      body=>"Let's go grab some...\n",
                      thread=>"ABC123",
                      priority=>10);

=head2 Presence Functions

    $Con->PresenceSend();
    $Con->PresenceSend(type=>"unavailable");
    $Con->PresenceSend(show=>"away");
    $Con->PresenceSend(signature=>...signature...);

=head2 Subscription Functions

    $Con->Subscription(type=>"subscribe",
                       to=>"bob@jabber.org");

    $Con->Subscription(type=>"unsubscribe",
                       to=>"bob@jabber.org");

    $Con->Subscription(type=>"subscribed",
                       to=>"bob@jabber.org");

    $Con->Subscription(type=>"unsubscribed",
                       to=>"bob@jabber.org");

=head2 Presence DB Functions

    $Con->PresenceDB();

    $Con->PresenceDBParse(Net::XMPP::Presence);

    $Con->PresenceDBDelete("bob\@jabber.org");
    $Con->PresenceDBDelete(Net::XMPP::JID);

    $Con->PresenceDBClear();

    $presence  = $Con->PresenceDBQuery("bob\@jabber.org");
    $presence  = $Con->PresenceDBQuery(Net::XMPP::JID);

    @resources = $Con->PresenceDBResources("bob\@jabber.org");
    @resources = $Con->PresenceDBResources(Net::XMPP::JID);

=head2 IQ  Functions

=head2 Auth Functions

    @result = $Con->AuthSend();
    @result = $Con->AuthSend(username=>"bob",
                             password=>"bobrulez",
                             resource=>"Bob");

=head2 Register Functions

    %hash   = $Con->RegisterRequest();
    %hash   = $Con->RegisterRequest(to=>"transport.jabber.org");
    %hash   = $Con->RegisterRequest(to=>"transport.jabber.org",
                                    timeout=>10);

    @result = $Con->RegisterSend(to=>"somewhere",
                                 username=>"newuser",
                                 resource=>"New User",
                                 password=>"imanewbie",
                                 email=>"newguy@new.com",
                                 key=>"some key");

=head2 Roster Functions

    $Roster = $Con->Roster();

    %roster = $Con->RosterParse($iq);
    %roster = $Con->RosterGet();
    $Con->RosterRequest();
    $Con->RosterAdd(jid=>"bob\@jabber.org",
                    name=>"Bob");
    $Con->RosterRemove(jid=>"bob@jabber.org");

=head2 Roster DB Functions

    $Con->RosterDB();

    $Con->RosterDBParse(Net::XMPP::IQ);

    $Con->RosterDBAdd("bob\@jabber.org",
                      name=>"Bob",
                      groups=>["foo"]
                     );

    $Con->RosterDBRemove("bob\@jabber.org");
    $Con->RosterDBRemove(Net::XMPP::JID);

    $Con->RosterDBClear();

    if ($Con->RosterDBExists("bob\@jabber.org")) { ...
    if ($Con->RosterDBExists(Net::XMPP::JID)) { ...

    @jids = $Con->RosterDBJIDs();

    if ($Con->RosterDBGroupExists("foo")) { ...

    @groups = $Con->RosterDBGroups();

    @jids = $Con->RosterDBGroupJIDs("foo");

    @jids = $Con->RosterDBNonGroupJIDs();

    %hash = $Con->RosterDBQuery("bob\@jabber.org");
    %hash = $Con->RosterDBQuery(Net::XMPP::JID);

    $value = $Con->RosterDBQuery("bob\@jabber.org","name");
    $value = $Con->RosterDBQuery(Net::XMPP::JID,"groups");


=head1 METHODS

=head2 Basic Functions

    GetErrorCode() - returns a string that will hopefully contain some
                     useful information about why a function returned
                     an undef to you.

    SetErrorCode(string) - set a useful error message before you return
                           an undef to the caller.

    SetCallBacks(message=>function,  - sets the callback functions for
                 presence=>function,   the top level tags listed.  The
                 iq=>function,         available tags to look for are
                 send=>function,       <message/>, <presence/>, and
                 receive=>function,    <iq/>.  If a packet is received
                 update=>function)     with an ID which is found in the
                                       registerd ID list (see RegisterID
                                       below) then it is not sent to
                                       these functions, instead it
                                       is inserted into a LIST and can
                                       be retrieved by some functions
                                       we will mention later.

                                       send and receive are used to
                                       log what XML is sent and received.
                                       update is used as way to update
                                       your program while waiting for
                                       a packet with an ID to be
                                       returned (useful for GUI apps).

                                       A major change that came with
                                       the last release is that the
                                       session id is passed to the
                                       callback as the first argument.
                                       This was done to facilitate
                                       the Server module.

                                       The next argument depends on
                                       which callback you are talking
                                       about.  message, presence, and iq
                                       all get passed in Net::XMPP
                                       objects that match those types.
                                       send and receive get passed in
                                       strings.  update gets passed
                                       nothing, not even the session id.

                                       If you set the function to undef,
                                       then the callback is removed from
                                       the list.

    SetPresenceCallBacks(type=>function - sets the callback functions for
                         etc...)          the specified presence type.
                                          The function takes types as the
                                          main key, and lets you specify
                                          a function for each type of
                                          packet you can get.
                                            "available"
                                            "unavailable"
                                            "subscribe"
                                            "unsubscribe"
                                            "subscribed"
                                            "unsubscribed"
                                            "probe"
                                            "error"
                                          When it gets a <presence/>
                                          packet it checks the type=''
                                          for a defined callback.  If
                                          there is one then it calls the
                                          function with two arguments:
                                            the session ID, and the
                                            Net::XMPP::Presence object.

                                          If you set the function to
                                          undef, then the callback is
                                          removed from the list.

                        NOTE: If you use this, which is a cleaner method,
                              then you must *NOT* specify a callback for
                              presence in the SetCallBacks function.

                                          Net::XMPP defines a few default
                                          callbacks for various types:

                                          "subscribe" -
                                            replies with subscribed

                                          "unsubscribe" -
                                            replies with unsubscribed

                                          "subscribed" -
                                            replies with subscribed

                                          "unsubscribed" -
                                            replies with unsubscribed


    SetMessageCallBacks(type=>function, - sets the callback functions for
                        etc...)           the specified message type. The
                                          function takes types as the
                                          main key, and lets you specify
                                          a function for each type of
                                          packet you can get.
                                           "normal"
                                           "chat"
                                           "groupchat"
                                           "headline"
                                           "error"
                                         When it gets a <message/> packet
                                         it checks the type='' for a
                                         defined callback. If there is
                                         one then it calls the function
                                         with two arguments:
                                           the session ID, and the
                                           Net::XMPP::Message object.

                                         If you set the function to
                                         undef, then the callback is
                                         removed from the list.

                       NOTE: If you use this, which is a cleaner method,
                             then you must *NOT* specify a callback for
                             message in the SetCallBacks function.


    SetIQCallBacks(namespace=>{      - sets the callback functions for
                     get=>function,    the specified namespace. The
                     set=>function,    function takes namespaces as the
                     result=>function  main key, and lets you specify a
                   },                  function for each type of packet
                   etc...)             you can get.
                                         "get"
                                         "set"
                                         "result"
                                       When it gets an <iq/> packet it
                                       checks the type='' and the
                                       xmlns='' for a defined callback.
                                       If there is one then it calls
                                       the function with two arguments:
                                       the session ID, and the
                                       Net::XMPP::xxxx object.

                                       If you set the function to undef,
                                       then the callback is removed from
                                       the list.

                       NOTE: If you use this, which is a cleaner method,
                             then you must *NOT* specify a callback for
                             iq in the SetCallBacks function.

    SetXPathCallBacks(xpath=>function, - registers a callback function
                        etc...)          for each xpath specified.  If
                                         Net::XMPP matches the xpath,
                                         then it calls the function with
                                         two arguments:
                                           the session ID, and the
                                           Net::XMPP::Message object.

                                         Xpaths are rooted at each
                                         packet:
                                           /message[@type="chat"]
                                           /iq/*[xmlns="jabber:iq:roster"][1]
                                           ...

    RemoveXPathCallBacks(xpath=>function, - unregisters a callback
                        etc...)             function for each xpath
                                            specified.

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

    Send(object,         - takes either a Net::XMPP::xxxxx object or
         ignoreActivity)   an XML string as an argument and sends it to
    Send(string,           the server.  If you set ignoreActivty to 1,
         ignoreActivity)   then the XML::Stream module will not record
                           this packet as couting towards user activity.
=head2 ID Functions

    SendWithID(object) - takes either a Net::XMPP::xxxxx object or an
    SendWithID(string)   XML string as an argument, adds the next
                         available ID number and sends that packet to
                         the server.  Returns the ID number assigned.

    SendAndReceiveWithID(object,  - uses SendWithID and WaitForID to
                         timeout)   provide a complete way to send and
    SendAndReceiveWithID(string,    receive packets with IDs.  Can take
                         timeout)   either a Net::XMPP::xxxxx object
                                    or an XML string.  Returns the
                                    proper Net::XMPP::xxxxx object
                                    based on the type of packet
                                    received.  The timeout is passed
                                    on to WaitForID, see that function
                                    for how the timeout works.

    ReceivedID(integer) - returns 1 if a packet has been received with
                          specified ID, 0 otherwise.

    GetID(integer) - returns the proper Net::XMPP::xxxxx object based
                     on the type of packet received with the specified
                     ID.  If the ID has been received the GetID returns
                     0.

    WaitForID(integer, - blocks until a packet with the ID is received.
              timeout)   Returns the proper Net::XMPP::xxxxx object
                         based on the type of packet received.  If the
                         timeout limit is reached then if the packet
                         does come in, it will be discarded.


    NOTE:  Only <iq/> officially support ids, so sending a <message/>, or
           <presence/> with an id is a risk.  The server will ignore the
           id tag and pass it through, so both clients must support the
           id tag for these functions to be useful.

=head2 Namespace Functions

    AddNamespace(ns=>string,  - This function is very complex.
                 tag=>string,   It is a little too complex to
                 xpath=>hash)   discuss within the confines of
                                this small paragraph.  Please
                                refer to the man page for
                                Net::XMPP::Namespaces for the
                                full documentation on this
                                subject.

=head2 Message Functions

    MessageSend(hash) - takes the hash and passes it to SetMessage in
                        Net::XMPP::Message (refer there for valid
                        settings).  Then it sends the message to the
                        server.

=head2 Presence Functions

    PresenceSend()                  - no arguments will send an empty
    PresenceSend(hash,                Presence to the server to tell it
                 signature=>string)   that you are available.  If you
                                      provide a hash, then it will pass
                                      that hash to the SetPresence()
                                      function as defined in the
                                      Net::XMPP::Presence module.
                                      Optionally, you can specify a
                                      signature and a jabber:x:signed
                                      will be placed in the <presence/>.

=head2 Subscription Functions

    Subscription(hash) - taks the hash and passes it to SetPresence in
                         Net::XMPP::Presence (refer there for valid
                         settings).  Then it sends the subscription to
                         server.

                         The valid types of subscription are:

                           subscribe    - subscribe to JID's presence
                           unsubscribe  - unsubscribe from JID's presence
                           subscribed   - response to a subscribe
                           unsubscribed - response to an unsubscribe

=head2 Presence DB Functions

    PresenceDB() - Tell the object to initialize the callbacks to
                   automatically populate the Presence DB.

    PresenceDBParse(Net::XMPP::Presence) - for every presence that you
                                             receive pass the Presence
                                             object to the DB so that
                                             it can track the resources
                                             and priorities for you.
                                             Returns either the presence
                                             passed in, if it not able
                                             to parsed for the DB, or the
                                             current presence as found by
                                             the PresenceDBQuery
                                             function.

    PresenceDBDelete(string|Net::XMPP::JID) - delete thes JID entry
                                                from the DB.

    PresenceDBClear() - delete all entries in the database.

    PresenceDBQuery(string|Net::XMPP::JID) - returns the NJ::Presence
                                               that was last received for
                                               the highest priority of
                                               this JID.  You can pass
                                               it a string or a NJ::JID
                                               object.

    PresenceDBResources(string|Net::XMPP::JID) - returns an array of
                                                   resources in order
                                                   from highest priority
                                                   to lowest.

=head2 IQ Functions

=head2 Auth Functions

    AuthSend(username=>string, - takes all of the information and
             password=>string,   builds a Net::XMPP::IQ::Auth packet.
             resource=>string)   It then sends that packet to the
                                 server with an ID and waits for that
                                 ID to return.  Then it looks in
                                 resulting packet and determines if
                                 authentication was successful for not.
                                 The array returned from AuthSend looks
                                 like this:
                                   [ type , message ]
                                 If type is "ok" then authentication
                                 was successful, otherwise message
                                 contains a little more detail about the
                                 error.

=head2 IQ::Register Functions

    RegisterRequest(to=>string,  - send an <iq/> request to the specified
                    timeout=>int)  server/transport, if not specified it
    RegisterRequest()              sends to the current active server.
                                   The function returns a hash that
                                   contains the required fields.   Here
                                   is an example of the hash:

                                   $hash{fields}    - The raw fields from
                                                      the iq:register.
                                                      To be used if there
                                                      is no x:data in the
                                                      packet.
                                   $hash{instructions} - How to fill out
                                                         the form.
                                   $hash{form}   - The new dynamic forms.

                                   In $hash{form}, the fields that are
                                   present are the required fields the
                                   server needs.

    RegisterSend(hash) - takes the contents of the hash and passes it
                         to the SetRegister function in the module
                         Net::XMPP::Query jabber:iq:register namespace.
                         This function returns an array that looks like
                         this:

                            [ type , message ]

                         If type is "ok" then registration was
                         successful, otherwise message contains a
                         little more detail about the error.

=head2 Roster Functions

    Roster() - returns a Net::XMPP::Roster object.  This will automatically
               intercept all of the roster and presence packets sent from
               the server and give you an accurate Roster.  For more
               information please read the man page for Net::XMPP::Roster.

    RosterParse(IQ object) - returns a hash that contains the roster
                             parsed into the following data structure:

                  $roster{'bob@jabber.org'}->{name}
                                      - Name you stored in the roster

                  $roster{'bob@jabber.org'}->{subscription}
                                      - Subscription status
                                        (to, from, both, none)

                  $roster{'bob@jabber.org'}->{ask}
                                      - The ask status from this user
                                        (subscribe, unsubscribe)

                  $roster{'bob@jabber.org'}->{groups}
                                      - Array of groups that
                                        bob@jabber.org is in

    RosterGet() - sends an empty Net::XMPP::IQ::Roster tag to the
                  server so the server will send the Roster to the
                  client.  Returns the above hash from RosterParse.

    RosterRequest() - sends an empty Net::XMPP::IQ::Roster tag to the
                      server so the server will send the Roster to the
                      client.

    RosterAdd(hash) - sends a packet asking that the jid be
                      added to the roster.  The hash format
                      is defined in the SetItem function
                      in the Net::XMPP::Query jabber:iq:roster
                      namespace.

    RosterRemove(hash) - sends a packet asking that the jid be
                         removed from the roster.  The hash
                         format is defined in the SetItem function
                         in the Net::XMPP::Query jabber:iq:roster
                         namespace.

=head2 Roster DB Functions

    RosterDB() - Tell the object to initialize the callbacks to
                 automatically populate the Roster DB.  If you do this,
                 then make sure that you call RosterRequest() instead of
                 RosterGet() so that the callbacks can catch it and
                 parse it.

    RosterDBParse(IQ object) - If you want to manually control the
                               database, then you can pass in all iq
                               packets with jabber:iq:roster queries to
                               this function.

    RosterDBAdd(jid,hash) - Add a new JID into the roster DB.  The JID
                            is either a string, or a Net::XMPP::JID
                            object.  The hash must be the same format as
                            the has returned by RosterParse above, and
                            is the actual hash, not a reference.

    RosterDBRemove(jid) - Remove a JID from the roster DB. The JID is
                          either a string, or a Net::XMPP::JID object.

    RosterDBClear() - Remove all JIDs from the roster DB.

    RosterDBExists(jid) - return 1 if the JID exists in the roster DB,
                          undef otherwise.  The JID is either a string,
                          or a Net::XMPP::JID object.

    RosterDBJIDs() - returns a list of Net::XMPP::JID objects that
                     represents all of the JIDs in the DB.

    RosterDBGroups() - returns the complete list of roster groups in the
                       roster.

    RosterDBGroupExists(group) - return 1 if the group is a group in the
                                 roster DB, undef otherwise.

    RosterDBGroupJIDs(group) - returns a list of Net::XMPP::JID objects
                               that represents all of the JIDs in the
                               specified roster group.

    RosterDBNonGroupJIDs() - returns a list of Net::XMPP::JID objects
                             that represents all of the JIDs not in a
                             roster group.

    RosterDBQuery(jid) - returns a hash containing the data from the
                         roster DB for the specified JID.  The JID is
                         either a string, or a Net::XMPP::JID object.
                         The hash format the same as in RosterParse
                         above.

    RosterDBQuery(jid,key) - returns the entry from the above hash for
                             the given key.  The available keys are:
                               name, ask, subsrcription and groups
                             The JID is either a string, or a
                             Net::XMPP::JID object.


=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use Net::XMPP::Roster;
use Net::XMPP::PrivacyLists;
use strict;
use Carp;
use vars qw( %XMLNS %NEWOBJECT $SASL_CALLBACK $TLS_CALLBACK );

##############################################################################
# Define the namespaces in an easy/constant manner.
#-----------------------------------------------------------------------------
# 1.0
#-----------------------------------------------------------------------------
$XMLNS{'xmppstreams'}   = "urn:ietf:params:xml:ns:xmpp-streams";
$XMLNS{'xmpp-bind'}     = "urn:ietf:params:xml:ns:xmpp-bind";
$XMLNS{'xmpp-sasl'}     = "urn:ietf:params:xml:ns:xmpp-sasl";
$XMLNS{'xmpp-session'}  = "urn:ietf:params:xml:ns:xmpp-session";
$XMLNS{'xmpp-tls'}      = "urn:ietf:params:xml:ns:xmpp-tls";
##############################################################################

##############################################################################
# BuildObject takes a root tag and builds the correct object.  NEWOBJECT is
# the table that maps tag to package.  Override these, or provide new ones.
#-----------------------------------------------------------------------------
$NEWOBJECT{'iq'}       = "Net::XMPP::IQ";
$NEWOBJECT{'message'}  = "Net::XMPP::Message";
$NEWOBJECT{'presence'} = "Net::XMPP::Presence";
$NEWOBJECT{'jid'}      = "Net::XMPP::JID";
##############################################################################

sub _message  { shift; my $o; eval "\$o = new $NEWOBJECT{'message'}(\@_);"; return $o;  }
sub _presence { shift; my $o; eval "\$o = new $NEWOBJECT{'presence'}(\@_);"; return $o; }
sub _iq       { shift; my $o; eval "\$o = new $NEWOBJECT{'iq'}(\@_);"; return $o;       }
sub _jid      { shift; my $o; eval "\$o = new $NEWOBJECT{'jid'}(\@_);"; return $o;      }

###############################################################################
#+-----------------------------------------------------------------------------
#|
#| Base API
#|
#+-----------------------------------------------------------------------------
###############################################################################

###############################################################################
#
# GetErrorCode - if you are returned an undef, you can call this function
#                and hopefully learn more information about the problem.
#
###############################################################################
sub GetErrorCode
{
    my $self = shift;
    return ((exists($self->{ERRORCODE}) && ($self->{ERRORCODE} ne "")) ?
            $self->{ERRORCODE} :
            $!
           );
}


###############################################################################
#
# SetErrorCode - sets the error code so that the caller can find out more
#                information about the problem
#
###############################################################################
sub SetErrorCode
{
    my $self = shift;
    my ($errorcode) = @_;
    $self->{ERRORCODE} = $errorcode;
}


###############################################################################
#
# CallBack - Central callback function.  If a packet comes back with an ID
#            and the tag and ID have been registered then the packet is not
#            returned as normal, instead it is inserted in the LIST and
#            stored until the user wants to fetch it.  If the tag and ID
#            are not registered the function checks if a callback exists
#            for this tag, if it does then that callback is called,
#            otherwise the function drops the packet since it does not know
#            how to handle it.
#
###############################################################################
sub CallBack
{
    my $self = shift;
    my $sid = shift;
    my ($object) = @_;

    my $tag;
    my $id;
    my $tree;

    if (ref($object) !~ /^Net::XMPP/)
    {
        if ($self->{DEBUG}->GetLevel() >= 1 || exists($self->{CB}->{receive}))
        {
            my $xml = $object->GetXML();
            $self->{DEBUG}->Log1("CallBack: sid($sid) received($xml)");
            &{$self->{CB}->{receive}}($sid,$xml) if exists($self->{CB}->{receive});
        }

        $tag = $object->get_tag();
        $id = "";
        $id = $object->get_attrib("id")
            if defined($object->get_attrib("id"));
        $tree = $object;
    }
    else
    {
        $tag = $object->GetTag();
        $id = $object->GetID();
        $tree = $object->GetTree();
    }

    $self->{DEBUG}->Log1("CallBack: tag($tag)");
    $self->{DEBUG}->Log1("CallBack: id($id)") if ($id ne "");

    my $pass = 1;
    $pass = 0
        if (!exists($self->{CB}->{$tag}) &&
            !exists($self->{CB}->{XPath}) &&
            !exists($self->{CB}->{DirectXPath}) &&
            !$self->CheckID($tag,$id)
           );

    if ($pass)
    {
        $self->{DEBUG}->Log1("CallBack: we either want it or were waiting for it.");

        if (exists($self->{CB}->{DirectXPath}))
        {
            $self->{DEBUG}->Log1("CallBack: check directxpath");

            my $direct_pass = 0;

            foreach my $xpath (keys(%{$self->{CB}->{DirectXPath}}))
            {
                $self->{DEBUG}->Log1("CallBack: check directxpath($xpath)");
                if ($object->XPathCheck($xpath))
                {
                    foreach my $func (keys(%{$self->{CB}->{DirectXPath}->{$xpath}}))
                    {
                        $self->{DEBUG}->Log1("CallBack: goto directxpath($xpath) function($func)");
                        &{$self->{CB}->{DirectXPath}->{$xpath}->{$func}}($sid,$object);
                        $direct_pass = 1;
                    }
                }
            }

            return if $direct_pass;
        }

        my $NJObject;
        if (ref($object) !~ /^Net::XMPP/)
        {
            $NJObject = $self->BuildObject($tag,$object);
        }
        else
        {
            $NJObject = $object;
        }

        if ($NJObject == -1)
        {
            $self->{DEBUG}->Log1("CallBack: DANGER!! DANGER!! We didn't build a packet!  We're all gonna die!!");
        }
        else
        {
            if ($self->CheckID($tag,$id))
            {
                $self->{DEBUG}->Log1("CallBack: found registry entry: tag($tag) id($id)");
                $self->DeregisterID($tag,$id);
                if ($self->TimedOutID($id))
                {
                    $self->{DEBUG}->Log1("CallBack: dropping packet due to timeout");
                    $self->CleanID($id);
                }
                else
                {
                    $self->{DEBUG}->Log1("CallBack: they still want it... we still got it...");
                    $self->GotID($id,$NJObject);
                }
            }
            else
            {
                $self->{DEBUG}->Log1("CallBack: no registry entry");

                if (exists($self->{CB}->{XPath}))
                {
                    $self->{DEBUG}->Log1("CallBack: check xpath");

                    foreach my $xpath (keys(%{$self->{CB}->{XPath}}))
                    {
                        if ($NJObject->GetTree()->XPathCheck($xpath))
                        {
                            foreach my $func (keys(%{$self->{CB}->{XPath}->{$xpath}}))
                            {
                                $self->{DEBUG}->Log1("CallBack: goto xpath($xpath) function($func)");
                                &{$self->{CB}->{XPath}->{$xpath}->{$func}}($sid,$NJObject);
                            }
                        }
                    }
                }

                if (exists($self->{CB}->{$tag}))
                {
                    $self->{DEBUG}->Log1("CallBack: goto user function($self->{CB}->{$tag})");
                    &{$self->{CB}->{$tag}}($sid,$NJObject);
                }
                else
                {
                    $self->{DEBUG}->Log1("CallBack: no defined function.  Dropping packet.");
                }
            }
        }
    }
    else
    {
        $self->{DEBUG}->Log1("CallBack: a packet that no one wants... how sad. =(");
    }
}


###############################################################################
#
# BuildObject - turn the packet into an object.
#
###############################################################################
sub BuildObject
{
    my $self = shift;
    my ($tag,$tree) = @_;

    my $obj = -1;

    if (exists($NEWOBJECT{$tag}))
    {
        $self->{DEBUG}->Log1("BuildObject: tag($tag) package($NEWOBJECT{$tag})");
        eval "\$obj = new $NEWOBJECT{$tag}(\$tree);";
    }

    return $obj;
}


###############################################################################
#
# SetCallBacks - Takes a hash with top level tags to look for as the keys
#                and pointers to functions as the values.  The functions
#                are called and passed the XML::Parser::Tree objects
#                generated by XML::Stream.
#
###############################################################################
sub SetCallBacks
{
    my $self = shift;
    while($#_ >= 0)
    {
        my $func = pop(@_);
        my $tag = pop(@_);
        $self->{DEBUG}->Log1("SetCallBacks: tag($tag) func($func)");
        if (defined($func))
        {
            $self->{CB}->{$tag} = $func;
        }
        else
        {
            delete($self->{CB}->{$tag});
        }
        $self->{STREAM}->SetCallBacks(update=>$func) if ($tag eq "update");
    }
}


###############################################################################
#
# SetIQCallBacks - define callbacks for the namespaces inside an iq.
#
###############################################################################
sub SetIQCallBacks
{
    my $self = shift;

    while($#_ >= 0)
    {
        my $hash = pop(@_);
        my $namespace = pop(@_);

        foreach my $type (keys(%{$hash}))
        {
            if (defined($hash->{$type}))
            {
                $self->{CB}->{IQns}->{$namespace}->{$type} = $hash->{$type};
            }
            else
            {
                delete($self->{CB}->{IQns}->{$namespace}->{$type});
            }
        }
    }
}


###############################################################################
#
# SetPresenceCallBacks - define callbacks for the different presence packets.
#
###############################################################################
sub SetPresenceCallBacks
{
    my $self = shift;
    my (%types) = @_;

    foreach my $type (keys(%types))
    {
        if (defined($types{$type}))
        {
            $self->{CB}->{Pres}->{$type} = $types{$type};
        }
        else
        {
            delete($self->{CB}->{Pres}->{$type});
        }
    }
}


###############################################################################
#
# SetMessageCallBacks - define callbacks for the different message packets.
#
###############################################################################
sub SetMessageCallBacks
{
    my $self = shift;
    my (%types) = @_;

    foreach my $type (keys(%types))
    {
        if (defined($types{$type}))
        {
            $self->{CB}->{Mess}->{$type} = $types{$type};
        }
        else
        {
            delete($self->{CB}->{Mess}->{$type});
        }
    }
}


###############################################################################
#
# SetXPathCallBacks - define callbacks for packets based on XPath.
#
###############################################################################
sub SetXPathCallBacks
{
    my $self = shift;
    my (%xpaths) = @_;

    foreach my $xpath (keys(%xpaths))
    {
        $self->{DEBUG}->Log1("SetXPathCallBacks: xpath($xpath) func($xpaths{$xpath})");
        $self->{CB}->{XPath}->{$xpath}->{$xpaths{$xpath}} = $xpaths{$xpath};
    }
}


###############################################################################
#
# RemoveXPathCallBacks - remove callbacks for packets based on XPath.
#
###############################################################################
sub RemoveXPathCallBacks
{
    my $self = shift;
    my (%xpaths) = @_;

    foreach my $xpath (keys(%xpaths))
    {
        $self->{DEBUG}->Log1("RemoveXPathCallBacks: xpath($xpath) func($xpaths{$xpath})");
        delete($self->{CB}->{XPath}->{$xpath}->{$xpaths{$xpath}});
        delete($self->{CB}->{XPath}->{$xpath})
            if (scalar(keys(%{$self->{CB}->{XPath}->{$xpath}})) == 0);
        delete($self->{CB}->{XPath})
            if (scalar(keys(%{$self->{CB}->{XPath}})) == 0);
    }
}


###############################################################################
#
# SetDirectXPathCallBacks - define callbacks for packets based on XPath.
#
###############################################################################
sub SetDirectXPathCallBacks
{
    my $self = shift;
    my (%xpaths) = @_;

    foreach my $xpath (keys(%xpaths))
    {
        $self->{DEBUG}->Log1("SetDirectXPathCallBacks: xpath($xpath) func($xpaths{$xpath})");
        $self->{CB}->{DirectXPath}->{$xpath}->{$xpaths{$xpath}} = $xpaths{$xpath};
    }
}


###############################################################################
#
# RemoveDirectXPathCallBacks - remove callbacks for packets based on XPath.
#
###############################################################################
sub RemoveDirectXPathCallBacks
{
    my $self = shift;
    my (%xpaths) = @_;

    foreach my $xpath (keys(%xpaths))
    {
        $self->{DEBUG}->Log1("RemoveDirectXPathCallBacks: xpath($xpath) func($xpaths{$xpath})");
        delete($self->{CB}->{DirectXPath}->{$xpath}->{$xpaths{$xpath}});
        delete($self->{CB}->{DirectXPath}->{$xpath})
            if (scalar(keys(%{$self->{CB}->{DirectXPath}->{$xpath}})) == 0);
        delete($self->{CB}->{DirectXPath})
            if (scalar(keys(%{$self->{CB}->{DirectXPath}})) == 0);
    }
}


###############################################################################
#
# Send - Takes either XML or a Net::XMPP::xxxx object and sends that
#        packet to the server.
#
###############################################################################
sub Send
{
    my $self = shift;
    my $object = shift;
    my $ignoreActivity = shift;
    $ignoreActivity = 0 unless defined($ignoreActivity);

    if (ref($object) eq "")
    {
        $self->SendXML($object,$ignoreActivity);
    }
    else
    {
        $self->SendXML($object->GetXML(),$ignoreActivity);
    }
}


###############################################################################
#
# SendXML - Sends the XML packet to the server
#
###############################################################################
sub SendXML
{
    my $self = shift;
    my $xml = shift;
    my $ignoreActivity = shift;
    $ignoreActivity = 0 unless defined($ignoreActivity);

    $self->{DEBUG}->Log1("SendXML: sent($xml)");
    &{$self->{CB}->{send}}($self->GetStreamID(),$xml) if exists($self->{CB}->{send});
    $self->{STREAM}->IgnoreActivity($self->GetStreamID(),$ignoreActivity);
    $self->{STREAM}->Send($self->GetStreamID(),$xml);
    $self->{STREAM}->IgnoreActivity($self->GetStreamID(),0);
}


###############################################################################
#
# SendWithID - Take either XML or a Net::XMPP::xxxx object and send it
#              with the next available ID number.  Then return that ID so
#              the client can track it.
#
###############################################################################
sub SendWithID
{
    my $self = shift;
    my ($object) = @_;

    #--------------------------------------------------------------------------
    # Take the current XML stream and insert an id attrib at the top level.
    #--------------------------------------------------------------------------
    my $id = $self->UniqueID();

    $self->{DEBUG}->Log1("SendWithID: id($id)");

    my $xml;
    if (ref($object) eq "")
    {
        $self->{DEBUG}->Log1("SendWithID: in($object)");
        $xml = $object;
        $xml =~ s/^(\<[^\>]+)(\>)/$1 id\=\'$id\'$2/;
        my ($tag) = ($xml =~ /^\<(\S+)\s/);
        $self->RegisterID($tag,$id);
    }
    else
    {
        $self->{DEBUG}->Log1("SendWithID: in(",$object->GetXML(),")");
        $object->SetID($id);
        $xml = $object->GetXML();
        $self->RegisterID($object->GetTag(),$id);
    }
    $self->{DEBUG}->Log1("SendWithID: out($xml)");

    #--------------------------------------------------------------------------
    # Send the new XML string.
    #--------------------------------------------------------------------------
    $self->SendXML($xml);

    #--------------------------------------------------------------------------
    # Return the ID number we just assigned.
    #--------------------------------------------------------------------------
    return $id;
}


###############################################################################
#
# UniqueID - Increment and return a new unique ID.
#
###############################################################################
sub UniqueID
{
    my $self = shift;

    my $id_num = $self->{RCVDB}->{currentID};

    $self->{RCVDB}->{currentID}++;

    return "netjabber-$id_num";
}


###############################################################################
#
# SendAndReceiveWithID - Take either XML or a Net::XMPP::xxxxx object and
#                        send it with the next ID.  Then wait for that ID
#                        to come back and return the response in a
#                        Net::XMPP::xxxx object.
#
###############################################################################
sub SendAndReceiveWithID
{
    my $self = shift;
    my ($object,$timeout) = @_;
    &{$self->{CB}->{startwait}}() if exists($self->{CB}->{startwait});
    $self->{DEBUG}->Log1("SendAndReceiveWithID: object($object)");
    my $id = $self->SendWithID($object);
    $self->{DEBUG}->Log1("SendAndReceiveWithID: sent with id($id)");
    my $packet = $self->WaitForID($id,$timeout);
    &{$self->{CB}->{endwait}}() if exists($self->{CB}->{endwait});
    return $packet;
}


###############################################################################
#
# ReceivedID - returns 1 if a packet with the ID has been received, or 0
#              if it has not.
#
###############################################################################
sub ReceivedID
{
    my $self = shift;
    my ($id) = @_;

    $self->{DEBUG}->Log1("ReceivedID: id($id)");
    return 1 if exists($self->{RCVDB}->{$id});
    $self->{DEBUG}->Log1("ReceivedID: nope...");
    return 0;
}


###############################################################################
#
# GetID - Return the Net::XMPP::xxxxx object that is stored in the LIST
#         that matches the ID if that ID exists.  Otherwise return 0.
#
###############################################################################
sub GetID
{
    my $self = shift;
    my ($id) = @_;

    $self->{DEBUG}->Log1("GetID: id($id)");
    return $self->{RCVDB}->{$id} if $self->ReceivedID($id);
    $self->{DEBUG}->Log1("GetID: haven't gotten that id yet...");
    return 0;
}


###############################################################################
#
# CleanID - Delete the list entry for this id since we don't want a leak.
#
###############################################################################
sub CleanID
{
    my $self = shift;
    my ($id) = @_;

    $self->{DEBUG}->Log1("CleanID: id($id)");
    delete($self->{RCVDB}->{$id});
}


###############################################################################
#
# WaitForID - Keep looping and calling Process(1) to poll every second
#             until the response from the server occurs.
#
###############################################################################
sub WaitForID
{
    my $self = shift;
    my ($id,$timeout) = @_;
    $timeout = "300" unless defined($timeout);

    $self->{DEBUG}->Log1("WaitForID: id($id)");
    my $endTime = time + $timeout;
    while(!$self->ReceivedID($id) && ($endTime >= time))
    {
        $self->{DEBUG}->Log1("WaitForID: haven't gotten it yet... let's wait for more packets");
        return unless (defined($self->Process(1)));
        &{$self->{CB}->{update}}() if exists($self->{CB}->{update});
    }
    if (!$self->ReceivedID($id))
    {
        $self->TimeoutID($id);
        $self->{DEBUG}->Log1("WaitForID: timed out...");
        return;
    }
    else
    {
        $self->{DEBUG}->Log1("WaitForID: we got it!");
        my $packet = $self->GetID($id);
        $self->CleanID($id);
        return $packet;
    }
}


###############################################################################
#
# GotID - Callback to store the Net::XMPP::xxxxx object in the LIST at
#         the ID index.  This is a private helper function.
#
###############################################################################
sub GotID
{
    my $self = shift;
    my ($id,$object) = @_;

    $self->{DEBUG}->Log1("GotID: id($id) xml(",$object->GetXML(),")");
    $self->{RCVDB}->{$id} = $object;
}


###############################################################################
#
# CheckID - Checks the ID registry if this tag and ID have been registered.
#           0 = no, 1 = yes
#
###############################################################################
sub CheckID
{
    my $self = shift;
    my ($tag,$id) = @_;
    $id = "" unless defined($id);

    $self->{DEBUG}->Log1("CheckID: tag($tag) id($id)");
    return 0 if ($id eq "");
    $self->{DEBUG}->Log1("CheckID: we have that here somewhere...");
    return exists($self->{IDRegistry}->{$tag}->{$id});
}


###############################################################################
#
# TimeoutID - Timeout the tag and ID in the registry so that the CallBack
#             can know what to put in the ID list and what to pass on.
#
###############################################################################
sub TimeoutID
{
    my $self = shift;
    my ($id) = @_;

    $self->{DEBUG}->Log1("TimeoutID: id($id)");
    $self->{RCVDB}->{$id} = 0;
}


###############################################################################
#
# TimedOutID - Timeout the tag and ID in the registry so that the CallBack
#             can know what to put in the ID list and what to pass on.
#
###############################################################################
sub TimedOutID
{
    my $self = shift;
    my ($id) = @_;

    return (exists($self->{RCVDB}->{$id}) && ($self->{RCVDB}->{$id} == 0));
}


###############################################################################
#
# RegisterID - Register the tag and ID in the registry so that the CallBack
#              can know what to put in the ID list and what to pass on.
#
###############################################################################
sub RegisterID
{
    my $self = shift;
    my ($tag,$id) = @_;

    $self->{DEBUG}->Log1("RegisterID: tag($tag) id($id)");
    $self->{IDRegistry}->{$tag}->{$id} = 1;
}


###############################################################################
#
# DeregisterID - Delete the tag and ID in the registry so that the CallBack
#                can knows that it has been received.
#
###############################################################################
sub DeregisterID
{
    my $self = shift;
    my ($tag,$id) = @_;

    $self->{DEBUG}->Log1("DeregisterID: tag($tag) id($id)");
    delete($self->{IDRegistry}->{$tag}->{$id});
}


###############################################################################
#
# AddNamespace - Add a custom namespace into the mix.
#
###############################################################################
sub AddNamespace
{
    my $self = shift;
    &Net::XMPP::Namespaces::add_ns(@_);
}


###############################################################################
#
# MessageSend - Takes the same hash that Net::XMPP::Message->SetMessage
#               takes and sends the message to the server.
#
###############################################################################
sub MessageSend
{
    my $self = shift;

    my $mess = $self->_message();
    $mess->SetMessage(@_);
    $self->Send($mess);
}


##############################################################################
#
# PresenceDB - initialize the module to use the presence database
#
##############################################################################
sub PresenceDB
{
    my $self = shift;

    $self->SetXPathCallBacks('/presence'=>sub{ shift; $self->PresenceDBParse(@_) });
}


###############################################################################
#
# PresenceDBParse - adds the presence information to the Presence DB so
#                   you can keep track of the current state of the JID and
#                   all of it's resources.
#
###############################################################################
sub PresenceDBParse
{
    my $self = shift;
    my ($presence) = @_;

    $self->{DEBUG}->Log4("PresenceDBParse: pres(",$presence->GetXML(),")");

    my $type = $presence->GetType();
    $type = "" unless defined($type);
    return $presence unless (($type eq "") ||
                 ($type eq "available") ||
                 ($type eq "unavailable"));

    my $fromJID = $presence->GetFrom("jid");
    my $fromID = $fromJID->GetJID();
    $fromID = "" unless defined($fromID);
    my $resource = $fromJID->GetResource();
    $resource = " " unless ($resource ne "");
    my $priority = $presence->GetPriority();
    $priority = 0 unless defined($priority);

    $self->{DEBUG}->Log1("PresenceDBParse: fromJID(",$fromJID->GetJID("full"),") resource($resource) priority($priority) type($type)");
    $self->{DEBUG}->Log2("PresenceDBParse: xml(",$presence->GetXML(),")");

    if (exists($self->{PRESENCEDB}->{$fromID}))
    {
        my $oldPriority = $self->{PRESENCEDB}->{$fromID}->{resources}->{$resource};
        $oldPriority = "" unless defined($oldPriority);

        my $loc = 0;
        foreach my $index (0..$#{$self->{PRESENCEDB}->{$fromID}->{priorities}->{$oldPriority}})
        {
            $loc = $index
               if ($self->{PRESENCEDB}->{$fromID}->{priorities}->{$oldPriority}->[$index]->{resource} eq $resource);
        }
        splice(@{$self->{PRESENCEDB}->{$fromID}->{priorities}->{$oldPriority}},$loc,1);
        delete($self->{PRESENCEDB}->{$fromID}->{resources}->{$resource});
        delete($self->{PRESENCEDB}->{$fromID}->{priorities}->{$oldPriority})
            if (exists($self->{PRESENCEDB}->{$fromID}->{priorities}->{$oldPriority}) &&
        ($#{$self->{PRESENCEDB}->{$fromID}->{priorities}->{$oldPriority}} == -1));
        delete($self->{PRESENCEDB}->{$fromID})
            if (scalar(keys(%{$self->{PRESENCEDB}->{$fromID}})) == 0);

        $self->{DEBUG}->Log1("PresenceDBParse: remove ",$fromJID->GetJID("full")," from the DB");
    }

    if (($type eq "") || ($type eq "available"))
    {
        my $loc = -1;
        foreach my $index (0..$#{$self->{PRESENCEDB}->{$fromID}->{priorities}->{$priority}}) {
            $loc = $index
                if ($self->{PRESENCEDB}->{$fromID}->{priorities}->{$priority}->[$index]->{resource} eq $resource);
        }
        $loc = $#{$self->{PRESENCEDB}->{$fromID}->{priorities}->{$priority}}+1
            if ($loc == -1);
        $self->{PRESENCEDB}->{$fromID}->{resources}->{$resource} = $priority;
        $self->{PRESENCEDB}->{$fromID}->{priorities}->{$priority}->[$loc]->{presence} =
            $presence;
        $self->{PRESENCEDB}->{$fromID}->{priorities}->{$priority}->[$loc]->{resource} =
            $resource;

        $self->{DEBUG}->Log1("PresenceDBParse: add ",$fromJID->GetJID("full")," to the DB");
    }

    my $currentPresence = $self->PresenceDBQuery($fromJID);
    return (defined($currentPresence) ? $currentPresence : $presence);
}


###############################################################################
#
# PresenceDBDelete - delete the JID from the DB completely.
#
###############################################################################
sub PresenceDBDelete
{
    my $self = shift;
    my ($jid) = @_;

    my $indexJID = $jid;
    $indexJID = $jid->GetJID() if $jid->isa("Net::XMPP::JID");

    return if !exists($self->{PRESENCEDB}->{$indexJID});
    delete($self->{PRESENCEDB}->{$indexJID});
    $self->{DEBUG}->Log1("PresenceDBDelete: delete ",$indexJID," from the DB");
}


###############################################################################
#
# PresenceDBClear - delete all of the JIDs from the DB completely.
#
###############################################################################
sub PresenceDBClear
{
    my $self = shift;

    $self->{DEBUG}->Log1("PresenceDBClear: clearing the database");
    foreach my $indexJID (keys(%{$self->{PRESENCEDB}}))
    {
        $self->{DEBUG}->Log3("PresenceDBClear: deleting ",$indexJID," from the DB");
        delete($self->{PRESENCEDB}->{$indexJID});
    }
    $self->{DEBUG}->Log3("PresenceDBClear: database is empty");
}


###############################################################################
#
# PresenceDBQuery - retrieve the last Net::XMPP::Presence received with
#                  the highest priority.
#
###############################################################################
sub PresenceDBQuery
{
    my $self = shift;
    my ($jid) = @_;

    my $indexJID = $jid;
    $indexJID = $jid->GetJID() if $jid->isa("Net::XMPP::JID");

    return if !exists($self->{PRESENCEDB}->{$indexJID});
    return if (scalar(keys(%{$self->{PRESENCEDB}->{$indexJID}->{priorities}})) == 0);

    my $highPriority =
        (sort {$b cmp $a} keys(%{$self->{PRESENCEDB}->{$indexJID}->{priorities}}))[0];

    return $self->{PRESENCEDB}->{$indexJID}->{priorities}->{$highPriority}->[0]->{presence};
}


###############################################################################
#
# PresenceDBResources - returns a list of the resources from highest
#                       priority to lowest.
#
###############################################################################
sub PresenceDBResources
{
    my $self = shift;
    my ($jid) = @_;

    my $indexJID = $jid;
    $indexJID = $jid->GetJID() if $jid->isa("Net::XMPP::JID");

    my @resources;

    return if !exists($self->{PRESENCEDB}->{$indexJID});

    foreach my $priority (sort {$b cmp $a} keys(%{$self->{PRESENCEDB}->{$indexJID}->{priorities}}))
    {
        foreach my $index (0..$#{$self->{PRESENCEDB}->{$indexJID}->{priorities}->{$priority}})
        {
            next if ($self->{PRESENCEDB}->{$indexJID}->{priorities}->{$priority}->[$index]->{resource} eq " ");
            push(@resources,$self->{PRESENCEDB}->{$indexJID}->{priorities}->{$priority}->[$index]->{resource});
        }
    }
    return @resources;
}


###############################################################################
#
# PresenceSend - Sends a presence tag to announce your availability
#
###############################################################################
sub PresenceSend
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $args{ignoreactivity} = 0 unless exists($args{ignoreactivity});
    my $ignoreActivity = delete($args{ignoreactivity});

    my $presence = $self->_presence();

    $presence->SetPresence(%args);
    $self->Send($presence,$ignoreActivity);
    return $presence;
}


###############################################################################
#
# PresenceProbe - Sends a presence probe to the server
#
###############################################################################
sub PresenceProbe
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }
    delete($args{type});

    my $presence = $self->_presence();
    $presence->SetPresence(type=>"probe",
                           %args);
    $self->Send($presence);
}


###############################################################################
#
# Subscription - Sends a presence tag to perform the subscription on the
#                specified JID.
#
###############################################################################
sub Subscription
{
    my $self = shift;

    my $presence = $self->_presence();
    $presence->SetPresence(@_);
    $self->Send($presence);
}


###############################################################################
#
# AuthSend - This is a self contained function to send a login iq tag with
#            an id.  Then wait for a reply what the same id to come back
#            and tell the caller what the result was.
#
###############################################################################
sub AuthSend
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    carp("AuthSend requires a username arguement")
        unless exists($args{username});
    carp("AuthSend requires a password arguement")
        unless exists($args{password});

    if($self->{STREAM}->GetStreamFeature($self->GetStreamID(),"xmpp-sasl"))
    {
        return $self->AuthSASL(%args);
    }

    return $self->AuthIQAuth(%args);
}


###############################################################################
#
# AuthIQAuth - Try and auth using jabber:iq:auth, the old Jabber way of
#              authenticating.
#
###############################################################################
sub AuthIQAuth
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $self->{DEBUG}->Log1("AuthIQAuth: old school auth");

    carp("AuthIQAuth requires a resource arguement")
        unless exists($args{resource});

    my $authType = "digest";
    my $token;
    my $sequence;

    #--------------------------------------------------------------------------
    # First let's ask the sever what all is available in terms of auth types.
    # If we get an error, then all we can do is digest or plain.
    #--------------------------------------------------------------------------
    my $iqAuth = $self->_iq();
    $iqAuth->SetIQ(type=>"get");
    my $iqAuthQuery = $iqAuth->NewChild("jabber:iq:auth");
    $iqAuthQuery->SetUsername($args{username});
    $iqAuth = $self->SendAndReceiveWithID($iqAuth);

    return unless defined($iqAuth);
    return ( $iqAuth->GetErrorCode() , $iqAuth->GetError() )
        if ($iqAuth->GetType() eq "error");

    if ($iqAuth->GetType() eq "error")
    {
        $authType = "digest";
    }
    else
    {
        $iqAuthQuery = $iqAuth->GetChild();
        $authType = "plain" if $iqAuthQuery->DefinedPassword();
        $authType = "digest" if $iqAuthQuery->DefinedDigest();
        $authType = "zerok" if ($iqAuthQuery->DefinedSequence() &&
                    $iqAuthQuery->DefinedToken());
        $token = $iqAuthQuery->GetToken() if ($authType eq "zerok");
        $sequence = $iqAuthQuery->GetSequence() if ($authType eq "zerok");
    }

    $self->{DEBUG}->Log1("AuthIQAuth: authType($authType)");

    delete($args{digest});
    delete($args{type});
    my $password = delete $args{password};
    if (ref($password) eq 'CODE')
    {
        $password = $password->();
    }

    #--------------------------------------------------------------------------
    # 0k authenticaion (http://core.jabber.org/0k.html)
    #
    # Tell the server that we want to connect this way, the server sends back
    # a token and a sequence number.  We take that token + the password and
    # SHA1 it.  Then we SHA1 it sequence number more times and send that hash.
    # The server SHA1s that hash one more time and compares it to the hash it
    # stored last time.  IF they match, we are in and it stores the hash we sent
    # for the next time and decreases the sequence number, else, no go.
    #--------------------------------------------------------------------------
    if ($authType eq "zerok")
    {
        my $hashA = Digest::SHA::sha1_hex($password);
        $args{hash} = Digest::SHA::sha1_hex($hashA.$token);

        for (1..$sequence)
        {
            $args{hash} = Digest::SHA::sha1_hex($args{hash});
        }
    }

    #--------------------------------------------------------------------------
    # If we have access to the SHA-1 digest algorithm then let's use it.
    # Remove the password from the hash, create the digest, and put the
    # digest in the hash instead.
    #
    # Note: Concat the Session ID and the password and then digest that
    # string to get the server to accept the digest.
    #--------------------------------------------------------------------------
    if ($authType eq "digest")
    {
        $args{digest} = Digest::SHA::sha1_hex($self->GetStreamID().$password);
    }

    #--------------------------------------------------------------------------
    # Create a Net::XMPP::IQ object to send to the server
    #--------------------------------------------------------------------------
    my $iqLogin = $self->_iq();
    $iqLogin->SetIQ(type=>"set");
    my $iqLoginQuery = $iqLogin->NewChild("jabber:iq:auth");
    $iqLoginQuery->SetAuth(%args);

    #--------------------------------------------------------------------------
    # Send the IQ with the next available ID and wait for a reply with that
    # id to be received.  Then grab the IQ reply.
    #--------------------------------------------------------------------------
    $iqLogin = $self->SendAndReceiveWithID($iqLogin);

    #--------------------------------------------------------------------------
    # From the reply IQ determine if we were successful or not.  If yes then
    # return "".  If no then return error string from the reply.
    #--------------------------------------------------------------------------
    $password =~ tr/\0-\377/x/;
    return unless defined($iqLogin);
    return ( $iqLogin->GetErrorCode() , $iqLogin->GetError() )
        if ($iqLogin->GetType() eq "error");

    $self->{DEBUG}->Log1("AuthIQAuth: we authed!");

    return ("ok","");
}


###############################################################################
#
# AuthSASL - Try and auth using SASL, the XMPP preferred way of authenticating.
#
###############################################################################
sub AuthSASL
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $self->{DEBUG}->Log1("AuthSASL: shiney new auth");

    carp("AuthSASL requires a username arguement")
        unless exists($args{username});
    carp("AuthSASL requires a password arguement")
        unless exists($args{password});

    $args{resource} = "" unless exists($args{resource});

    #-------------------------------------------------------------------------
    # Create the SASLClient on our end
    #-------------------------------------------------------------------------
    my $sid = $self->{SESSION}->{id};
    my $status =
        $self->{STREAM}->SASLClient($sid,
                                    $args{username},
                                    $args{password}
                                   );

    $args{timeout} = "120" unless exists($args{timeout});

    #-------------------------------------------------------------------------
    # While we haven't timed out, keep waiting for the SASLClient to finish
    #-------------------------------------------------------------------------
    my $endTime = time + $args{timeout};
    while(!$self->{STREAM}->SASLClientDone($sid) && ($endTime >= time))
    {
        $self->{DEBUG}->Log1("AuthSASL: haven't authed yet... let's wait.");
        return unless (defined($self->Process(1)));
        &{$self->{CB}->{update}}() if exists($self->{CB}->{update});
    }

    #-------------------------------------------------------------------------
    # The loop finished... but was it done?
    #-------------------------------------------------------------------------
    if (!$self->{STREAM}->SASLClientDone($sid))
    {
        $self->{DEBUG}->Log1("AuthSASL: timed out...");
        return( "system","SASL timed out authenticating");
    }

    #-------------------------------------------------------------------------
    # Ok, it was done... but did we auth?
    #-------------------------------------------------------------------------
    if (!$self->{STREAM}->SASLClientAuthed($sid))
    {
        $self->{DEBUG}->Log1("AuthSASL: Authentication failed.");
        return ( "error", $self->{STREAM}->SASLClientError($sid));
    }

    #-------------------------------------------------------------------------
    # Phew... Restart the <stream:stream> per XMPP
    #-------------------------------------------------------------------------
    $self->{DEBUG}->Log1("AuthSASL: We authed!");
    $self->{SESSION} = $self->{STREAM}->OpenStream($sid);
    $sid = $self->{SESSION}->{id};

    $self->{DEBUG}->Log1("AuthSASL: We got a new session. sid($sid)");

    #-------------------------------------------------------------------------
    # Look in the new set of <stream:feature/>s and see if xmpp-bind was
    # offered.
    #-------------------------------------------------------------------------
    my $bind = $self->{STREAM}->GetStreamFeature($sid,"xmpp-bind");
    if ($bind)
    {
        $self->{DEBUG}->Log1("AuthSASL: Binding to resource");
        my $jid = $self->BindResource($args{resource});
	$self->{SESSION}->{FULLJID} = $jid;
    }

    #-------------------------------------------------------------------------
    # Look in the new set of <stream:feature/>s and see if xmpp-session was
    # offered.
    #-------------------------------------------------------------------------
    my $session = $self->{STREAM}->GetStreamFeature($sid,"xmpp-session");
    if ($session)
    {
        $self->{DEBUG}->Log1("AuthSASL: Starting session");
        $self->StartSession();
    }

    return ("ok","");
}


##############################################################################
#
# BindResource - bind to a resource
#
##############################################################################
sub BindResource
{
    my $self = shift;
    my $resource = shift;

    $self->{DEBUG}->Log2("BindResource: Binding to resource");
    my $iq = $self->_iq();

    $iq->SetIQ(type=>"set");
    my $bind = $iq->NewChild(&ConstXMLNS("xmpp-bind"));

    if (defined($resource) && ($resource ne ""))
    {
        $self->{DEBUG}->Log2("BindResource: resource($resource)");
        $bind->SetBind(resource=>$resource);
    }

    my $result = $self->SendAndReceiveWithID($iq);
    return $result->GetChild(&ConstXMLNS("xmpp-bind"))->GetJID();;
}


##############################################################################
#
# StartSession - Initialize a session
#
##############################################################################
sub StartSession
{
    my $self = shift;

    my $iq = $self->_iq();

    $iq->SetIQ(type=>"set");
    my $session = $iq->NewChild(&ConstXMLNS("xmpp-session"));

    my $result = $self->SendAndReceiveWithID($iq);
}


##############################################################################
#
# PrivacyLists - Initialize a Net::XMPP::PrivacyLists object and return it.
#
##############################################################################
sub PrivacyLists
{
    my $self = shift;

    return Net::XMPP::PrivacyLists->new(connection=>$self);
}


##############################################################################
#
# PrivacyListsGet - Sends an empty IQ to the server to request that the user's
#                   Privacy Lists be sent to them.  Returns the iq packet
#                   of the result.
#
##############################################################################
sub PrivacyListsGet
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"get");
    my $query = $iq->NewChild("jabber:iq:privacy");

    if (exists($args{list}))
    {
        $query->AddList(name=>$args{list});
    }

    $iq = $self->SendAndReceiveWithID($iq);
    return unless defined($iq);

    return $iq;
}


##############################################################################
#
# PrivacyListsRequest - Sends an empty IQ to the server to request that the
#                       user's privacy lists be sent to them, and return to
#                       let the user's program handle parsing the return packet.
#
##############################################################################
sub PrivacyListsRequest
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"get");
    my $query = $iq->NewChild("jabber:iq:privacy");

    if (exists($args{list}))
    {
        $query->AddList(name=>$args{list});
    }

    $self->Send($iq);
}


##############################################################################
#
# PrivacyListsSet - Sends an empty IQ to the server to request that the
#                       user's privacy lists be sent to them, and return to
#                       let the user's program handle parsing the return packet.
#
##############################################################################
sub PrivacyListsSet
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"set");
    my $query = $iq->NewChild("jabber:iq:privacy");

    #XXX error check that there is a list
    my $list = $query->AddList(name=>$args{list});

    foreach my $item (@{$args{items}})
    {
        $list->AddItem(%{$item});
    }

    $iq = $self->SendAndReceiveWithID($iq);
    return unless defined($iq);

    return if $iq->DefinedError();

    return 1;
}


###############################################################################
#
# RegisterRequest - This is a self contained function to send an iq tag
#                   an id that requests the target address to send back
#                   the required fields.  It waits for a reply what the
#                   same id to come back and tell the caller what the
#                   fields are.
#
###############################################################################
sub RegisterRequest
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    #--------------------------------------------------------------------------
    # Create a Net::XMPP::IQ object to send to the server
    #--------------------------------------------------------------------------
    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to})) if exists($args{to});
    $iq->SetIQ(type=>"get");
    my $query = $iq->NewChild("jabber:iq:register");

    #--------------------------------------------------------------------------
    # Send the IQ with the next available ID and wait for a reply with that
    # id to be received.  Then grab the IQ reply.
    #--------------------------------------------------------------------------
    if ($args{mode} eq "passthru")
    {
        my $id = $self->UniqueID();
        $iq->SetIQ(id=>$id);
        $self->Send($iq);
        return $id;
    }

    return $self->SendWithID($iq) if ($args{mode} eq "nonblock");

    $iq = $self->SendAndReceiveWithID($iq,$timeout);

    #--------------------------------------------------------------------------
    # Check if there was an error.
    #--------------------------------------------------------------------------
    return unless defined($iq);
    if ($iq->GetType() eq "error")
    {
        $self->SetErrorCode($iq->GetErrorCode().": ".$iq->GetError());
        return;
    }

    my %register;
    #--------------------------------------------------------------------------
    # From the reply IQ determine what fields are required and send a hash
    # back with the fields and any values that are already defined (like key)
    #--------------------------------------------------------------------------
    $query = $iq->GetChild();
    $register{fields} = { $query->GetRegister() };

    return %register;
}


###############################################################################
#
# RegisterSend - This is a self contained function to send a registration
#                iq tag with an id.  Then wait for a reply what the same
#                id to come back and tell the caller what the result was.
#
###############################################################################
sub RegisterSend
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    #--------------------------------------------------------------------------
    # Create a Net::XMPP::IQ object to send to the server
    #--------------------------------------------------------------------------
    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to})) if exists($args{to});
    $iq->SetIQ(type=>"set");
    my $iqRegister = $iq->NewChild("jabber:iq:register");
    $iqRegister->SetRegister(%args);

    #--------------------------------------------------------------------------
    # Send the IQ with the next available ID and wait for a reply with that
    # id to be received.  Then grab the IQ reply.
    #--------------------------------------------------------------------------
    $iq = $self->SendAndReceiveWithID($iq);

    #--------------------------------------------------------------------------
    # From the reply IQ determine if we were successful or not.  If yes then
    # return "".  If no then return error string from the reply.
    #--------------------------------------------------------------------------
    return unless defined($iq);
    return ( $iq->GetErrorCode() , $iq->GetError() )
        if ($iq->GetType() eq "error");
    return ("ok","");
}


##############################################################################
#
# RosterAdd - Takes the Jabber ID of the user to add to their Roster and
#             sends the IQ packet to the server.
#
##############################################################################
sub RosterAdd
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"set");
    my $roster = $iq->NewChild("jabber:iq:roster");
    my $item = $roster->AddItem();
    $item->SetItem(%args);

    $self->{DEBUG}->Log1("RosterAdd: xml(",$iq->GetXML(),")");
    $self->Send($iq);
}


##############################################################################
#
# RosterAdd - Takes the Jabber ID of the user to remove from their Roster
#             and sends the IQ packet to the server.
#
##############################################################################
sub RosterRemove
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }
    delete($args{subscription});

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"set");
    my $roster = $iq->NewChild("jabber:iq:roster");
    my $item = $roster->AddItem();
    $item->SetItem(%args,
                   subscription=>"remove");
    $self->Send($iq);
}


##############################################################################
#
# RosterParse - Returns a hash of roster items.
#
##############################################################################
sub RosterParse
{
    my $self = shift;
    my($iq) = @_;

    my %roster;
    my $query = $iq->GetChild("jabber:iq:roster");

    if (defined($query)) #$query->GetXMLNS() eq "jabber:iq:roster")
    {
        my @items = $query->GetItems();

        foreach my $item (@items)
        {
            my $jid = $item->GetJID();
            $roster{$jid}->{name} = $item->GetName();
            $roster{$jid}->{subscription} = $item->GetSubscription();
            $roster{$jid}->{ask} = $item->GetAsk();
            $roster{$jid}->{groups} = [ $item->GetGroup() ];
        }
    }

    return %roster;
}


##############################################################################
#
# RosterGet - Sends an empty IQ to the server to request that the user's
#             Roster be sent to them.  Returns a hash of roster items.
#
##############################################################################
sub RosterGet
{
    my $self = shift;

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"get");
    my $query = $iq->NewChild("jabber:iq:roster");

    $iq = $self->SendAndReceiveWithID($iq);

    return unless defined($iq);

    return $self->RosterParse($iq);
}


##############################################################################
#
# RosterRequest - Sends an empty IQ to the server to request that the user's
#                 Roster be sent to them, and return to let the user's program
#                 handle parsing the return packet.
#
##############################################################################
sub RosterRequest
{
    my $self = shift;

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"get");
    my $query = $iq->NewChild("jabber:iq:roster");

    $self->Send($iq);
}


##############################################################################
#
# Roster - Initialize a Net::XMPP::Roster object and return it.
#
##############################################################################
sub Roster
{
    my $self = shift;

    return Net::XMPP::Roster->new(connection=>$self);
}


##############################################################################
#
# RosterDB - initialize the module to use the roster database
#
##############################################################################
sub RosterDB
{
    my $self = shift;

    $self->SetXPathCallBacks('/iq[@type="result" or @type="set"]/query[@xmlns="jabber:iq:roster"]'=>sub{ shift; $self->RosterDBParse(@_) });
}


##############################################################################
#
# RosterDBAdd - adds the entry to the Roster DB.
#
##############################################################################
sub RosterDBAdd
{
    my $self = shift;
    my ($jid,%item) = @_;

    $self->{ROSTERDB}->{JIDS}->{$jid} = \%item;

    foreach my $group (@{$item{groups}})
    {
        $self->{ROSTERDB}->{GROUPS}->{$group}->{$jid} = 1;
    }
}


###############################################################################
#
# RosterDBClear - delete all of the JIDs from the DB completely.
#
###############################################################################
sub RosterDBClear
{
    my $self = shift;

    $self->{DEBUG}->Log1("RosterDBClear: clearing the database");
    foreach my $jid ($self->RosterDBJIDs())
    {
        $self->{DEBUG}->Log3("RosterDBClear: deleting ",$jid->GetJID()," from the DB");
        $self->RosterDBRemove($jid);
    }
    $self->{DEBUG}->Log3("RosterDBClear: database is empty");
}


##############################################################################
#
# RosterDBExists - allows you to query if the JID exists in the Roster DB.
#
##############################################################################
sub RosterDBExists
{
    my $self = shift;
    my ($jid) = @_;

    if ($jid->isa("Net::XMPP::JID"))
    {
        $jid = $jid->GetJID();
    }

    return unless exists($self->{ROSTERDB});
    return unless exists($self->{ROSTERDB}->{JIDS});
    return unless exists($self->{ROSTERDB}->{JIDS}->{$jid});
    return 1;
}


##############################################################################
#
# RosterDBGroupExists - allows you to query if the group exists in the Roster
#                       DB.
#
##############################################################################
sub RosterDBGroupExists
{
    my $self = shift;
    my ($group) = @_;

    return unless exists($self->{ROSTERDB});
    return unless exists($self->{ROSTERDB}->{GROUPS});
    return unless exists($self->{ROSTERDB}->{GROUPS}->{$group});
    return 1;
}


##############################################################################
#
# RosterDBGroupJIDs - returns a list of the current groups in your roster.
#
##############################################################################
sub RosterDBGroupJIDs
{
    my $self = shift;
    my $group = shift;

    return unless $self->RosterDBGroupExists($group);
    my @jids;
    foreach my $jid (keys(%{$self->{ROSTERDB}->{GROUPS}->{$group}}))
    {
        push(@jids,$self->_jid($jid));
    }
    return @jids;
}


##############################################################################
#
# RosterDBGroups - returns a list of the current groups in your roster.
#
##############################################################################
sub RosterDBGroups
{
    my $self = shift;

    return () unless exists($self->{ROSTERDB}->{GROUPS});
    return () if (scalar(keys(%{$self->{ROSTERDB}->{GROUPS}})) == 0);
    return keys(%{$self->{ROSTERDB}->{GROUPS}});
}


##############################################################################
#
# RosterDBJIDs - returns a list of all of the JIDs in your roster.
#
##############################################################################
sub RosterDBJIDs
{
    my $self = shift;
    my $group = shift;

    my @jids;

    return () unless exists($self->{ROSTERDB});
    return () unless exists($self->{ROSTERDB}->{JIDS});
    foreach my $jid (keys(%{$self->{ROSTERDB}->{JIDS}}))
    {
        push(@jids,$self->_jid($jid));
    }
    return @jids;
}


##############################################################################
#
# RosterDBNonGroupJIDs - returns a list of the JIDs not in a group.
#
##############################################################################
sub RosterDBNonGroupJIDs
{
    my $self = shift;
    my $group = shift;

    my @jids;

    return () unless exists($self->{ROSTERDB});
    return () unless exists($self->{ROSTERDB}->{JIDS});
    foreach my $jid (keys(%{$self->{ROSTERDB}->{JIDS}}))
    {
        next if (exists($self->{ROSTERDB}->{JIDS}->{$jid}->{groups}) &&
                 ($#{$self->{ROSTERDB}->{JIDS}->{$jid}->{groups}} > -1));

        push(@jids,$self->_jid($jid));
    }
    return @jids;
}


##############################################################################
#
# RosterDBParse - takes an iq packet that containsa roster, parses it, and puts
#                 the roster into the Roster DB.
#
##############################################################################
sub RosterDBParse
{
    my $self = shift;
    my ($iq) = @_;

    #print "RosterDBParse: iq(",$iq->GetXML(),")\n";

    my $type = $iq->GetType();
    return unless (($type eq "set") || ($type eq "result"));

    my %newroster = $self->RosterParse($iq);

    $self->RosterDBProcessParsed(%newroster);
}


##############################################################################
#
# RosterDBProcessParsed - takes a parsed roster and puts it into the Roster DB.
#
##############################################################################
sub RosterDBProcessParsed
{
    my $self = shift;
    my (%roster) = @_;

    foreach my $jid (keys(%roster))
    {
        $self->RosterDBRemove($jid);

        if ($roster{$jid}->{subscription} ne "remove")
        {
            $self->RosterDBAdd($jid, %{$roster{$jid}} );
        }
    }
}


##############################################################################
#
# RosterDBQuery - allows you to get one of the pieces of info from the
#                 Roster DB.
#
##############################################################################
sub RosterDBQuery
{
    my $self = shift;
    my $jid = shift;
    my $key = shift;

    if ($jid->isa("Net::XMPP::JID"))
    {
        $jid = $jid->GetJID();
    }

    return unless $self->RosterDBExists($jid);
    if (defined($key))
    {
        return unless exists($self->{ROSTERDB}->{JIDS}->{$jid}->{$key});
        return $self->{ROSTERDB}->{JIDS}->{$jid}->{$key};
    }
    return %{$self->{ROSTERDB}->{JIDS}->{$jid}};
}


##############################################################################
#
# RosterDBRemove - removes the JID from the Roster DB.
#
##############################################################################
sub RosterDBRemove
{
    my $self = shift;
    my ($jid) = @_;

    if ($self->RosterDBExists($jid))
    {
        if (defined($self->RosterDBQuery($jid,"groups")))
        {
            foreach my $group (@{$self->RosterDBQuery($jid,"groups")})
            {
                delete($self->{ROSTERDB}->{GROUPS}->{$group}->{$jid});
                delete($self->{ROSTERDB}->{GROUPS}->{$group})
                    if (scalar(keys(%{$self->{ROSTERDB}->{GROUPS}->{$group}})) == 0);
                delete($self->{ROSTERDB}->{GROUPS})
                    if (scalar(keys(%{$self->{ROSTERDB}->{GROUPS}})) == 0);
            }
        }

        delete($self->{ROSTERDB}->{JIDS}->{$jid});
    }
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
# TLSInit - Initialize the connection for TLS.
#
##############################################################################
sub TLSInit
{
    my $self = shift;

    $TLS_CALLBACK = sub{ $self->ProcessTLSStanza( @_ ) };
    $self->SetDirectXPathCallBacks('/[@xmlns="'.&ConstXMLNS("xmpp-tls").'"]'=>$TLS_CALLBACK);
}


##############################################################################
#
# ProcessTLSStanza - process a TLS based packet.
#
##############################################################################
sub ProcessTLSStanza
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    my $tag = &XML::Stream::XPath($node,"name()");

    if ($tag eq "failure")
    {
        $self->TLSClientFailure($node);
    }

    if ($tag eq "proceed")
    {
        $self->TLSClientProceed($node);
    }
}


##############################################################################
#
# TLSStart - client function to have the socket start TLS.
#
##############################################################################
sub TLSStart
{
    my $self = shift;
    my $timeout = shift;
    $timeout = 120 unless defined($timeout);
    $timeout = 120 if ($timeout eq "");

    $self->TLSSendStartTLS();

    my $endTime = time + $timeout;
    while(!$self->TLSClientDone() && ($endTime >= time))
    {
        $self->Process();
    }

    if (!$self->TLSClientSecure())
    {
        return;
    }

    $self->RestartStream($timeout);
}


##############################################################################
#
# TLSClientProceed - handle a <proceed/> packet.
#
##############################################################################
sub TLSClientProceed
{
    my $self = shift;
    my $node = shift;

    my ($status,$message) = $self->{STREAM}->StartTLS($self->GetStreamID());

    if ($status)
    {
        $self->{TLS}->{done} = 1;
        $self->{TLS}->{secure} = 1;
    }
    else
    {
        $self->{TLS}->{done} = 1;
        $self->{TLS}->{error} = $message;
    }

    $self->RemoveDirectXPathCallBacks('/[@xmlns="'.&ConstXMLNS("xmpp-tls").'"]'=>$TLS_CALLBACK);
}


##############################################################################
#
# TLSClientSecure - return 1 if the socket is secure, 0 otherwise.
#
##############################################################################
sub TLSClientSecure
{
    my $self = shift;

    return $self->{TLS}->{secure};
}


##############################################################################
#
# TLSClientDone - return 1 if the TLS process is done
#
##############################################################################
sub TLSClientDone
{
    my $self = shift;

    return $self->{TLS}->{done};
}


##############################################################################
#
# TLSClientError - return the TLS error if any
#
##############################################################################
sub TLSClientError
{
    my $self = shift;

    return $self->{TLS}->{error};
}


##############################################################################
#
# TLSClientFailure - handle a <failure/>
#
##############################################################################
sub TLSClientFailure
{
    my $self = shift;
    my $node = shift;

    my $type = &XML::Stream::XPath($node,"*/name()");

    $self->{TLS}->{error} = $type;
    $self->{TLS}->{done} = 1;
}


##############################################################################
#
# TLSSendFailure - Send a <failure/> in the TLS namespace
#
##############################################################################
sub TLSSendFailure
{
    my $self = shift;
    my $type = shift;

    $self->Send("<failure xmlns='".&ConstXMLNS('xmpp-tls')."'><${type}/></failure>");
}


##############################################################################
#
# TLSSendStartTLS - send a <starttls/> in the TLS namespace.
#
##############################################################################
sub TLSSendStartTLS
{
    my $self = shift;

    $self->Send("<starttls xmlns='".&ConstXMLNS('xmpp-tls')."'/>");
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
# SASLInit - Initialize the connection for SASL.
#
##############################################################################
sub SASLInit
{
    my $self = shift;

    $SASL_CALLBACK = sub{ $self->ProcessSASLStanza( @_ ) };
    $self->SetDirectXPathCallBacks('/[@xmlns="'.&ConstXMLNS("xmpp-sasl").'"]'=> $SASL_CALLBACK);
}


##############################################################################
#
# ProcessSASLStanza - process a SASL based packet.
#
##############################################################################
sub ProcessSASLStanza
{
    my $self = shift;
    my $sid = shift;
    my $node = shift;

    my $tag = &XML::Stream::XPath($node,"name()");

    if ($tag eq "challenge")
    {
        $self->SASLAnswerChallenge($node);
    }

    if ($tag eq "failure")
    {
        $self->SASLClientFailure($node);
    }

    if ($tag eq "success")
    {
        $self->SASLClientSuccess($node);
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
    my $node = shift;

    my $challenge64 = &XML::Stream::XPath($node,"text()");
    my $challenge = MIME::Base64::decode_base64($challenge64);

    my $response = $self->SASLGetClient()->client_step($challenge);

    my $response64 = MIME::Base64::encode_base64($response,"");
    $self->SASLSendResponse($response64);
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
    my $username = shift;
    my $password = shift;

    my $mechanisms = $self->GetStreamFeature("xmpp-sasl");

    return unless defined($mechanisms);

    my $sasl = new Authen::SASL(mechanism=>join(" ",@{$mechanisms}),
                                callback=>{ user => $username,
                                            pass => $password
                                          }
                               );

    $self->{SASL}->{client} = $sasl->client_new();
    $self->{SASL}->{username} = $username;
    $self->{SASL}->{password} = $password;
    $self->{SASL}->{authed} = 0;
    $self->{SASL}->{done} = 0;

    $self->SASLSendAuth();
}


##############################################################################
#
# SASLClientAuthed - return 1 if we authed via SASL, 0 otherwise
#
##############################################################################
sub SASLClientAuthed
{
    my $self = shift;

    return $self->{SASL}->{authed};
}


##############################################################################
#
# SASLClientDone - return 1 if the SASL process is finished
#
##############################################################################
sub SASLClientDone
{
    my $self = shift;

    return $self->{SASL}->{done};
}


##############################################################################
#
# SASLClientError - return the error if any
#
##############################################################################
sub SASLClientError
{
    my $self = shift;

    return $self->{SASL}->{error};
}


##############################################################################
#
# SASLClientFailure - handle a received <failure/>
#
##############################################################################
sub SASLClientFailure
{
    my $self = shift;
    my $node = shift;

    my $type = &XML::Stream::XPath($node,"*/name()");

    $self->{SASL}->{error} = $type;
    $self->{SASL}->{done} = 1;
}


##############################################################################
#
# SASLClientSuccess - handle a received <success/>
#
##############################################################################
sub SASLClientSuccess
{
    my $self = shift;
    my $node = shift;

    $self->{SASL}->{authed} = 1;
    $self->{SASL}->{done} = 1;

    $self->RemoveDirectXPathCallBacks('/[@xmlns="'.&ConstXMLNS("xmpp-sasl").'"]'=>$SASL_CALLBACK);
}


###############################################################################
#
# SASLGetClient - This is a helper function to return the SASL client object.
#
###############################################################################
sub SASLGetClient
{
    my $self = shift;

    return $self->{SASL}->{client};
}


##############################################################################
#
# SASLSendAuth - send an <auth/> in the SASL namespace
#
##############################################################################
sub SASLSendAuth
{
    my $self = shift;

    $self->Send("<auth xmlns='".&ConstXMLNS('xmpp-sasl')."' mechanism='".$self->SASLGetClient()->mechanism()."'/>");
}


##############################################################################
#
# SASLSendChallenge - Send a <challenge/> in the SASL namespace
#
##############################################################################
sub SASLSendChallenge
{
    my $self = shift;
    my $challenge = shift;

    $self->Send("<challenge xmlns='".&ConstXMLNS('xmpp-sasl')."'>${challenge}</challenge>");
}


##############################################################################
#
# SASLSendFailure - Send a <failure/> tag in the SASL namespace
#
##############################################################################
sub SASLSendFailure
{
    my $self = shift;
    my $type = shift;

    $self->Send("<failure xmlns='".&ConstXMLNS('xmpp-sasl')."'><${type}/></failure>");
}


##############################################################################
#
# SASLSendResponse - Send a <response/> tag in the SASL namespace
#
##############################################################################
sub SASLSendResponse
{
    my $self = shift;
    my $response = shift;

    $self->Send("<response xmlns='".&ConstXMLNS('xmpp-sasl')."'>${response}</response>");
}




##############################################################################
#+----------------------------------------------------------------------------
#|
#| Default CallBacks
#|
#+----------------------------------------------------------------------------
##############################################################################


##############################################################################
#
# callbackInit - initialize the default callbacks
#
##############################################################################
sub callbackInit
{
    my $self = shift;

    $self->SetCallBacks(iq=>sub{ $self->callbackIQ(@_) },
                        presence=>sub{ $self->callbackPresence(@_) },
                        message=>sub{ $self->callbackMessage(@_) },
                        );

    $self->SetPresenceCallBacks(subscribe=>sub{ $self->callbackPresenceSubscribe(@_) },
                                unsubscribe=>sub{ $self->callbackPresenceUnsubscribe(@_) },
                                subscribed=>sub{ $self->callbackPresenceSubscribed(@_) },
                                unsubscribed=>sub{ $self->callbackPresenceUnsubscribed(@_) },
                               );

    $self->TLSInit();
    $self->SASLInit();
}


##############################################################################
#
# callbackMessage - default callback for <message/> packets.
#
##############################################################################
sub callbackMessage
{
    my $self = shift;
    my $sid = shift;
    my $message = shift;

    my $type = "normal";
    $type = $message->GetType() if $message->DefinedType();

    if (exists($self->{CB}->{Mess}->{$type}) &&
        (ref($self->{CB}->{Mess}->{$type}) eq "CODE"))
    {
        &{$self->{CB}->{Mess}->{$type}}($sid,$message);
    }
}


##############################################################################
#
# callbackPresence - default callback for <presence/> packets.
#
##############################################################################
sub callbackPresence
{
    my $self = shift;
    my $sid = shift;
    my $presence = shift;

    my $type = "available";
    $type = $presence->GetType() if $presence->DefinedType();

    if (exists($self->{CB}->{Pres}->{$type}) &&
        (ref($self->{CB}->{Pres}->{$type}) eq "CODE"))
    {
        &{$self->{CB}->{Pres}->{$type}}($sid,$presence);
    }
}


##############################################################################
#
# callbackIQ - default callback for <iq/> packets.
#
##############################################################################
sub callbackIQ
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    return unless $iq->DefinedChild();
    my $query = $iq->GetChild();
    return unless defined($query);

    my $type = $iq->GetType();
    my $ns = $query->GetXMLNS();

    if (exists($self->{CB}->{IQns}->{$ns}) &&
        (ref($self->{CB}->{IQns}->{$ns}) eq "CODE"))
    {
        &{$self->{CB}->{IQns}->{$ns}}($sid,$iq);

    }
    elsif (exists($self->{CB}->{IQns}->{$ns}->{$type}) &&
           (ref($self->{CB}->{IQns}->{$ns}->{$type}) eq "CODE"))
    {
        &{$self->{CB}->{IQns}->{$ns}->{$type}}($sid,$iq);
    }
}


##############################################################################
#
# callbackPresenceSubscribe - default callback for subscribe packets.
#
##############################################################################
sub callbackPresenceSubscribe
{
    my $self = shift;
    my $sid = shift;
    my $presence = shift;

    my $reply = $presence->Reply(type=>"subscribed");
    $self->Send($reply,1);
    $reply->SetType("subscribe");
    $self->Send($reply,1);
}


##############################################################################
#
# callbackPresenceUnsubscribe - default callback for unsubscribe packets.
#
##############################################################################
sub callbackPresenceUnsubscribe
{
    my $self = shift;
    my $sid = shift;
    my $presence = shift;

    my $reply = $presence->Reply(type=>"unsubscribed");
    $self->Send($reply,1);
}


##############################################################################
#
# callbackPresenceSubscribed - default callback for subscribed packets.
#
##############################################################################
sub callbackPresenceSubscribed
{
    my $self = shift;
    my $sid = shift;
    my $presence = shift;

    my $reply = $presence->Reply(type=>"subscribed");
    $self->Send($reply,1);
}


##############################################################################
#
# callbackPresenceUnsubscribed - default callback for unsubscribed packets.
#
##############################################################################
sub callbackPresenceUnsubscribed
{
    my $self = shift;
    my $sid = shift;
    my $presence = shift;

    my $reply = $presence->Reply(type=>"unsubscribed");
    $self->Send($reply,1);
}



##############################################################################
#+----------------------------------------------------------------------------
#|
#| Stream functions
#|
#+----------------------------------------------------------------------------
##############################################################################
sub GetStreamID
{
    my $self = shift;

    return $self->{SESSION}->{id};
}


sub GetStreamFeature
{
    my $self = shift;
    my $feature = shift;

    return $self->{STREAM}->GetStreamFeature($self->GetStreamID(),$feature);
}


sub RestartStream
{
    my $self = shift;
    my $timeout = shift;

    $self->{SESSION} =
        $self->{STREAM}->OpenStream($self->GetStreamID(),$timeout);
    return $self->GetStreamID();
}


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


1;
