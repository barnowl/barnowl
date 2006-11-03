###############################################################################
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
###############################################################################

package Net::Jabber;

=head1 NAME

Net::Jabber - Jabber Perl Library

=head1 SYNOPSIS

  Net::Jabber provides a Perl user with access to the Jabber Instant
  Messaging protocol.

  For more information about Jabber visit:

    http://www.jabber.org

=head1 DESCRIPTION

  Net::Jabber is a convenient tool to use for any perl script that would
  like to utilize the Jabber Instant Messaging protocol.  While not a
  client in and of itself, it provides all of the necessary back-end
  functions to make a CGI client or command-line perl client feasible and
  easy to use.  Net::Jabber is a wrapper around the rest of the official
  Net::Jabber::xxxxxx packages.

  There is are example scripts in the example directory that provide you
  with examples of very simple Jabber programs.


  NOTE: The parser that XML::Stream::Parser provides, as are most Perl
  parsers, is synchronous.  If you are in the middle of parsing a packet
  and call a user defined callback, the Parser is blocked until your
  callback finishes.  This means you cannot be operating on a packet,
  send out another packet and wait for a response to that packet.  It
  will never get to you.  Threading might solve this, but as of the
  writing of this, threading in Perl is not quite up to par yet.  This
  issue will be revisted in the future.

=head1 EXAMPLES

    For a client:
      use Net::Jabber;
      my $client = new Net::Jabber::Client();

    For a component:
      use Net::Jabber;
      my $component = new Net::Jabber::Component();

=head1 METHODS

  The Net::Jabber module does not define any methods that you will call
  directly in your code.  Instead you will instantiate objects that call
  functions from this module to do work.  The three main objects that
  you will work with are the Message, Presence, and IQ modules.  Each one
  corresponds to the Jabber equivilant and allows you get and set all
  parts of those packets.

=head1 PACKAGES

  For more information on each of these packages, please see the man page
  for each one.

=head2 Net::Jabber::Client

  This package contains the code needed to communicate with a Jabber
  server: login, wait for messages, send messages, and logout.  It uses
  XML::Stream to read the stream from the server and based on what kind
  of tag it encounters it calls a function to handle the tag.

=head2 Net::Jabber::Component

  This package contains the code needed to write a server component.  A
  component is a program tha handles the communication between a jabber
  server and some outside program or communications pacakge (IRC, talk,
  email, etc...)  With this module you can write a full component in just
  a few lines of Perl.  It uses XML::Stream to communicate with its host
  server and based on what kind of tag it encounters it calls a function
  to handle the tag.

=head2 Net::Jabber::Protocol

  A collection of high-level functions that Client and Component use to
  make their lives easier through inheritance.

=head2 Net::Jabber::JID

  The Jabber IDs consist of three parts: user id, server, and resource.
  This module gives you access to those components without having to
  parse the string yourself.

=head2 Net::Jabber::Message

  Everything needed to create and read a <message/> received from the
  server.

=head2 Net::Jabber::Presence

  Everything needed to create and read a <presence/> received from the
  server.

=head2 Net::Jabber::IQ

  IQ is a wrapper around a number of modules that provide support for the
  various Info/Query namespaces that Jabber recognizes.

=head2 Net::Jabber::Stanza

  This module represents a namespaced stanza that is used to extend a
  <message/>, <presence/>, and <iq/>.  Ultimately each namespace is
  documented in a JEP of some kind.  http://jabber.org/jeps/

  The man page for Net::Jabber::Stanza contains a listing of all
  supported namespaces, and the methods that are supported by the objects
  that represent those namespaces.

=head2 Net::Jabber::Namespaces

  Jabber allows for any stanza to be extended by any bit of XML.  This
  module contains all of the internals for defining the Jabber based
  extensions defined by the JEPs.  The documentation for this module
  explains more about how to add your own custom namespace and have it be
  supported.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software, you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.005;
use strict;
use Carp;
use POSIX;
use Net::XMPP 1.0;

use base qw( Net::XMPP );

use vars qw( $VERSION );

$VERSION = "2.0";

use Net::Jabber::Debug;
use Net::Jabber::JID;
use Net::Jabber::Namespaces;
use Net::Jabber::Stanza;
use Net::Jabber::Message;
use Net::Jabber::IQ;
use Net::Jabber::Presence;
use Net::Jabber::Protocol;
use Net::Jabber::Client;
use Net::Jabber::Component;

sub GetTimeStamp { return &Net::XMPP::GetTimeStamp(@_); }
sub printData    { return &Net::XMPP::printData(@_);    }
sub sprintData   { return &Net::XMPP::sprintData(@_);   }

1;
