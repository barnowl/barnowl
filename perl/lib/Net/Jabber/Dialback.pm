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

package Net::Jabber::Dialback;

=head1 NAME

Net::Jabber::Dialback - Jabber Dialback Module

=head1 SYNOPSIS

  Net::Jabber::Dialback is a companion to the Net::Jabber::Server
  module.  It provides the user a simple interface to set and retrieve
  all parts of a Jabber Server Dialback.

=head1 DESCRIPTION

  To initialize the Dialback with a Jabber <db:*/> you must pass it
  the XML::Stream hash.  For example:

    my $dialback = new Net::Jabber::Dialback(%hash);

  You now have access to all of the retrieval functions available.

  To create a new message to send to the server:

    use Net::Jabber qw(Server);

    $DB = new Net::Jabber::Dialback("verify");
    $DB = new Net::Jabber::Dialback("result");

  Please see the specific documentation for Net::Jabber::Dialback::Result
  and Net::Jabber::Dialback::Verify.

  For more information about the array format being passed to the
  CallBack please read the Net::Jabber::Client documentation.

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

use Net::Jabber::Dialback::Result;
($Net::Jabber::Dialback::Result::VERSION < $VERSION) &&
  die("Net::Jabber::Dialback::Result $VERSION required--this is only version $Net::Jabber::Dialback::Result::VERSION");

use Net::Jabber::Dialback::Verify;
($Net::Jabber::Dialback::Verify::VERSION < $VERSION) &&
  die("Net::Jabber::Dialback::Verify $VERSION required--this is only version $Net::Jabber::Dialback::Verify::VERSION");

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = { };

    bless($self, $proto);

    if ("@_" ne (""))
    {
        if (ref($_[0]) =~ /Net::Jabber::Dialback/)
        {
            return $_[0];
        }
        else
        {
            my ($temp) = @_;
            return new Net::Jabber::Dialback::Result()
                if ($temp eq "result");
            return new Net::Jabber::Dialback::Verify()
                if ($temp eq "verify");

            my @temp = @{$temp};
            return new Net::Jabber::Dialback::Result(@temp)
                if ($temp[0] eq "db:result");
            return new Net::Jabber::Dialback::Verify(@temp)
                if ($temp[0] eq "db:verify");
        }
    }
    else
    {
        carp "You must specify either \"result\" or \"verify\" as an argument";
    }
}

1;
