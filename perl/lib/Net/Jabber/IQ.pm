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

package Net::Jabber::IQ;

=head1 NAME

Net::Jabber::IQ - Jabber Info/Query Library

=head1 DESCRIPTION

  Net::Jabber::IQ inherits all of its methods from Net::XMPP::IQ.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.003;
use strict;
use Carp;
use Net::XMPP::IQ;
use base qw( Net::XMPP::IQ );

use vars qw( $VERSION );

$VERSION = "2.0";

sub GetQuery     { my $self = shift; $self->GetChild(@_);     }
sub DefinedQuery { my $self = shift; $self->DefinedChild(@_); }
sub NewQuery     { my $self = shift; $self->RemoveFirstChild(); $self->NewFirstChild(@_);     }
sub AddQuery     { my $self = shift; $self->AddChild(@_);     } 
sub RemoveQuery  { my $self = shift; $self->RemoveFirstChild(@_);  }

sub GetX     { my $self = shift; $self->GetChild(@_);     }
sub DefinedX { my $self = shift; $self->DefinedChild(@_); }
sub NewX     { my $self = shift; $self->NewChild(@_);     }
sub AddX     { my $self = shift; $self->AddChild(@_);     } 
sub RemoveX  { my $self = shift; $self->RemoveChild(@_);  }

sub _new_jid    { my $self = shift; return new Net::Jabber::JID(@_);    }
sub _new_packet { my $self = shift; return new Net::Jabber::Stanza(@_); }
sub _iq         { my $self = shift; return new Net::Jabber::IQ(@_);     }

1;
