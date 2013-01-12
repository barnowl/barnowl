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
#  Copyright (C) 1998-2004 Jabber Software Foundation http://jabber.org/
#
##############################################################################

package XML::Stream::Namespace;

=head1 NAME

XML::Stream::Namespace - Object to make defining Namespaces easier in
                         XML::Stream.

=head1 SYNOPSIS

XML::Stream::Namespace is a helper package to XML::Stream.  It provides
a clean way of defining Namespaces for XML::Stream to use when connecting.

=head1 DESCRIPTION

  This module allows you to set and read elements from an XML::Stream
  Namespace.

=head1 METHODS

  SetNamespace("mynamespace");
  SetXMLNS("http://www.mynamespace.com/xmlns");
  SetAttributes(attrib1=>"value1",
                attrib2=>"value2");

  GetNamespace() returns "mynamespace"
  GetXMLNS() returns "http://www.mynamespace.com/xmlns"
  GetAttributes() returns a hash ( attrib1=>"value1",attrib2=>"value2")
  GetStream() returns the following string:
    "xmlns:mynamespace='http://www.nynamespace.com/xmlns'
     mynamespace:attrib1='value1'
     mynamespace:attrib2='value2'"

=head1 EXAMPLES


  $myNamespace = new XML::Stream::Namespace("mynamspace");
  $myNamespace->SetXMLNS("http://www.mynamespace.org/xmlns");
  $myNamespace->SetAttributes(foo=>"bar",
                              bob=>"vila");

  $stream = new XML::Stream;
  $stream->Connect(name=>"foo.bar.org",
                   port=>1234,
                   namespace=>"foo:bar",
                   namespaces=>[ $myNamespace ]);

  #
  # The above Connect will send the following as the opening string
  # of the stream to foo.bar.org:1234...
  #
  #   <stream:stream
  #    xmlns:stream="http://etherx.jabber.org/streams"
  #    to="foo.bar.org"
  #    xmlns="foo:bar"
  #    xmlns:mynamespace="http://www.mynamespace.org/xmlns"
  #    mynamespace:foo="bar"
  #    mynamespace:bob="vila">
  #


=head1 AUTHOR

Written by Ryan Eatmon in February 2000
Idea By Thomas Charron in January of 2000 for http://etherx.jabber.org/streams/

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use Carp;
use vars qw( $VERSION );

$VERSION = "1.22";

sub new
{
  my $proto = shift;
  my $self = { };

  ($self->{Namespace}) = @_ if ($#_ > -1);

  $self->{Attributes} = {};

  bless($self,$proto);
  return $self;
}


sub SetNamespace
{
  my $self = shift;
  my ($namespace) = @_;

  $self->{Namespace} = $namespace;
}


sub SetXMLNS
{
  my $self = shift;
  my ($xmlns) = @_;

  $self->{XMLNS} = $xmlns;
}


sub SetAttributes
{
  my $self = shift;
  my %att = @_;

  my $key;
  foreach $key (keys(%att))
  {
    $self->{Attributes}->{$key} = $att{$key};
  }
}


sub GetNamespace
{
  my $self = shift;

  return $self->{Namespace};
}

sub GetXMLNS
{
  my $self = shift;

  return $self->{XMLNS};
}

sub GetAttributes
{
  my $self = shift;
  my ($attrib) = @_;

  return $self->{Attributes} if ($attrib eq "");
  return $self->{Attributes}->{$attrib};
}


sub GetStream
{
  my $self = shift;

  my $string = "";

  $string .= "xmlns:".$self->GetNamespace();
  $string .= "='".$self->GetXMLNS()."'";
  my $attrib;
  foreach $attrib (keys(%{$self->GetAttributes()}))
  {
    $string .= " ".$self->GetNamespace().":";
    $string .= $attrib;
    $string .= "='".$self->GetAttributes($attrib)."'";
  }

  return $string;
}

1;

