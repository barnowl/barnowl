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

package Net::XMPP::Namespaces;

=head1 NAME

Net::XMPP::Namespaces - In depth discussion on how namespaces are handled

=head1 SYNOPSIS

  Net::XMPP::Namespaces provides an depth look at how Net::XMPP handles
  namespacs, and how to add your own custom ones.  It also serves as the
  storage bin for all of the Namespace information Net::XMPP requires.

=head1 DESCRIPTION

  XMPP as a protocol is very well defined.  There are three main top
  level packets (message, iq, and presence).  There is also a way to
  extend the protocol in a very clear and strucutred way, via namespaces.

  Two major ways that namespaces are used in Jabber is for making the
  <iq/> a generic wrapper, and as a way for adding data to any packet via
  a child tag <x/>.  We will use <x/> to represent the packet, but in
  reality it could be any child tag: <foo/>, <data/>, <error/>, etc.

  The Info/Query <iq/> packet uses namespaces to determine the type of
  information to access.  Usually there is a <query/> tag in the <iq/>
  that represents the namespace, but in fact it can be any tag.  The
  definition of the Query portion, is the first tag that has a namespace.

    <iq type="get"><query xmlns="..."/></iq>

      or

    <iq type="get"><foo xmlns="..."/></iq>

  After that Query stanza can be any number of other stanzas (<x/> tags)
  you want to include.  The Query packet is represented and available by
  calling GetQuery() or GetChild(), and the other namespaces are
  available by calling GetChild().

  The X tag is just a way to piggy back data on other packets.  Like
  embedding the timestamp for a message using jabber:x:delay, or signing
  you presence for encryption using jabber:x:signed.

  To this end, Net::XMPP has sought to find a way to easily, and clearly
  define the functions needed to access the XML for a namespace.  We will
  go over the full docs, and then show two examples of real namespaces so
  that you can see what we are talking about.

=head2 Overview

  To avoid a lot of nasty modules populating memory that are not used,
  and to avoid having to change 15 modules when a minor change is
  introduced, the Net::XMPP modules have taken AUTOLOADing to the
  extreme.  Namespaces.pm is nothing but a set of function calls that
  generates a big hash of hashes.  The hash is accessed by the Stanza.pm
  AUTOLOAD function to do something.  (This will make sense, I promise.)

  Before going on, I highly suggest you read a Perl book on AUTOLOAD and
  how it works.  From this point on I will assume that you understand it.

  When you create a Net::XMPP::IQ object and add a Query to it (NewChild)
  several things are happening in the background.  The argument to
  NewChild is the namespace you want to add. (custom-namespace)

  Now that you have a Query object to work with you will call the GetXXX
  functions, and SetXXX functions to set the data.  There are no defined
  GetXXX and SetXXXX functions.  You cannot look in the Namespaces.pm
  file and find them.  Instead you will find something like this:

  &add_ns(ns    => "mynamespace",
          tag   => "mytag",
          xpath => {
                    JID       => { type=>'jid', path => '@jid' },
                    Username  => { path => 'username/text()' },
                    Test      => { type => 'master' }
                   }
         );

  When the GetUsername() function is called, the AUTOLOAD function looks
  in the Namespaces.pm hash for a "Username" key.  Based on the "type" of
  the field (scalar being the default) it will use the "path" as an XPath
  to retrieve the data and call the XPathGet() method in Stanza.pm.

  Confused yet?

=head2 Net::XMPP private namespaces

  Now this is where this starts to get a little sticky.  When you see a
  namespace with __netxmpp__, or __netjabber__ from Net::Jabber, at the
  beginning it is usually something custom to Net::XMPP and NOT part of
  the actual XMPP protocol.

  There are some places where the structure of the XML allows for
  multiple children with the same name.  The main places you will see
  this behavior is where you have multiple tags with the same name and
  those have children under them (jabber:iq:roster).

  In jabber:iq:roster, the <item/> tag can be repeated multiple times,
  and is sort of like a mini-namespace in itself.  To that end, we treat
  it like a seperate namespace and defined a __netxmpp__:iq:roster:item
  namespace to hold it.  What happens is this, in my code I define that
  the <item/>s tag is "item" and anything with that tag name is to create
  a new Net::XMPP::Stanza object with the namespace
  __netxmpp__:iq:roster:item which then becomes a child of the
  jabber:iq:roster Stanza object.  Also, when you want to add a new item
  to a jabber:iq:roster project you call NewQuery with the private
  namespace.

  I know this sounds complicated.  And if after reading this entire
  document it is still complicated, email me, ask questions, and I will
  monitor it and adjust these docs to answer the questions that people
  ask.

=head2 add_ns()

  To repeat, here is an example call to add_ns():

    &add_ns(ns    => "mynamespace",
            tag   => "mytag",
            xpath => {
                      JID       => { type=>'jid', path => '@jid' },
                      Username  => { path => 'username/text()' },
                      Test      => { type => 'master' }
                     }
           );

  ns - This is the new namespace that you are trying to add.

  tag - This is the root tag to use for objects based on this namespace.

  xpath - The hash reference passed in the add_ns call to each name of
  entry tells Net::XMPP how to handle subsequent GetXXXX(), SetXXXX(),
  DefinedXXXX(), RemoveXXXX(), AddXXXX() calls.  The basic options you
  can pass in are:

     type - This tells Stanza how to handle the call.  The possible
            values are:

           array - The value to set and returned is an an array
                   reference.  For example, <group/> in jabber:iq:roster.

           child - This tells Stanza that it needs to look for the
                   __netxmpp__ style namesapced children.  AddXXX() adds
                   a new child, and GetXXX() will return a new Stanza
                   object representing the packet.

           flag - This is for child elements that are tags by themselves:
                  <foo/>.  Since the presence of the tag is what is
                  important, and there is no cdata to store, we just call
                  it a flag.

           jid - The value is a Jabber ID.  GetXXX() will return a
                 Net::XMPP::JID object unless you pass it "jid", then it
                 returns a string.

           master - The GetXXX() and SetXXX() calls return and take a
                    hash representing all of the GetXXX() and SetXXX()
                    calls.  For example:

                      SetTest(foo=>"bar",
                              bar=>"baz");

                    Translates into:

                      SetFoo("bar");
                      SetBar("baz");

                    GetTest() would return a hash containing what the
                    packet contains:

                      { foo=>"bar",  bar=>"baz" }

           raw - This will stick whatever raw XML you specify directly
                 into the Stanza at the point where the path specifies.

           scalar - This will set and get a scalar value.  This is the
                    main workhorse as attributes and CDATA is represented
                    by a scalar.  This is the default setting if you do
                    not provide one.

           special - The special type is unique in that instead of a
                     string "special", you actually give it an array:

                       [ "special" , <subtype> ]

                     This allows Net::XMPP to be able to handle the
                     SetXXXX() call in a special manner according to your
                     choosing.  Right now this is mainly used by
                     jabber:iq:time to automatically set the time info in
                     the correct format, and jabber:iq:version to set the
                     machine OS and add the Net::Jabber version to the
                     return packet.  You will likely NOT need to use
                     this, but I wanted to mention it.

           timestamp - If you call SetXXX() but do not pass it anything,
                       or pass it "", then Net::XMPP will place a
                       timestamp in the xpath location.

     path - This is the XPath path to where the bit data lives.  The
            difference.  Now, this is not full XPath due to the nature
            of how it gets used.  Instead of providing a rooted path
            all the way to the top, it's a relative path ignoring what
            the parent is.  For example, if the "tag" you specified was
            "foo", and the path is "bar/text()", then the XPath will be
            rooted in the XML of the <foo/> packet.  It will set and get
            the CDATA from:

               <foo><bar>xxxxx</bar></foo>

            For a flag and a child type, just specify the child element.
            Take a look at the code in this file for more help on what
            this means.  Also, read up on XPath if you don't already know
            what it is.

     child - This is a hash reference that tells Net::XMPP how to handle
             adding and getting child objects.  The keys for the hash are
             as follows:

             ns - the real or custom (__netxmpp__) namesapce to use for
                  this child packet.

             skip_xmlns => 1 - this tells Net::XMPP not to add an
                               xmlns='' into the XML for the child
                               object.

             specify_name => 1 - allows you to call NewChild("ns","tag")
                                 and specify the tag to use for the child
                                 object.  This, IMHO, is BAD XML
                                 practice.  You should always know what
                                 the tag of the child is and use an
                                 attribute or CDATA to change the type
                                 of the stanza.  You do not want to use
                                 this.

             tag - If you use specify_name, then this is the default tag
                   to use.  You do not want to use this.

     calls - Array reference telling Net::XMPP what functions to create
             for this name.  For most of the types above you will get
             Get, Set, Defined, and Remove.  For child types you need to
             decide how you API will look and specify them yourself:

               ["Get","Defined"]
               ["Add"]
               ["Get","Add","Defined"]

            It all depends on how you want your API to look.

  Once more... The following:

    &add_ns(ns    => "mynamespace",
            tag   => "mytag",
            xpath => {
                      JID       => { type=>'jid', path => '@jid' },
                      Username  => { path => 'username/text()' },
                      Test      => { type => 'master' }
                     }
           );

  generates the following API calls:

    GetJID()
    SetJID()
    DefinedJID()
    RemoveJID()
    GetUsername()
    SetUsername()
    DefinedUsername()
    RemoveUsername()
    GetTest()
    SetTest()

=head2 Wrap Up

  Well.  I hope that I have not scared you off from writing a custom
  namespace for you application and use Net::XMPP.  Look in the
  Net::XMPP::Protocol manpage for an example on using the add_ns()
  function to register your custom namespace so that Net::XMPP can
  properly handle it.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use vars qw ( %NS %SKIPNS );

$SKIPNS{'__netxmpp__'} = 1;

#------------------------------------------------------------------------------
# __netxmpp__:child:test
#------------------------------------------------------------------------------
{
    &add_ns(ns    => "__netxmpptest__:child:test",
            tag   => "test",
            xpath => {
                      Bar  => { path => 'bar/text()' },
                      Foo  => { path => '@foo' },
                      Test => { type => 'master' }
                     }
           );
}

#------------------------------------------------------------------------------
# __netxmpp__:child:test:two
#------------------------------------------------------------------------------
{
    &add_ns(ns    => "__netxmpptest__:child:test:two",
            tag   => "test",
            xpath => {
                      Bob  => { path => 'owner/@bob' },
                      Joe  => { path => 'joe/text()' },
                      Test => { type => 'master' }
                     }
           );
}

#-----------------------------------------------------------------------------
# urn:ietf:params:xml:ns:xmpp-bind
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => "urn:ietf:params:xml:ns:xmpp-bind",
            tag   => "bind",
            xpath => {
                      JID      => { type => 'jid',
                                    path => 'jid/text()',
                                  },
                      Resource => { path => 'resource/text()' },
                      Bind     => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::XMPP',
                     },
           );
}

#-----------------------------------------------------------------------------
# urn:ietf:params:xml:ns:xmpp-session
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => "urn:ietf:params:xml:ns:xmpp-session",
            tag   => "session",
            xpath => { Session => { type => 'master' } },
            docs  => {
                      module => 'Net::XMPP',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:iq:auth
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => "jabber:iq:auth",
            tag   => "query",
            xpath => {
                      Digest   => { path => 'digest/text()' },
                      Hash     => { path => 'hash/text()' },
                      Password => { path => 'password/text()' },
                      Resource => { path => 'resource/text()' },
                      Sequence => { path => 'sequence/text()' },
                      Token    => { path => 'token/text()' },
                      Username => { path => 'username/text()' },
                      Auth     => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::XMPP',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:iq:privacy
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => "jabber:iq:privacy",
            tag   => "query",
            xpath => {
                      Active  => { path => 'active/@name' },
                      Default => { path => 'default/@name' },
                      List    => {
                                  type  => 'child',
                                  path  => 'list',
                                  child => { ns => '__netxmpp__:iq:privacy:list', },
                                  calls => [ 'Add' ],
                                 },
                      Lists   => {
                                  type  => 'child',
                                  path  => 'list',
                                  child => { ns => '__netxmpp__:iq:privacy:list', },
                                 },
                      Privacy => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::XMPP',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netxmpp__:iq:privacy:list
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netxmpp__:iq:privacy:list',
            xpath => {
                      Name  => { path => '@name' },
                      Item  => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netxmpp__:iq:privacy:list:item', },
                                calls => [ 'Add' ],
                               },
                      Items => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netxmpp__:iq:privacy:item', },
                               },
                      List  => { type => 'master' },
                    },
            docs  => {
                      module => 'Net::XMPP',
                      name   => 'jabber:iq:privacy - list objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netxmpp__:iq:privacy:list:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => '__netxmpp__:iq:privacy:list:item',
            xpath => {
                      Action      => { path => '@action' },
                      IQ          => {
                                      type => 'flag',
                                      path => 'iq',
                                     },
                      Message     => {
                                      type => 'flag',
                                      path => 'message',
                                     },
                      Order       => { path => '@order' },
                      PresenceIn  => {
                                      type => 'flag',
                                      path => 'presence-in',
                                     },
                      PresenceOut => {
                                      type => 'flag',
                                      path => 'presence-out',
                                     },
                      Type        => { path => '@type' },
                      Value       => { path => '@value' },
                      Item        => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::XMPP',
                      name   => 'jabber:iq:privacy - item objects',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:iq:register
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => "jabber:iq:register",
            tag   => "query",
            xpath => {
                      Address      => { path => 'address/text()' },
                      City         => { path => 'city/text()' },
                      Date         => { path => 'date/text()' },
                      Email        => { path => 'email/text()' },
                      First        => { path => 'first/text()' },
                      Instructions => { path => 'instructions/text()' },
                      Key          => { path => 'key/text()' },
                      Last         => { path => 'last/text()' },
                      Misc         => { path => 'misc/text()' },
                      Name         => { path => 'name/text()' },
                      Nick         => { path => 'nick/text()' },
                      Password     => { path => 'password/text()' },
                      Phone        => { path => 'phone/text()' },
                      Registered   => {
                                       type => 'flag',
                                       path => 'registered',
                                      },
                      Remove       => {
                                       type => 'flag',
                                       path => 'password/text()',
                                      },
                      State        => { path => 'state/text()' },
                      Text         => { path => 'text/text()' },
                      URL          => { path => 'url/text()' },
                      Username     => { path => 'username/text()' },
                      Zip          => { path => 'zip/text()' },
                      Register     => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::XMPP',
                     },
           );
}

#-----------------------------------------------------------------------------
# jabber:iq:roster
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => 'jabber:iq:roster',
            tag   => "query",
            xpath => {
                      Item  => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netxmpp__:iq:roster:item', },
                                calls => [ 'Add' ],
                               },
                      Items => {
                                type  => 'child',
                                path  => 'item',
                                child => { ns => '__netxmpp__:iq:roster:item', },
                                calls => [ 'Get' ],
                               },
                      Roster => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::XMPP',
                     },
           );
}

#-----------------------------------------------------------------------------
# __netxmpp__:iq:roster:item
#-----------------------------------------------------------------------------
{
    &add_ns(ns    => "__netxmpp__:iq:roster:item",
            xpath => {
                      Ask          => { path => '@ask' },
                      Group        => {
                                       type => 'array',
                                       path => 'group/text()',
                                      },
                      JID          => {
                                       type => 'jid',
                                       path => '@jid',
                                      },
                      Name         => { path => '@name' },
                      Subscription => { path => '@subscription' },
                      Item         => { type => 'master' },
                     },
            docs  => {
                      module => 'Net::XMPP',
                      name   => 'jabber:iq:roster - item objects',
                     },
           );
}



sub add_ns
{
    my (%args) = @_;

    # XXX error check...

    $NS{$args{ns}}->{tag} = $args{tag} if exists($args{tag});
    $NS{$args{ns}}->{xpath} = $args{xpath};
    if (exists($args{docs}))
    {
        $NS{$args{ns}}->{docs} = $args{docs};
        $NS{$args{ns}}->{docs}->{name} = $args{ns}
            unless exists($args{docs}->{name});
    }
}


1;
