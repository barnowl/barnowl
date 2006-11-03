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

package Net::Jabber::Protocol;

=head1 NAME

Net::Jabber::Protocol - Jabber Protocol Library

=head1 SYNOPSIS

  Net::Jabber::Protocol is a module that provides a developer easy
  access to the Jabber Instant Messaging protocol.  It provides high
  level functions to the Net::Jabber Client, Component, and Server
  objects.  These functions are automatically indluded in those modules
  through AUTOLOAD and delegates.

=head1 DESCRIPTION

  Protocol.pm seeks to provide enough high level APIs and automation of
  the low level APIs that writing a Jabber Client/Transport in Perl is
  trivial.  For those that wish to work with the low level you can do
  that too, but those functions are covered in the documentation for
  each module.

  Net::Jabber::Protocol provides functions to login, send and receive
  messages, set personal information, create a new user account, manage
  the roster, and disconnect.  You can use all or none of the functions,
  there is no requirement.

  For more information on how the details for how Net::Jabber is written
  please see the help for Net::Jabber itself.

  For more information on writing a Client see Net::Jabber::Client.

  For more information on writing a Transport see Net::Jabber::Transport.

=head2 Modes

  Several of the functions take a mode argument that let you specify how
  the function should behave:

    block - send the packet with an ID, and then block until an answer
            comes back.  You can optionally specify a timeout so that
            you do not block forever.
           
    nonblock - send the packet with an ID, but then return that id and
               control to the master program.  Net::Jabber is still
               tracking this packet, so you must use the CheckID function
               to tell when it comes in.  (This might not be very
               useful...)

    passthru - send the packet with an ID, but do NOT register it with
               Net::Jabber, then return the ID.  This is useful when
               combined with the XPath function because you can register
               a one shot function tied to the id you get back.
               

=head2 Basic Functions

    use Net::Jabber qw( Client );
    $Con = new Net::Jabber::Client();                # From
    $status = $Con->Connect(hostname=>"jabber.org"); # Net::Jabber::Client

      or

    use Net::Jabber qw( Component );
    $Con = new Net::Jabber::Component();             #
    $status = $Con->Connect(hostname=>"jabber.org",  # From
                            secret=>"bob");          # Net::Jabber::Component


    #
    # For callback setup, see Net::XMPP::Protocol
    #

    $Con->Info(name=>"Jarl",
               version=>"v0.6000");

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

=head2 IQ  Functions

=head2 Agents Functions

    %agents = $Con->AgentsGet();
    %agents = $Con->AgentsGet(to=>"transport.jabber.org");

=head2 Browse Functions

    %hash = $Con->BrowseRequest(jid=>"jabber.org");
    %hash = $Con->BrowseRequest(jid=>"jabber.org",
                                timeout=>10);

    $id = $Con->BrowseRequest(jid=>"jabber.org",
                              mode=>"nonblock");

    $id = $Con->BrowseRequest(jid=>"jabber.org",
                              mode=>"passthru");

=head2 Browse DB Functions

    $Con->BrowseDBDelete("jabber.org");
    $Con->BrowseDBDelete(Net::Jabber::JID);

    $presence  = $Con->BrowseDBQuery(jid=>"bob\@jabber.org");
    $presence  = $Con->BrowseDBQuery(jid=>Net::Jabber::JID);
    $presence  = $Con->BrowseDBQuery(jid=>"users.jabber.org",
                                     timeout=>10);
    $presence  = $Con->BrowseDBQuery(jid=>"conference.jabber.org",
                                     refresh=>1);

=head2 Bystreams Functions

    %hash = $Con->ByteStreamsProxyRequest(jid=>"proxy.server"); 
    %hash = $Con->ByteStreamsProxyRequest(jid=>"proxy.server",
                                          timeout=>10); 

    $id = $Con->ByteStreamsProxyRequest(jid=>"proxy.server",
                                        mode=>"nonblock");

    $id = $Con->ByteStreamsProxyRequest(jid=>"proxy.server",
                                        mode=>"passthru");

    
    %hash = $Con->ByteStreamsProxyParse($query);

    
    $status = $Con->ByteStreamsProxyActivate(sid=>"stream_id",
                                             jid=>"proxy.server"); 
    $status = $Con->ByteStreamsProxyActivate(sid=>"stream_id",
                                             jid=>"proxy.server",
                                            timeout=>10); 

    $id = $Con->ByteStreamsProxyActivate(sid=>"stream_id",
                                         jid=>"proxy.server",
                                        mode=>"nonblock");

    $id = $Con->ByteStreamsProxyActivate(sid=>"stream_id",
                                         jid=>"proxy.server",
                                        mode=>"passthru"); 


    $jid = $Con->ByteStreamsOffer(sid=>"stream_id",
                                  streamhosts=>[{jid=>"jid",
                                                 host=>"host",
                                                 port=>"port",
                                                 zeroconf=>"zero",
                                                },
                                                ...
                                               ],
                                  jid=>"bob\@jabber.org"); 
    $jid = $Con->ByteStreamsOffer(sid=>"stream_id",
                                  streamhosts=>[{},{},...],
                                  jid=>"bob\@jabber.org",
                                  timeout=>10); 

    $id = $Con->ByteStreamsOffer(sid=>"stream_id",
                                 streamhosts=>[{},{},...],
                                 jid=>"bob\@jabber.org",
                                 mode=>"nonblock");

    $id = $Con->ByteStreamsOffer(sid=>"stream_id",
                                 streamhosts=>[{},{},...],
                                 jid=>"bob\@jabber.org",
                                 mode=>"passthru");
 
=head2 Disco Functions

    %hash = $Con->DiscoInfoRequest(jid=>"jabber.org");
    %hash = $Con->DiscoInfoRequest(jid=>"jabber.org",
                                   node=>"node...");
    %hash = $Con->DiscoInfoRequest(jid=>"jabber.org",
                                   node=>"node...",
                                   timeout=>10);

    $id = $Con->DiscoInfoRequest(jid=>"jabber.org",
                                 mode=>"nonblock");
    $id = $Con->DiscoInfoRequest(jid=>"jabber.org",
                                 node=>"node...",
                                 mode=>"nonblock");

    $id = $Con->DiscoInfoRequest(jid=>"jabber.org",
                                 mode=>"passthru");
    $id = $Con->DiscoInfoRequest(jid=>"jabber.org",
                                 node=>"node...",
                                 mode=>"passthru");

    
    %hash = $Con->DiscoInfoParse($query);


    %hash = $Con->DiscoItemsRequest(jid=>"jabber.org");
    %hash = $Con->DiscoItemsRequest(jid=>"jabber.org",
                                    timeout=>10);

    $id = $Con->DiscoItemsRequest(jid=>"jabber.org",
                                  mode=>"nonblock");

    $id = $Con->DiscoItemsRequest(jid=>"jabber.org",
                                  mode=>"passthru");

    
    %hash = $Con->DiscoItemsParse($query);

=head2 Feature Negotiation Functions

  
    %hash = $Con->FeatureNegRequest(jid=>"jabber.org",
                                    features=>{ feat1=>["opt1","opt2",...],
                                                feat2=>["optA","optB",...]
                                              }
                                   );
    %hash = $Con->FeatureNegRequest(jid=>"jabber.org",
                                    features=>{ ... },
                                    timeout=>10);

    $id = $Con->FeatureNegRequest(jid=>"jabber.org",
                                  features=>{ ... },
                                  mode=>"nonblock");

    $id = $Con->FeatureNegRequest(jid=>"jabber.org",
                                  features=>{ ... },
                                  mode=>"passthru");

    my $query = $self->FeatureNegQuery(\{ ... });
    $iq->AddQuery($query);

    %hash = $Con->FeatureNegParse($query);  

=head2 File Transfer Functions

    $method = $Con->FileTransferOffer(jid=>"bob\@jabber.org",
                                      sid=>"stream_id",
                                      filename=>"/path/to/file",
                                      methods=>["http://jabber.org/protocol/si/profile/bytestreams",
                                                "jabber:iq:oob",
                                                ...
                                               ]
                                     );
    $method = $Con->FileTransferOffer(jid=>"bob\@jabber.org",
                                      sid=>"stream_id",
                                      filename=>"/path/to/file",
                                      methods=>\@methods,
                                      timeout=>"10");

    $id = $Con->FileTransferOffer(jid=>"bob\@jabber.org",
                                  sid=>"stream_id",
                                  filename=>"/path/to/file",
                                  methods=>\@methods,
                                  mode=>"nonblock");

    $id = $Con->FileTransferOffer(jid=>"bob\@jabber.org",
                                  sid=>"stream_id",
                                  filename=>"/path/to/file",
                                  methods=>\@methods,
                                  mode=>"passthru");

=head2 Last Functions

    $Con->LastQuery();
    $Con->LastQuery(to=>"bob@jabber.org");

    %result = $Con->LastQuery(mode=>"block");
    %result = $Con->LastQuery(to=>"bob@jabber.org",
                              mode=>"block");

    %result = $Con->LastQuery(to=>"bob@jabber.org",
                              mode=>"block",
                              timeout=>10);
    %result = $Con->LastQuery(mode=>"block",
                              timeout=>10);

    $Con->LastSend(to=>"bob@jabber.org");

    $seconds = $Con->LastActivity();

=head2 Multi-User Chat Functions

    $Con->MUCJoin(room=>"jabber",
                  server=>"conference.jabber.org",
                  nick=>"nick");

    $Con->MUCJoin(room=>"jabber",
                  server=>"conference.jabber.org",
                  nick=>"nick",
                  password=>"secret");

=head2 Register Functions

    @result = $Con->RegisterSendData("users.jabber.org",
                                     first=>"Bob",
                                     last=>"Smith",
                                     nick=>"bob",
                                     email=>"foo@bar.net");


=head2 RPC Functions

    $query = $Con->RPCEncode(type=>"methodCall",
                             methodName=>"methodName",
                             params=>[param,param,...]);
    $query = $Con->RPCEncode(type=>"methodResponse",
                             params=>[param,param,...]);
    $query = $Con->RPCEncode(type=>"methodResponse",
                             faultCode=>4,
                             faultString=>"Too many params");

    @response = $Con->RPCParse($iq);

    @response = $Con->RPCCall(to=>"dataHouse.jabber.org",
                              methodname=>"numUsers",
                              params=>[ param,param,... ]
                             );

    $Con->RPCResponse(to=>"you\@jabber.org",
                      params=>[ param,param,... ]);

    $Con->RPCResponse(to=>"you\@jabber.org",
                      faultCode=>"4",
                      faultString=>"Too many parameters"
                     );

    $Con->RPCSetCallBacks(myMethodA=>\&methoda,
                          myMethodB=>\&do_somthing,
                          etc...
                         );

=head2 Search Functions

    %fields = $Con->SearchRequest();
    %fields = $Con->SearchRequest(to=>"users.jabber.org");
    %fields = $Con->SearchRequest(to=>"users.jabber.org",
                                  timeout=>10);

    $Con->SearchSend(to=>"somewhere",
                     name=>"",
                     first=>"Bob",
                     last=>"",
                     nick=>"bob",
                     email=>"",
                     key=>"some key");

    $Con->SearchSendData("users.jabber.org",
                         first=>"Bob",
                         last=>"",
                         nick=>"bob",
                         email=>"");

=head2 Time Functions

    $Con->TimeQuery();
    $Con->TimeQuery(to=>"bob@jabber.org");

    %result = $Con->TimeQuery(mode=>"block");
    %result = $Con->TimeQuery(to=>"bob@jabber.org",
                              mode=>"block");

    $Con->TimeSend(to=>"bob@jabber.org");

=head2 Version Functions

    $Con->VersionQuery();
    $Con->VersionQuery(to=>"bob@jabber.org");

    %result = $Con->VersionQuery(mode=>"block");
    %result = $Con->VersionQuery(to=>"bob@jabber.org",
                                 mode=>"block");

    $Con->VersionSend(to=>"bob@jabber.org",
                      name=>"Net::Jabber",
                      ver=>"1.0a",
                      os=>"Perl");

=head1 METHODS

=head2 Basic Functions

    Info(name=>string,    - Set some information so that Net::Jabber
         version=>string)   can auto-reply to some packets for you to
                            reduce the work you have to do.

                            NOTE: This requires that you use the
                            SetIQCallBacks methodology and not the
                            SetCallBacks for <iq/> packets.

=head2 IQ Functions

=head2 Agents Functions

    ********************************
    *                              *
    * Deprecated in favor of Disco *
    *                              *
    ********************************

    AgentsGet(to=>string, - takes all of the information and
    AgentsGet()             builds a Net::Jabber::IQ::Agents packet.
                            It then sends that packet either to the
                            server, or to the specified transport,
                            with an ID and waits for that ID to return.
                            Then it looks in the resulting packet and
                            builds a hash that contains the values
                            of the agent list.  The hash is layed out
                            like this:  (NOTE: the jid is the key to
                            distinguish the various agents)

                              $hash{<JID>}->{order} = 4
                                          ->{name} = "ICQ Transport"
                                          ->{transport} = "ICQ #"
                                          ->{description} = "ICQ..blah.."
                                          ->{service} = "icq"
                                          ->{register} = 1
                                          ->{search} = 1
                                        etc...

                            The order field determines the order that
                            it came from the server in... in case you
                            care.  For more info on the valid fields
                            see the Net::Jabber::Query jabber:iq:agent
                            namespace.

=head2 Browse Functions

    ********************************
    *                              *
    * Deprecated in favor of Disco *
    *                              *
    ********************************

    BrowseRequest(jid=>string, - sends a jabber:iq:browse request to
                  mode=>string,  the jid passed as an argument.
                  timeout=>int)  Returns a hash with the resulting
                                 tree if mode is set to "block":

                $browse{'category'} = "conference"
                $browse{'children'}->[0]
                $browse{'children'}->[1]
                $browse{'children'}->[11]
                $browse{'jid'} = "conference.jabber.org"
                $browse{'name'} = "Jabber.org Conferencing Center"
                $browse{'ns'}->[0]
                $browse{'ns'}->[1]
                $browse{'type'} = "public"

                                 The ns array is an array of the
                                 namespaces that this jid supports.
                                 The children array points to hashs
                                 of this form, and represent the fact
                                 that they can be browsed to.

                                 See MODES above for using the mode
                                 and timeout.

=head2 Browse DB Functions

    BrowseDBDelete(string|Net::Jabber::JID) - delete thes JID browse
                                              data from the DB.

    BrowseDBQuery(jid=>string | NJ::JID, - returns the browse data
                  timeout=>integer,        for the requested JID.  If
                  refresh=>0|1)            the DB does not contain
                                           the data for the JID, then
                                           it attempts to fetch the
                                           data via BrowseRequest().
                                           The timeout is passed to
                                           the BrowseRequest() call,
                                           and refresh tells the DB
                                           to request the data, even
                                           if it already has some.

=head2 Bytestreams Functions

    ByteStreamsProxyRequest(jid=>string, - sends a bytestreams request
                            mode=>string,  to the jid passed as an
                            timeout=>int)  argument.  Returns an array
                                           ref with the resulting tree
                                           if mode is set to "block".

                                           See ByteStreamsProxyParse
                                           for the format of the
                                           resulting tree.

                                           See MODES above for using
                                           the mode and timeout.

    ByteStreamsProxyParse(Net::Jabber::Query) - parses the query and
                                                returns an array ref
                                                to the resulting tree:

                $host[0]->{jid} = "bytestreams1.proxy.server";
                $host[0]->{host} = "proxy1.server";
                $host[0]->{port} = "5006";
                $host[1]->{jid} = "bytestreams2.proxy.server";
                $host[1]->{host} = "proxy2.server";
                $host[1]->{port} = "5007";
                ...

    ByteStreamsProxyActivate(jid=>string, - sends a bytestreams activate
                             sid=>string,   to the jid passed as an
                             mode=>string,  argument.  Returns 1 if the
                             timeout=>int)  proxy activated (undef if
                                            it did not) if mode is set
                                            to "block".

                                            sid is the stream id that
                                            is being used to talk about
                                            this stream.

                                            See MODES above for using
                                            the mode and timeout.

    ByteStreamsOffer(jid=>string,         - sends a bytestreams offer
                     sid=>string,           to the jid passed as an
                     streamhosts=>arrayref  argument.  Returns the jid
                     mode=>string,          of the streamhost that the
                     timeout=>int)          user selected if mode is set
                                            to "block".

                                            streamhosts is the same
                                            format as the array ref
                                            returned from
                                            ByteStreamsProxyParse.

                                            See MODES above for using
                                            the mode and timeout.

=head2 Disco Functions

    DiscoInfoRequest(jid=>string, - sends a disco#info request to
                     node=>string,  the jid passed as an argument,
                     mode=>string,  and the node if specified.
                     timeout=>int)  Returns a hash with the resulting
                                    tree if mode is set to "block".

                                    See DiscoInfoParse for the format
                                    of the resulting tree.
                                    
                                    See MODES above for using the mode
                                    and timeout.

    DiscoInfoParse(Net::Jabber::Query) - parses the query and
                                         returns a hash ref
                                         to the resulting tree:

             $info{identity}->[0]->{category} = "groupchat";
             $info{identity}->[0]->{name} = "Public Chatrooms";
             $info{identity}->[0]->{type} = "public";

             $info{identity}->[1]->{category} = "groupchat";
             $info{identity}->[1]->{name} = "Private Chatrooms";
             $info{identity}->[1]->{type} = "private";

             $info{feature}->{http://jabber.org/protocol/disco#info} = 1;
             $info{feature}->{http://jabber.org/protocol/muc#admin} = 1;
                                    
    DiscoItemsRequest(jid=>string, - sends a disco#items request to
                      mode=>string,  the jid passed as an argument.
                      timeout=>int)  Returns a hash with the resulting
                                     tree if mode is set to "block".

                                     See DiscoItemsParse for the format
                                     of the resulting tree.
                                    
                                     See MODES above for using the mode
                                     and timeout.

    DiscoItemsParse(Net::Jabber::Query) - parses the query and
                                          returns a hash ref
                                          to the resulting tree:

             $items{jid}->{node} = name;

             $items{"proxy.server"}->{""} = "Bytestream Proxy Server";
             $items{"conf.server"}->{"public"} = "Public Chatrooms";
             $items{"conf.server"}->{"private"} = "Private Chatrooms";

=head2 Feature Negotiation Functions

    FeatureNegRequest(jid=>string,       - sends a feature negotiation to
                      features=>hash ref,  the jid passed as an argument,
                      mode=>string,        using the features specified.
                      timeout=>int)        Returns a hash with the resulting
                                           tree if mode is set to "block".

                                           See DiscoInfoQuery for the format
                                           of the features hash ref.
                                    
                                           See DiscoInfoParse for the format
                                           of the resulting tree.
                                    
                                           See MODES above for using the mode
                                           and timeout.

    FeatureNegParse(Net::Jabber::Query) - parses the query and
                                          returns a hash ref
                                          to the resulting tree:

             $features->{feat1} = ["opt1","opt2",...];
             $features->{feat2} = ["optA","optB",...];
             ....

                                          If this is a result:

             $features->{feat1} = "opt2";
             $features->{feat2} = "optA";
             ....

    FeatureNeqQuery(hash ref) - takes a hash ref and turns it into a
                                feature negotiation query that you can
                                AddQuery into your packaet.  The format
                                of the hash ref is as follows:

             $features->{feat1} = ["opt1","opt2",...];
             $features->{feat2} = ["optA","optB",...];
             ....

=head2 File Transfer Functions

    FileTransferOffer(jid=>string,         - sends a file transfer stream
                      sid=>string,           initiation to the jid passed
                      filename=>string,      as an argument.  Returns the
                      mode=>string,          method (if the users accepts),
                      timeout=>int)          undef (if the user declines),
                                             if the mode is set to "block".

                                             See MODES above for using
                                             the mode and timeout.

=head2 Last Functions

    LastQuery(to=>string,     - asks the jid specified for its last
              mode=>string,     activity.  If the to is blank, then it
              timeout=>int)     queries the server.  Returns a hash with
    LastQuery()                 the various items set if mode is set to
                                "block":

                                  $last{seconds} - Seconds since activity
                                  $last{message} - Message for activity

                                See MODES above for using the mode
                                and timeout.

    LastSend(to=>string, - sends the specified last to the specified jid.
             hash)         the hash is the seconds and message as shown
                           in the Net::Jabber::Query man page.

    LastActivity() - returns the number of seconds since the last activity
                     by the user.

=head2 Multi-User Chat Functions

    MUCJoin(room=>string,    - Sends the appropriate MUC protocol to join
            server=>string,    the specified room with the specified nick.
            nick=>string,
            password=>string)

=head2 Register Functions

    RegisterSendData(string|JID, - takes the contents of the hash and
                     hash)         builds a jabebr:x:data return packet
                                   which it sends in a Net::Jabber::Query
                                   jabber:iq:register namespace packet.
                                   The first argument is the JID to send
                                   the packet to.  This function returns
                                   an array that looks like this:

                                     [ type , message ]

                                   If type is "ok" then registration was
                                   successful, otherwise message contains
                                   a little more detail about the error.

=head2 RPC Functions

    RPCParse(IQ object) - returns an array.  The first argument tells
                          the status "ok" or "fault".  The second
                          argument is an array if "ok", or a hash if
                          "fault".

    RPCCall(to=>jid|string,     - takes the methodName and params,
            methodName=>string,   builds the RPC calls and sends it
            params=>array,        to the specified address.  Returns
            mode=>string,         the above data from RPCParse.
            timeout=>int)         
                                  See MODES above for using the mode
                                  and timeout.

    RPCResponse(to=>jid|string,      - generates a response back to
                params=>array,         the caller.  If any part of
                faultCode=>int,        fault is specified, then it
                faultString=>string)   wins.


    Note: To ensure that you get the correct type for a param sent
          back, you can specify the type by prepending the type to
          the value:

            "i4:5" or "int:5"
            "boolean:0"
            "string:56"
            "double:5.0"
            "datetime:20020415T11:11:11"
            "base64:...."

    RPCSetCallBacks(method=>function, - sets the callback functions
                    method=>function,   for the specified methods.
                    etc...)             The method comes from the
                                        <methodName/> and is case
                                        sensitive.  The single
                                        arguemnt is a ref to an
                                        array that contains the
                                        <params/>.  The function you
                                        write should return one of two
                                        things:

                                          ["ok", [...] ]

                                        The [...] is a list of the
                                        <params/> you want to return.

                                          ["fault", {faultCode=>1,
                                                     faultString=>...} ]

                                        If you set the function to undef,
                                        then the method is removed from
                                        the list.

=head2 Search Functions

    SearchRequest(to=>string,  - send an <iq/> request to the specified
                  mode=>string,  server/transport, if not specified it
                  timeout=>int)  sends to the current active server.
    SearchRequest()              The function returns a hash that
                                 contains the required fields.   Here
                                 is an example of the hash:

                                 $hash{fields}    - The raw fields from
                                                    the iq:register.  To
                                                    be used if there is
                                                    no x:data in the
                                                    packet.
                                 $hash{instructions} - How to fill out
                                                       the form.
                                 $hash{form}   - The new dynamic forms.

                                 In $hash{form}, the fields that are
                                 present are the required fields the
                                 server needs.
                                
                                 See MODES above for using the mode
                                 and timeout.

    SearchSend(to=>string|JID, - takes the contents of the hash and
               hash)             passes it to the SetSearch function
                                 in the Net::Jabber::Query
                                 jabber:iq:search namespace.  And then
                                 sends the packet.

    SearchSendData(string|JID, - takes the contents of the hash and
                   hash)         builds a jabebr:x:data return packet
                                 which it sends in a Net::Jabber::Query
                                 jabber:iq:search namespace packet.
                                 The first argument is the JID to send
                                 the packet to.

=head2 Time Functions

    TimeQuery(to=>string,     - asks the jid specified for its localtime.
              mode=>string,     If the to is blank, then it queries the
              timeout=>int)     server.  Returns a hash with the various
    TimeQuery()                 items set if mode is set to "block":

                                  $time{utc}     - Time in UTC
                                  $time{tz}      - Timezone
                                  $time{display} - Display string

                                See MODES above for using the mode
                                and timeout.

    TimeSend(to=>string) - sends the current UTC time to the specified
                           jid.

=head2 Version Functions

    VersionQuery(to=>string,     - asks the jid specified for its
                 mode=>string,     client version information.  If the
                 timeout=>int)     to is blank, then it queries the
    VersionQuery()                 server.  Returns ahash with the
                                   various items set if mode is set to
                                   "block":

                                     $version{name} - Name
                                     $version{ver}  - Version
                                     $version{os}   - Operating System/
                                                        Platform

                                  See MODES above for using the mode
                                  and timeout.

    VersionSend(to=>string,   - sends the specified version information
                name=>string,   to the jid specified in the to.
                ver=>string,
                os=>string)

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use Carp;
use vars qw($VERSION);

$VERSION = "2.0";

##############################################################################
# BuildObject takes a root tag and builds the correct object.  NEWOBJECT is
# the table that maps tag to package.  Override these, or provide new ones.
#-----------------------------------------------------------------------------
$Net::XMPP::Protocol::NEWOBJECT{'iq'}       = "Net::Jabber::IQ";
$Net::XMPP::Protocol::NEWOBJECT{'message'}  = "Net::Jabber::Message";
$Net::XMPP::Protocol::NEWOBJECT{'presence'} = "Net::Jabber::Presence";
$Net::XMPP::Protocol::NEWOBJECT{'jid'}      = "Net::Jabber::JID";
##############################################################################

###############################################################################
#+-----------------------------------------------------------------------------
#|
#| Base API
#|
#+-----------------------------------------------------------------------------
###############################################################################

###############################################################################
#
# Info - set the base information about this Jabber Client/Component for
#        use in a default response.
#
###############################################################################
sub Info
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    foreach my $arg (keys(%args))
    {
        $self->{INFO}->{$arg} = $args{$arg};
    }
}


###############################################################################
#
# DefineNamespace - adds the namespace and corresponding functions onto the
#                   of available functions based on namespace.
#
#     Deprecated in favor of AddNamespace
#
###############################################################################
sub DefineNamespace
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    croak("You must specify xmlns=>'' for the function call to DefineNamespace")
        if !exists($args{xmlns});
    croak("You must specify type=>'' for the function call to DefineNamespace")
        if !exists($args{type});
    croak("You must specify functions=>'' for the function call to DefineNamespace")
        if !exists($args{functions});
    
    my %xpath;

    my $tag;
    if (exists($args{tag}))
    {
        $tag = $args{tag};
    }
    else
    {
        $tag = "x" if ($args{type} eq "X");
        $tag = "query" if ($args{type} eq "Query");
    }

    foreach my $function (@{$args{functions}})
    {
        my %tempHash = %{$function};
        my %funcHash;
        foreach my $func (keys(%tempHash))
        {
            $funcHash{lc($func)} = $tempHash{$func};
        }

        croak("You must specify name=>'' for each function in call to DefineNamespace")
            if !exists($funcHash{name});

        my $name = delete($funcHash{name});

        if (!exists($funcHash{set}) && exists($funcHash{get}))
        {
            croak("The DefineNamespace arugments have changed, and I cannot determine the\nnew values automatically for name($name).  Please read the man page\nfor Net::Jabber::Namespaces.  I apologize for this incompatability.\n");
        }

        if (exists($funcHash{type}) || exists($funcHash{path}) ||
            exists($funcHash{child}) || exists($funcHash{calls}))
        {

            foreach my $type (keys(%funcHash))
            {
                if ($type eq "child")
                {
                    $xpath{$name}->{$type}->{ns} = $funcHash{$type}->[1];
                    my $i = 2;
                    while( $i <= $#{$funcHash{$type}} )
                    {
                        if ($funcHash{$type}->[$i] eq "__netjabber__:skip_xmlns")
                        {
                            $xpath{$name}->{$type}->{skip_xmlns} = 1;
                        }
                        
                        if ($funcHash{$type}->[$i] eq "__netjabber__:specifyname")
                        {
                            $xpath{$name}->{$type}->{specify_name} = 1;
                            $i++;
                            $xpath{$name}->{$type}->{tag} = $funcHash{$type}->[$i+1];
                        }

                        $i++;
                    }
                }
                else
                {
                    $xpath{$name}->{$type} = $funcHash{$type};
                }
            }
            next;
        }
        
        my $type = $funcHash{set}->[0];
        my $xpath = $funcHash{set}->[1];
        if (exists($funcHash{hash}))
        {
            $xpath = "text()" if ($funcHash{hash} eq "data");
            $xpath .= "/text()" if ($funcHash{hash} eq "child-data");
            $xpath = "\@$xpath" if ($funcHash{hash} eq "att");
            $xpath = "$1/\@$2" if ($funcHash{hash} =~ /^att-(\S+)-(.+)$/);
        }

        if ($type eq "master")
        {
            $xpath{$name}->{type} = $type;
            next;
        }
        
        if ($type eq "scalar")
        {
            $xpath{$name}->{path} = $xpath;
            next;
        }
        
        if ($type eq "flag")
        {
            $xpath{$name}->{type} = 'flag';
            $xpath{$name}->{path} = $xpath;
            next;
        }

        if (($funcHash{hash} eq "child-add") && exists($funcHash{add}))
        {
            $xpath{$name}->{type} = "node";
            $xpath{$name}->{path} = $funcHash{add}->[3];
            $xpath{$name}->{child}->{ns} = $funcHash{add}->[1];
            $xpath{$name}->{calls} = [ 'Add' ];
            next;
        }
    }

    $self->AddNamespace(ns => $args{xmlns},
                        tag => $tag,
                        xpath => \%xpath );
}

###############################################################################
#
# AgentsGet - Sends an empty IQ to the server/transport to request that the
#             list of supported Agents be sent to them.  Returns a hash
#             containing the values for the agents.
#
###############################################################################
sub AgentsGet
{
    my $self = shift;

    my $iq = $self->_iq();
    $iq->SetIQ(@_);
    $iq->SetIQ(type=>"get");
    my $query = $iq->NewQuery("jabber:iq:agents");

    $iq = $self->SendAndReceiveWithID($iq);

    return unless defined($iq);

    $query = $iq->GetQuery();
    my @agents = $query->GetAgents();

    my %agents;
    my $count = 0;
    foreach my $agent (@agents)
    {
        my $jid = $agent->GetJID();
        $agents{$jid}->{name} = $agent->GetName();
        $agents{$jid}->{description} = $agent->GetDescription();
        $agents{$jid}->{transport} = $agent->GetTransport();
        $agents{$jid}->{service} = $agent->GetService();
        $agents{$jid}->{register} = $agent->DefinedRegister();
        $agents{$jid}->{search} = $agent->DefinedSearch();
        $agents{$jid}->{groupchat} = $agent->DefinedGroupChat();
        $agents{$jid}->{agents} = $agent->DefinedAgents();
        $agents{$jid}->{order} = $count++;
    }

    return %agents;
}


###############################################################################
#
# BrowseRequest - requests the browse information from the specified JID.
#
###############################################################################
sub BrowseRequest
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"get");
    my $query = $iq->NewQuery("jabber:iq:browse");

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

    $query = $iq->GetQuery();

    if (defined($query))
    {
        my %browse = %{$self->BrowseParse($query)};
        return %browse;
    }
    else
    {
        return;
    }
}


###############################################################################
#
# BrowseParse - helper function for BrowseRequest to convert the object
#               tree into a hash for better consumption.
#
###############################################################################
sub BrowseParse
{ 
    my $self = shift;
    my $item = shift;
    my %browse;

    if ($item->DefinedCategory())
    {
        $browse{category} = $item->GetCategory();
    }
    else
    {
        $browse{category} = $item->GetTag();
    }
    $browse{type} = $item->GetType();
    $browse{name} = $item->GetName();
    $browse{jid} = $item->GetJID();
    $browse{ns} = [ $item->GetNS() ];

    foreach my $subitem ($item->GetItems())
    {
        my ($subbrowse) = $self->BrowseParse($subitem);
        push(@{$browse{children}},$subbrowse);
    }

    return \%browse;
}


###############################################################################
#
# BrowseDBDelete - delete the JID from the DB completely.
#
###############################################################################
sub BrowseDBDelete
{
    my $self = shift;
    my ($jid) = @_;

    my $indexJID = $jid;
    $indexJID = $jid->GetJID() if (ref($jid) eq "Net::Jabber::JID");

    return if !exists($self->{BROWSEDB}->{$indexJID});
    delete($self->{BROWSEDB}->{$indexJID});
    $self->{DEBUG}->Log1("BrowseDBDelete: delete ",$indexJID," from the DB");
}


###############################################################################
#
# BrowseDBQuery - retrieve the last Net::Jabber::Browse received with
#                  the highest priority.
#
###############################################################################
sub BrowseDBQuery
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $args{timeout} = 10 unless exists($args{timeout});

    my $indexJID = $args{jid};
    $indexJID = $args{jid}->GetJID() if (ref($args{jid}) eq "Net::Jabber::JID");

    if ((exists($args{refresh}) && ($args{refresh} eq "1")) ||
        (!exists($self->{BROWSEDB}->{$indexJID})))
    {
        my %browse = $self->BrowseRequest(jid=>$args{jid},
                                          timeout=>$args{timeout});

        $self->{BROWSEDB}->{$indexJID} = \%browse;
    }
    return %{$self->{BROWSEDB}->{$indexJID}};
}


###############################################################################
#
# ByteStreamsProxyRequest - This queries a proxy server to get a list of 
#
###############################################################################
sub ByteStreamsProxyRequest
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"get");
    my $query = $iq->NewQuery("http://jabber.org/protocol/bytestreams");

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

    $query = $iq->GetQuery();

    if (defined($query))
    {
        my @hosts = @{$self->ByteStreamsProxyParse($query)};
        return @hosts;
    }
    else
    {
        return;
    }
}


###############################################################################
#
# ByteStreamsProxyParse - helper function for ByteStreamProxyRequest to convert
#                         the object tree into a hash for better consumption.
#
###############################################################################
sub ByteStreamsProxyParse
{
    my $self = shift;
    my $item = shift;

    my @hosts;

    foreach my $host ($item->GetStreamHosts())
    {
        my %host;
        $host{jid} = $host->GetJID();
        $host{host} = $host->GetHost() if $host->DefinedHost();
        $host{port} = $host->GetPort() if $host->DefinedPort();
        $host{zeroconf} = $host->GetZeroConf() if $host->DefinedZeroConf();

        push(@hosts,\%host);
    }
    
    return \@hosts;
}


###############################################################################
#
# ByteStreamsProxyActivate - This tells a proxy to activate the connection 
#
###############################################################################
sub ByteStreamsProxyActivate
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"set");
    my $query = $iq->NewQuery("http://jabber.org/protocol/bytestreams");
    $query->SetByteStreams(sid=>$args{sid},
                           activate=>(ref($args{recipient}) eq "Net::Jabber::JID" ? $args{recipient}->GetJID("full") : $args{recipient})
                         );
    
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

    return 1;
}


###############################################################################
#
# ByteStreamsOffer - This offers a recipient a list of stream hosts to pick
#                    from.
#
###############################################################################
sub ByteStreamsOffer
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"set");
    my $query = $iq->NewQuery("http://jabber.org/protocol/bytestreams");

    $query->SetByteStreams(sid=>$args{sid});

    foreach my $host (@{$args{streamhosts}})
    {
        $query->AddStreamHost(jid=>$host->{jid},
                              (exists($host->{host}) ? (host=>$host->{host}) : ()),
                              (exists($host->{port}) ? (port=>$host->{port}) : ()),
                              (exists($host->{zeroconf}) ? (zeroconf=>$host->{zeroconf}) : ()),
                             );
    }

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

    $query = $iq->GetQuery();

    if (defined($query))
    {
        return $query->GetStreamHostUsedJID();
    }
    else
    {
        return;
    }
}


###############################################################################
#
# DiscoInfoRequest - requests the disco information from the specified JID.
#
###############################################################################
sub DiscoInfoRequest
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"get");
    my $query = $iq->NewQuery("http://jabber.org/protocol/disco#info");
    $query->SetDiscoInfo(node=>$args{node}) if exists($args{node});

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
    return unless $iq->DefinedQuery();

    $query = $iq->GetQuery();

    return %{$self->DiscoInfoParse($query)};
}


###############################################################################
#
# DiscoInfoParse - helper function for DiscoInfoRequest to convert the object
#                  tree into a hash for better consumption.
#
###############################################################################
sub DiscoInfoParse
{
    my $self = shift;
    my $item = shift;

    my %disco;

    foreach my $ident ($item->GetIdentities())
    {
        my %identity;
        $identity{category} = $ident->GetCategory();
        $identity{name} = $ident->GetName();
        $identity{type} = $ident->GetType();
        push(@{$disco{identity}},\%identity);
    }

    foreach my $feat ($item->GetFeatures())
    {
        $disco{feature}->{$feat->GetVar()} = 1;
    }
    
    return \%disco;
}


###############################################################################
#
# DiscoItemsRequest - requests the disco information from the specified JID.
#
###############################################################################
sub DiscoItemsRequest
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"get");
    my $query = $iq->NewQuery("http://jabber.org/protocol/disco#items");
    $query->SetDiscoItems(node=>$args{node}) if exists($args{node});

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

    $query = $iq->GetQuery();

    if (defined($query))
    {
        my %disco = %{$self->DiscoItemsParse($query)};
        return %disco;
    }
    else
    {
        return;
    }
}


###############################################################################
#
# DiscoItemsParse - helper function for DiscoItemsRequest to convert the object
#                   tree into a hash for better consumption.
#
###############################################################################
sub DiscoItemsParse
{
    my $self = shift;
    my $item = shift;

    my %disco;

    foreach my $item ($item->GetItems())
    {
        $disco{$item->GetJID()}->{$item->GetNode()} = $item->GetName();
    }
    
    return \%disco;
}


###############################################################################
#
# FeatureNegRequest - requests a feature negotiation from the specified JID.
#
###############################################################################
sub FeatureNegRequest
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"get");

    my $query = $self->FeatureNegQuery($args{features});

    $iq->AddQuery($query);
    
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

    $query = $iq->GetQuery();

    if (defined($query))
    {
        my %feats = %{$self->FeatureNegParse($query)};
        return %feats;
    }
    else
    {
        return;
    }
}

#xxx fneg needs to reutrn a type='submit' on the x:data in a result


###############################################################################
#
# FeatureNegQuery - given a feature hash, return a query that contains it.
#
###############################################################################
sub FeatureNegQuery
{
    my $self = shift;
    my $features = shift;

    my $tag = "query";
    $tag = $Net::Jabber::Query::TAGS{'http://jabber.org/protocol/feature-neg'}
        if exists($Net::Jabber::Query::TAGS{'http://jabber.org/protocol/feature-neg'});
    
    my $query = new Net::Jabber::Query($tag);
    $query->SetXMLNS("http://jabber.org/protocol/feature-neg");
    my $xdata = $query->NewX("jabber:x:data");
    
    foreach my $feature (keys(%{$features}))
    {
        my $field = $xdata->AddField(type=>"list-single",
                                     var=>$feature);
        foreach my $value (@{$features->{$feature}})
        {
            $field->AddOption(value=>$value);
        }
    }

    return $query;
}


###############################################################################
#
# FeatureNegParse - helper function for FeatureNegRequest to convert the object
#                   tree into a hash for better consumption.
#
###############################################################################
sub FeatureNegParse
{
    my $self = shift;
    my $item = shift;

    my %feats;

    my $xdata = $item->GetX("jabber:x:data");
    
    foreach my $field ($xdata->GetFields())
    {
        my @options;
        
        foreach my $option ($field->GetOptions())
        {
            push(@options,$option->GetValue());
        }

        if ($#options == -1)
        {
            
            $feats{$field->GetVar()} = $field->GetValue();
        }
        else
        {
            $feats{$field->GetVar()} = \@options;
        }
    }
    
    return \%feats;
}

#XXX - need a feature-neg answer function...

###############################################################################
#
# FileTransferOffer - offer a file transfer JEP-95
#
###############################################################################
sub FileTransferOffer
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"set");
    my $query = $iq->NewQuery("http://jabber.org/protocol/si");
    my $profile = $query->NewQuery("http://jabber.org/protocol/si/profile/file-transfer");

    # XXX support hashing via MD5
    # XXX support date via JEP-82

    my ($filename) = ($args{filename} =~ /\/?([^\/]+)$/);

    $profile->SetFile(name=>$filename,
                      size=>(-s $args{filename})
                     );

    $profile->SetFile(desc=>$args{desc}) if exists($args{desc});

    $query->SetStream(mimetype=>(-B $args{filename} ? 
                                    "application/octect-stream" :
                                    "text/plain"
                                ),
                      id=>$args{sid},
                      profile=>"http://jabber.org/protocol/si/profile/file-transfer"
                     );

    if (!exists($args{skip_methods}))
    {
        if ($#{$args{methods}} == -1)
        {
            print STDERR "You did not provide any valid methods for file transfer.\n";
            return;
        }

        my $fneg = $self->FeatureNegQuery({'stream-method'=>$args{methods}});

        $query->AddQuery($fneg);
    }

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

    $query = $iq->GetQuery();

    if (defined($query))
    {
        my @fneg = $query->GetQuery("http://jabber.org/protocol/feature-neg");
        my @xdata = $fneg[0]->GetX("jabber:x:data");
        my @fields = $xdata[0]->GetFields();
        return $fields[0]->GetValue();
        # XXX need better error handling
    }
    else
    {
        return;
    }
}


###############################################################################
#
# TreeTransferOffer - offer a file transfer JEP-95
#
###############################################################################
sub TreeTransferOffer
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>$args{jid},
               type=>"set");
    my $query = $iq->NewQuery("http://jabber.org/protocol/si");
    my $profile = $query->NewQuery("http://jabber.org/protocol/si/profile/tree-transfer");

    my ($root) = ($args{directory} =~ /\/?([^\/]+)$/);

    my $rootDir = $profile->AddDirectory(name=>$root);

    my %tree;
    $tree{counter} = 0;
    $self->TreeTransferDescend($args{sidbase},
                               $args{directory},
                               $rootDir,
                               \%tree
                              );

    $profile->SetTree(numfiles=>$tree{counter},
                      size=>$tree{size}
                     );

    $query->SetStream(id=>$args{sidbase},
                      profile=>"http://jabber.org/protocol/si/profile/tree-transfer"
                     );

    if ($#{$args{methods}} == -1)
    {
        print STDERR "You did not provide any valid methods for the tree transfer.\n";
        return;
    }

    my $fneg = $self->FeatureNegQuery({'stream-method'=>$args{methods}});

    $query->AddQuery($fneg);

    #--------------------------------------------------------------------------
    # Send the IQ with the next available ID and wait for a reply with that
    # id to be received.  Then grab the IQ reply.
    #--------------------------------------------------------------------------
    if ($args{mode} eq "passthru")
    {
        my $id = $self->UniqueID();
        $iq->SetIQ(id=>$id);
        $self->Send($iq);
        $tree{id} = $id;
        return %tree;
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

    $query = $iq->GetQuery();

    if (defined($query))
    {
        my @fneg = $query->GetQuery("http://jabber.org/protocol/feature-neg");
        my @xdata = $fneg[0]->GetX("jabber:x:data");
        my @fields = $xdata[0]->GetFields();
        return $fields[0]->GetValue();
        # XXX need better error handling
    }
    else
    {
        return;
    }
}


###############################################################################
#
# TreeTransferDescend - descend a directory structure and build the packet.
#
###############################################################################
sub TreeTransferDescend
{
    my $self = shift;
    my $sidbase = shift;
    my $path = shift;
    my $parent = shift;
    my $tree = shift;

    $tree->{size} += (-s $path);
            
    opendir(DIR, $path);
    foreach my $file ( sort {$a cmp $b} readdir(DIR) )
    {
        next if ($file =~ /^\.\.?$/);

        if (-d "$path/$file")
        {
            my $tempParent = $parent->AddDirectory(name=>$file);
            $self->TreeTransferDescend($sidbase,
                                       "$path/$file",
                                       $tempParent,
                                       $tree
                                      );
        }
        else
        {
            $tree->{size} += (-s "$path/$file");
            
            $tree->{tree}->{"$path/$file"}->{order} = $tree->{counter};
            $tree->{tree}->{"$path/$file"}->{sid} =
                $sidbase."-".$tree->{counter};
            $tree->{tree}->{"$path/$file"}->{name} = $file;

            $parent->AddFile(name=>$tree->{tree}->{"$path/$file"}->{name},
                             sid=>$tree->{tree}->{"$path/$file"}->{sid});
            $tree->{counter}++;
        }
    }
    closedir(DIR);
}


###############################################################################
#
# LastQuery - Sends an iq:last query to either the server or the specified
#             JID.
#
###############################################################################
sub LastQuery
{
    my $self = shift;
    my %args;
    $args{mode} = "passthru";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $args{waitforid} = 0 unless exists($args{waitforid});
    my $waitforid = delete($args{waitforid});
    $args{mode} = "block" if $waitforid;
    
    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to})) if exists($args{to});
    $iq->SetIQ(type=>'get');
    my $last = $iq->NewQuery("jabber:iq:last");

    if ($args{mode} eq "passthru")
    {
        my $id = $self->UniqueID();
        $iq->SetIQ(id=>$id);
        $self->Send($iq);
        return $id;
    }
    
    return $self->SendWithID($iq) if ($args{mode} eq "nonblock");

    $iq = $self->SendAndReceiveWithID($iq,$timeout);

    return unless defined($iq);

    $last = $iq->GetQuery();

    return unless defined($last);

    return $last->GetLast();
}


###############################################################################
#
# LastSend - sends an iq:last packet to the specified user.
#
###############################################################################
sub LastSend
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $args{ignoreactivity} = 0 unless exists($args{ignoreactivity});
    my $ignoreActivity = delete($args{ignoreactivity});

    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to}),
             type=>'result');
    my $last = $iq->NewQuery("jabber:iq:last");
    $last->SetLast(%args);

    $self->Send($iq,$ignoreActivity);
}


###############################################################################
#
# LastActivity - returns number of seconds since the last activity.
#
###############################################################################
sub LastActivity
{
    my $self = shift;

    return (time - $self->{STREAM}->LastActivity($self->{SESSION}->{id}));
}


###############################################################################
#
# RegisterSendData - This is a self contained function to send a register iq
#                    tag with an id.  It uses the jabber:x:data method to
#                    return the data.
#
###############################################################################
sub RegisterSendData
{
    my $self = shift;
    my $to = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    #--------------------------------------------------------------------------
    # Create a Net::Jabber::IQ object to send to the server
    #--------------------------------------------------------------------------
    my $iq = $self->_iq();
    $iq->SetIQ(to=>$to) if (defined($to) && ($to ne ""));
    $iq->SetIQ(type=>"set");
    my $iqRegister = $iq->NewQuery("jabber:iq:register");
    my $xForm = $iqRegister->NewX("jabber:x:data");
    foreach my $var (keys(%args))
    {
        next if ($args{$var} eq "");
        $xForm->AddField(var=>$var,
                         value=>$args{$var}
                        );
    }

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


###############################################################################
#
# RPCSetCallBacks - place to register a callback for RPC calls.  This is
#                   used in conjunction with the default IQ callback.
#
###############################################################################
sub RPCSetCallBacks
{
    my $self = shift;
    while($#_ >= 0) {
        my $func = pop(@_);
        my $method = pop(@_);
        $self->{DEBUG}->Log2("RPCSetCallBacks: method($method) func($func)");
        if (defined($func))
        {
            $self->{RPCCB}{$method} = $func;
        }
        else
        {
            delete($self->{RPCCB}{$method});
        }
    }
}


###############################################################################
#
# RPCCall - Make an RPC call to the specified JID.
#
###############################################################################
sub RPCCall
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"set",
               to=>delete($args{to}));
    $iq->AddQuery($self->RPCEncode(type=>"methodCall",
                                   %args));

    if ($args{mode} eq "passthru")
    {
        my $id = $self->UniqueID();
        $iq->SetIQ(id=>$id);
        $self->Send($iq);
        return $id;
    }
    
    return $self->SendWithID($iq) if ($args{mode} eq "nonblock");

    $iq = $self->SendAndReceiveWithID($iq,$timeout);

    return unless defined($iq);

    return $self->RPCParse($iq);
}


###############################################################################
#
# RPCResponse - Send back an RPC response, or fault, to the specified JID.
#
###############################################################################
sub RPCResponse
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $iq = $self->_iq();
    $iq->SetIQ(type=>"result",
               to=>delete($args{to}));
    $iq->AddQuery($self->RPCEncode(type=>"methodResponse",
                                   %args));

    $iq = $self->SendAndReceiveWithID($iq);
    return unless defined($iq);

    return $self->RPCParse($iq);
}


###############################################################################
#
# RPCEncode - Returns a Net::Jabber::Query with the arguments encoded for the
#             RPC packet.
#
###############################################################################
sub RPCEncode
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $query = new Net::Jabber::Stanza("query");
    $query->SetXMLNS("jabber:iq:rpc");

    my $source;

    if ($args{type} eq "methodCall")
    {
        $source = $query->AddMethodCall();
        $source->SetMethodName($args{methodname});
    }

    if ($args{type} eq "methodResponse")
    {
        $source = $query->AddMethodResponse();
    }

    if (exists($args{faultcode}) || exists($args{faultstring}))
    {
        my $struct = $source->AddFault()->AddValue()->AddStruct();
        $struct->AddMember(name=>"faultCode")->AddValue(i4=>$args{faultcode});
        $struct->AddMember(name=>"faultString")->AddValue(string=>$args{faultstring});
    }
    elsif (exists($args{params}))
    {
        my $params = $source->AddParams();
        foreach my $param (@{$args{params}})
        {
            $self->RPCEncode_Value($params->AddParam(),$param);
        }
    }

    return $query;
}


###############################################################################
#
# RPCEncode_Value - Run through the value, and encode it into XML.
#
###############################################################################
sub RPCEncode_Value
{
    my $self = shift;
    my $obj = shift;
    my $value = shift;

    if (ref($value) eq "ARRAY")
    {
        my $array = $obj->AddValue()->AddArray();
        foreach my $data (@{$value})
        {
            $self->RPCEncode_Value($array->AddData(),$data);
        }
    }
    elsif (ref($value) eq "HASH")
    {
        my $struct = $obj->AddValue()->AddStruct();
        foreach my $key (keys(%{$value}))
        {
            $self->RPCEncode_Value($struct->AddMember(name=>$key),$value->{$key});
        }
    }
    else
    {
        if ($value =~ /^(int|i4|boolean|string|double|datetime|base64):/i)
        {
            my $type = $1;
            my($val) = ($value =~ /^$type:(.*)$/);
            $obj->AddValue($type=>$val);
        }
        elsif ($value =~ /^[+-]?\d+$/)
        {
            $obj->AddValue(i4=>$value);
        }
        elsif ($value =~ /^(-?(?:\d+(?:\.\d*)?|\.\d+)|([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?)$/)
        {
            $obj->AddValue(double=>$value);
        }
        else
        {
            $obj->AddValue(string=>$value);
        }
    }
}


###############################################################################
#
# RPCParse - Returns an array of the params sent in the RPC packet.
#
###############################################################################
sub RPCParse
{
    my $self = shift;
    my($iq) = @_;

    my $query = $iq->GetQuery();

    my $source;
    $source = $query->GetMethodCall() if $query->DefinedMethodCall();
    $source = $query->GetMethodResponse() if $query->DefinedMethodResponse();

    if (defined($source))
    {
        if (($source->GetTag() eq "methodResponse") && ($source->DefinedFault()))
        {
            my %response =
                $self->RPCParse_Struct($source->GetFault()->GetValue()->GetStruct());
            return ("fault",\%response);
        }

        if ($source->DefinedParams())
        {
            #------------------------------------------------------------------
            # The <param/>s part
            #------------------------------------------------------------------
            my @response;
            foreach my $param ($source->GetParams()->GetParams())
            {
                push(@response,$self->RPCParse_Value($param->GetValue()));
            }
            return ("ok",\@response);
        }
    }
    else
    {
        print "AAAAHHHH!!!!\n";
    }
}


###############################################################################
#
# RPCParse_Value - Takes a <value/> and returns the data it represents
#
###############################################################################
sub RPCParse_Value
{
    my $self = shift;
    my($value) = @_;

    if ($value->DefinedStruct())
    {
        my %struct = $self->RPCParse_Struct($value->GetStruct());
        return \%struct;
    }

    if ($value->DefinedArray())
    {
        my @array = $self->RPCParse_Array($value->GetArray());
        return \@array;
    }

    return $value->GetI4() if $value->DefinedI4();
    return $value->GetInt() if $value->DefinedInt();
    return $value->GetBoolean() if $value->DefinedBoolean();
    return $value->GetString() if $value->DefinedString();
    return $value->GetDouble() if $value->DefinedDouble();
    return $value->GetDateTime() if $value->DefinedDateTime();
    return $value->GetBase64() if $value->DefinedBase64();

    return $value->GetValue();
}


###############################################################################
#
# RPCParse_Struct - Takes a <struct/> and returns the hash of values.
#
###############################################################################
sub RPCParse_Struct
{
    my $self = shift;
    my($struct) = @_;

    my %struct;
    foreach my $member ($struct->GetMembers())
    {
        $struct{$member->GetName()} = $self->RPCParse_Value($member->GetValue());
    }

    return %struct;
}


###############################################################################
#
# RPCParse_Array - Takes a <array/> and returns the hash of values.
#
###############################################################################
sub RPCParse_Array
{
    my $self = shift;
    my($array) = @_;

    my @array;
    foreach my $data ($array->GetDatas())
    {
        push(@array,$self->RPCParse_Value($data->GetValue()));
    }

    return @array;
}


###############################################################################
#
# SearchRequest - This is a self contained function to send an iq tag
#                 an id that requests the target address to send back
#                 the required fields.  It waits for a reply what the
#                 same id to come back and tell the caller what the
#                 fields are.
#
###############################################################################
sub SearchRequest
{
    my $self = shift;
    my %args;
    $args{mode} = "block";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    #--------------------------------------------------------------------------
    # Create a Net::Jabber::IQ object to send to the server
    #--------------------------------------------------------------------------
    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to})) if exists($args{to});
    $iq->SetIQ(type=>"get");
    my $query = $iq->NewQuery("jabber:iq:search");

    $self->{DEBUG}->Log1("SearchRequest: sent(",$iq->GetXML(),")");

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

    $self->{DEBUG}->Log1("SearchRequest: received(",$iq->GetXML(),")")
        if defined($iq);

    #--------------------------------------------------------------------------
    # Check if there was an error.
    #--------------------------------------------------------------------------
    return unless defined($iq);
    if ($iq->GetType() eq "error")
    {
        $self->SetErrorCode($iq->GetErrorCode().": ".$iq->GetError());
        $self->{DEBUG}->Log1("SearchRequest: error(",$self->GetErrorCode(),")");
        return;
    }

    my %search;
    #--------------------------------------------------------------------------
    # From the reply IQ determine what fields are required and send a hash
    # back with the fields and any values that are already defined (like key)
    #--------------------------------------------------------------------------
    $query = $iq->GetQuery();
    $search{fields} = { $query->GetSearch() };

    #--------------------------------------------------------------------------
    # Get any forms so that we have the option of showing a nive dynamic form
    # to the user and not just a bunch of fields.
    #--------------------------------------------------------------------------
    &ExtractForms(\%search,$query->GetX("jabber:x:data"));

    #--------------------------------------------------------------------------
    # Get any oobs so that we have the option of sending the user to the http
    # form and not a dynamic one.
    #--------------------------------------------------------------------------
    &ExtractOobs(\%search,$query->GetX("jabber:x:oob"));

    return %search;
}


###############################################################################
#
# SearchSend - This is a self contained function to send a search
#              iq tag with an id.  Then wait for a reply what the same
#              id to come back and tell the caller what the result was.
#
###############################################################################
sub SearchSend
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    #--------------------------------------------------------------------------
    # Create a Net::Jabber::IQ object to send to the server
    #--------------------------------------------------------------------------
    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to})) if exists($args{to});
    $iq->SetIQ(type=>"set");
    my $iqSearch = $iq->NewQuery("jabber:iq:search");
    $iqSearch->SetSearch(%args);

    #--------------------------------------------------------------------------
    # Send the IQ.
    #--------------------------------------------------------------------------
    $self->Send($iq);
}


###############################################################################
#
# SearchSendData - This is a self contained function to send a search iq tag
#                  with an id.  It uses the jabber:x:data method to return the
#                  data.
#
###############################################################################
sub SearchSendData
{
    my $self = shift;
    my $to = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    #--------------------------------------------------------------------------
    # Create a Net::Jabber::IQ object to send to the server
    #--------------------------------------------------------------------------
    my $iq = $self->_iq();
    $iq->SetIQ(to=>$to) if (defined($to) && ($to ne ""));
    $iq->SetIQ(type=>"set");
    my $iqSearch = $iq->NewQuery("jabber:iq:search");
    my $xForm = $iqSearch->NewX("jabber:x:data");
    foreach my $var (keys(%args))
    {
        next if ($args{$var} eq "");
        $xForm->AddField(var=>$var,
                         value=>$args{$var}
                        );
    }

    #--------------------------------------------------------------------------
    # Send the IQ.
    #--------------------------------------------------------------------------
    $self->Send($iq);
}


###############################################################################
#
# TimeQuery - Sends an iq:time query to either the server or the specified
#             JID.
#
###############################################################################
sub TimeQuery
{
    my $self = shift;
    my %args;
    $args{mode} = "passthru";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $args{waitforid} = 0 unless exists($args{waitforid});
    my $waitforid = delete($args{waitforid});
    $args{mode} = "block" if $waitforid;

    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to})) if exists($args{to});
    $iq->SetIQ(type=>'get',%args);
    my $time = $iq->NewQuery("jabber:iq:time");

    if ($args{mode} eq "passthru")
    {
        my $id = $self->UniqueID();
        $iq->SetIQ(id=>$id);
        $self->Send($iq);
        return $id;
    }
    
    return $self->SendWithID($iq) if ($args{mode} eq "nonblock");

    $iq = $self->SendAndReceiveWithID($iq,$timeout);

    return unless defined($iq);

    my $query = $iq->GetQuery();

    return unless defined($query);

    my %result;
    $result{utc} = $query->GetUTC();
    $result{display} = $query->GetDisplay();
    $result{tz} = $query->GetTZ();
    return %result;
}


###############################################################################
#
# TimeSend - sends an iq:time packet to the specified user.
#
###############################################################################
sub TimeSend
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to}),
               type=>'result');
    my $time = $iq->NewQuery("jabber:iq:time");
    $time->SetTime(%args);

    $self->Send($iq);
}



###############################################################################
#
# VersionQuery - Sends an iq:version query to either the server or the
#                specified JID.
#
###############################################################################
sub VersionQuery
{
    my $self = shift;
    my %args;
    $args{mode} = "passthru";
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $args{waitforid} = 0 unless exists($args{waitforid});
    my $waitforid = delete($args{waitforid});
    $args{mode} = "block" if $waitforid;
    
    my $timeout = exists($args{timeout}) ? delete($args{timeout}) : undef;

    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to})) if exists($args{to});
    $iq->SetIQ(type=>'get',%args);
    my $version = $iq->NewQuery("jabber:iq:version");

    if ($args{mode} eq "passthru")
    {
        my $id = $self->UniqueID();
        $iq->SetIQ(id=>$id);
        $self->Send($iq);
        return $id;
    }
    
    return $self->SendWithID($iq) if ($args{mode} eq "nonblock");

    $iq = $self->SendAndReceiveWithID($iq,$timeout);

    return unless defined($iq);

    my $query = $iq->GetQuery();

    return unless defined($query);

    my %result;
    $result{name} = $query->GetName();
    $result{ver} = $query->GetVer();
    $result{os} = $query->GetOS();
    return %result;
}


###############################################################################
#
# VersionSend - sends an iq:version packet to the specified user.
#
###############################################################################
sub VersionSend
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $iq = $self->_iq();
    $iq->SetIQ(to=>delete($args{to}),
               type=>'result');
    my $version = $iq->NewQuery("jabber:iq:version");
    $version->SetVersion(%args);

    $self->Send($iq);
}


###############################################################################
#
# MUCJoin - join a MUC room
#
###############################################################################
sub MUCJoin
{
    my $self = shift;
    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    my $presence = $self->_presence();
    $presence->SetTo($args{room}.'@'.$args{server}.'/'.$args{nick});
    my $x = $presence->NewChild("http://jabber.org/protocol/muc");

    if (exists($args{password}) && ($args{password} ne ""))
    {
        $x->SetMUC(password=>$args{password});
    }
    
    return $presence->GetXML() if exists($args{'__netjabber__:test'});
    $self->Send($presence);
}


###############################################################################
#+-----------------------------------------------------------------------------
#|
#| Helper Functions
#|
#+-----------------------------------------------------------------------------
###############################################################################


###############################################################################
#
# ExtractForms - Helper function to make extracting jabber:x:data for forms
#                more centrally definable.
#
###############################################################################
sub ExtractForms
{
    my ($target,@xForms) = @_;

    my $tempVar = "1";
    foreach my $xForm (@xForms) {
        $target->{instructions} = $xForm->GetInstructions();
        my $order = 0;
        foreach my $field ($xForm->GetFields())
        {
            $target->{form}->[$order]->{type} = $field->GetType()
                if $field->DefinedType();
            $target->{form}->[$order]->{label} = $field->GetLabel()
                if $field->DefinedLabel();
            $target->{form}->[$order]->{desc} = $field->GetDesc()
                if $field->DefinedDesc();
            $target->{form}->[$order]->{var} = $field->GetVar()
                if $field->DefinedVar();
            $target->{form}->[$order]->{var} = "__netjabber__:tempvar:".$tempVar++
                if !$field->DefinedVar();
            if ($field->DefinedValue())
            {
                if ($field->GetType() eq "list-multi")
                {
                    $target->{form}->[$order]->{value} = [ $field->GetValue() ];
                }
                else
                {
                    $target->{form}->[$order]->{value} = ($field->GetValue())[0];
                }
            }  
            my $count = 0;
            foreach my $option ($field->GetOptions())
            {
                $target->{form}->[$order]->{options}->[$count]->{value} =
                    $option->GetValue();
                $target->{form}->[$order]->{options}->[$count]->{label} =
                    $option->GetLabel();
                $count++;
            }
            $order++;
        }
        foreach my $reported ($xForm->GetReported())
        {
            my $order = 0;
            foreach my $field ($reported->GetFields())
            {
                $target->{reported}->[$order]->{label} = $field->GetLabel();
                $target->{reported}->[$order]->{var} = $field->GetVar();
                $order++;
            }
        }
    }
}


###############################################################################
#
# ExtractOobs - Helper function to make extracting jabber:x:oob for forms
#               more centrally definable.
#
###############################################################################
sub ExtractOobs
{
    my ($target,@xOobs) = @_;

    foreach my $xOob (@xOobs)
    {
        $target->{oob}->{url} = $xOob->GetURL();
        $target->{oob}->{desc} = $xOob->GetDesc();
    }
}


###############################################################################
#+-----------------------------------------------------------------------------
#|
#| Default CallBacks
#|
#+-----------------------------------------------------------------------------
###############################################################################


###############################################################################
#
# callbackInit - initialize the default callbacks
#
###############################################################################
sub callbackInit
{
    my $self = shift;

    $self->SUPER::callbackInit();

    $self->SetIQCallBacks("jabber:iq:last"=>
                          {
                            get=>sub{ $self->callbackGetIQLast(@_) },
                            result=>sub{ $self->callbackResultIQLast(@_) }
                          },
                          "jabber:iq:rpc"=>
                          {
                            set=>sub{ $self->callbackSetIQRPC(@_) },
                          },
                          "jabber:iq:time"=>
                          {
                            get=>sub{ $self->callbackGetIQTime(@_) },
                            result=>sub{ $self->callbackResultIQTime(@_) }
                          },
                          "jabber:iq:version"=>
                          {
                            get=>sub{ $self->callbackGetIQVersion(@_) },
                            result=>sub{ $self->callbackResultIQVersion(@_) }
                          },
                         );
}


###############################################################################
#
# callbackSetIQRPC - callback to handle auto-replying to an iq:rpc by calling
#                    the user registered functions.
#
###############################################################################
sub callbackSetIQRPC
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    my $query = $iq->GetQuery();

    my $reply = $iq->Reply(type=>"result");
    my $replyQuery = $reply->GetQuery();

    if (!$query->DefinedMethodCall())
    {
        my $methodResponse = $replyQuery->AddMethodResponse();
        my $struct = $methodResponse->AddFault()->AddValue()->AddStruct();
        $struct->AddMember(name=>"faultCode")->AddValue(int=>400);
        $struct->AddMember(name=>"faultString")->AddValue(string=>"Missing methodCall.");
        $self->Send($reply,1);
        return;
    }

    if (!$query->GetMethodCall()->DefinedMethodName())
    {
        my $methodResponse = $replyQuery->AddMethodResponse();
        my $struct = $methodResponse->AddFault()->AddValue()->AddStruct();
        $struct->AddMember(name=>"faultCode")->AddValue(int=>400);
        $struct->AddMember(name=>"faultString")->AddValue(string=>"Missing methodName.");
        $self->Send($reply,1);
        return;
    }

    my $methodName = $query->GetMethodCall()->GetMethodName();

    if (!exists($self->{RPCCB}->{$methodName}))
    {
        my $methodResponse = $replyQuery->AddMethodResponse();
        my $struct = $methodResponse->AddFault()->AddValue()->AddStruct();
        $struct->AddMember(name=>"faultCode")->AddValue(int=>404);
        $struct->AddMember(name=>"faultString")->AddValue(string=>"methodName $methodName not defined.");
        $self->Send($reply,1);
        return;
    }

    my @params = $self->RPCParse($iq);

    my @return = &{$self->{RPCCB}->{$methodName}}($iq,$params[1]);

    if ($return[0] ne "ok") {
        my $methodResponse = $replyQuery->AddMethodResponse();
        my $struct = $methodResponse->AddFault()->AddValue()->AddStruct();
        $struct->AddMember(name=>"faultCode")->AddValue(int=>$return[1]->{faultCode});
        $struct->AddMember(name=>"faultString")->AddValue(string=>$return[1]->{faultString});
        $self->Send($reply,1);
        return;
    }
    $reply->RemoveQuery();
    $reply->AddQuery($self->RPCEncode(type=>"methodResponse",
                                      params=>$return[1]));

    $self->Send($reply,1);
}


###############################################################################
#
# callbackGetIQTime - callback to handle auto-replying to an iq:time get.
#
###############################################################################
sub callbackGetIQTime
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    my $query = $iq->GetQuery();

    my $reply = $iq->Reply(type=>"result");
    my $replyQuery = $reply->GetQuery();
    $replyQuery->SetTime();

    $self->Send($reply,1);
}


###############################################################################
#
# callbackResultIQTime - callback to handle formatting iq:time result into
#                        a message.
#
###############################################################################
sub callbackResultIQTime
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    my $fromJID = $iq->GetFrom("jid");
    my $query = $iq->GetQuery();

    my $body = "UTC: ".$query->GetUTC()."\n";
    $body .=   "Time: ".$query->GetDisplay()."\n";
    $body .=   "Timezone: ".$query->GetTZ()."\n";
    
    my $message = $self->_message();
    $message->SetMessage(to=>$iq->GetTo(),
                         from=>$iq->GetFrom(),
                         subject=>"CTCP: Time",
                         body=>$body);


    $self->CallBack($sid,$message);
}


###############################################################################
#
# callbackGetIQVersion - callback to handle auto-replying to an iq:time
#                        get.
#
###############################################################################
sub callbackGetIQVersion
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    my $query = $iq->GetQuery();

    my $reply = $iq->Reply(type=>"result");
    my $replyQuery = $reply->GetQuery();
    $replyQuery->SetVersion(name=>$self->{INFO}->{name},
                            ver=>$self->{INFO}->{version},
                            os=>"");

    $self->Send($reply,1);
}


###############################################################################
#
# callbackResultIQVersion - callback to handle formatting iq:time result
#                           into a message.
#
###############################################################################
sub callbackResultIQVersion
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    my $query = $iq->GetQuery();

    my $body = "Program: ".$query->GetName()."\n";
    $body .=   "Version: ".$query->GetVer()."\n";
    $body .=   "OS: ".$query->GetOS()."\n";

    my $message = $self->_message();
    $message->SetMessage(to=>$iq->GetTo(),
                         from=>$iq->GetFrom(),
                         subject=>"CTCP: Version",
                         body=>$body);

    $self->CallBack($sid,$message);
}


###############################################################################
#
# callbackGetIQLast - callback to handle auto-replying to an iq:last get.
#
###############################################################################
sub callbackGetIQLast
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    my $query = $iq->GetQuery();
    my $reply = $iq->Reply(type=>"result");
    my $replyQuery = $reply->GetQuery();
    $replyQuery->SetLast(seconds=>$self->LastActivity());

    $self->Send($reply,1);
}


###############################################################################
#
# callbackResultIQLast - callback to handle formatting iq:last result into
#                        a message.
#
###############################################################################
sub callbackResultIQLast
{
    my $self = shift;
    my $sid = shift;
    my $iq = shift;

    my $fromJID = $iq->GetFrom("jid");
    my $query = $iq->GetQuery();
    my $seconds = $query->GetSeconds();

    my $lastTime = &Net::Jabber::GetTimeStamp("local",(time - $seconds),"long");

    my $elapsedTime = &Net::Jabber::GetHumanTime($seconds);

    my $body;
    if ($fromJID->GetUserID() eq "")
    {
        $body  = "Start Time: $lastTime\n";
        $body .= "Up time: $elapsedTime\n";
        $body .= "Message: ".$query->GetMessage()."\n"
            if ($query->DefinedMessage());
    }
    elsif ($fromJID->GetResource() eq "")
    {
        $body  = "Logout Time: $lastTime\n";
        $body .= "Elapsed time: $elapsedTime\n";
        $body .= "Message: ".$query->GetMessage()."\n"
            if ($query->DefinedMessage());
    }
    else
    {
        $body  = "Last activity: $lastTime\n";
        $body .= "Elapsed time: $elapsedTime\n";
        $body .= "Message: ".$query->GetMessage()."\n"
            if ($query->DefinedMessage());
    }
    
    my $message = $self->_message();
    $message->SetMessage(from=>$iq->GetFrom(),
                         subject=>"Last Activity",
                         body=>$body);

    $self->CallBack($sid,$message);
}


1;
