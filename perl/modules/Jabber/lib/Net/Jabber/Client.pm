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

package Net::Jabber::Client;

=head1 NAME

Net::Jabber::Client - Jabber Client Library

=head1 SYNOPSIS

  Net::Jabber::Client is a module that provides a developer easy access
  to the Jabber Instant Messaging protocol.

=head1 DESCRIPTION

  Client.pm inherits its methods from Net::XMPP::Client,
  Net::XMPP::Protocol and Net::Jabber::Protocol.  The Protocol modules
  provide enough high level APIs and automation of the low level APIs
  that writing a Jabber Client in Perl is trivial.  For those that wish
  to work with the low level you can do that too, but those functions are
  covered in the documentation for each module.

  Net::Jabber::Client provides functions to connect to a Jabber server,
  login, send and receive messages, set personal information, create a
  new user account, manage the roster, and disconnect.  You can use all
  or none of the functions, there is no requirement.

  For more information on how the details for how Net::Jabber is written
  please see the help for Net::Jabber itself.

  For a full list of high level functions available please see:

    Net::XMPP::Client
    Net::XMPP::Protocol
    Net::Jabber::Protocol

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use Carp;
use Net::XMPP::Client;
use Net::Jabber::Protocol;
use base qw( Net::XMPP::Client Net::Jabber::Protocol );
use vars qw( $VERSION ); 

$VERSION = "2.0";

1;
