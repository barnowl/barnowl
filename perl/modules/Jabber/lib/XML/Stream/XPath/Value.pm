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

package XML::Stream::XPath::Value;

use 5.006_001;
use strict;
use vars qw( $VERSION );

$VERSION = "1.22";

sub new
{
    my $proto = shift;
    my $self = { };

    bless($self,$proto);

    $self->setList(@_);
    $self->setValues();
    $self->setAttribs();
    $self->setValid(0);
    $self->in_context(0);

    return $self;
}


sub setList
{
    my $self = shift;
    my (@values) = @_;
    $self->{LIST} = \@values;
}


sub getList
{
    my $self = shift;
    return unless ($#{$self->{LIST}} > -1);
    return @{$self->{LIST}};
}


sub getFirstElem
{
    my $self = shift;
    return unless ($#{$self->{LIST}} > -1);
    return $self->{LIST}->[0];
}


sub setValues
{
    my $self = shift;
    my (@values) = @_;
    $self->{VALUES} = \@values;
}


sub getValues
{
    my $self = shift;
    return unless ($#{$self->{VALUES}} > -1);
    return $self->{VALUES}->[0] if !wantarray;
    return @{$self->{VALUES}};
}


sub setAttribs
{
    my $self = shift;
    my (%attribs) = @_;
    $self->{ATTRIBS} = \%attribs;
}


sub getAttribs
{ 
    my $self = shift;
    return unless (scalar(keys(%{$self->{ATTRIBS}})) > 0);
    return %{$self->{ATTRIBS}};
}


sub setValid
{
    my $self = shift;
    my $valid = shift;
    $self->{VALID} = $valid;
}


sub check
{
    my $self = shift;
    return $self->{VALID};
}


sub in_context
{
    my $self = shift;
    my $in_context = shift;

    if (defined($in_context))
    {
        $self->{INCONTEXT} = $in_context;
    }
    return $self->{INCONTEXT};
}


sub display
{
    my $self = shift;
    if (0)
    {
        print "VALUE: list(",join(",",@{$self->{LIST}}),")\n";
    }
    else
    {
        print "VALUE: list(\n";
        foreach my $elem (@{$self->{LIST}})
        {
            print "VALUE:      ",$elem->GetXML(),"\n";
        }
        print "VALUE:     )\n";
    }
    print "VALUE: values(",join(",",@{$self->{VALUES}}),")\n";
}

1;

