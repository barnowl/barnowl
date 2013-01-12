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

package Net::XMPP::PrivacyLists;

=head1 NAME

Net::XMPP::PrivacyLists - XMPP Privacy Lists Object

=head1 SYNOPSIS

  This module is not yet complete.  Do not use.

=head1 DESCRIPTION

=head2 Basic Functions

=head2 Advanced Functions

=head1 METHODS

=head2 Basic Functions

=head2 Advanced Functions

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use Carp;

sub new
{
    my $proto = shift;
    my $self = { };

    my %args;
    while($#_ >= 0) { $args{ lc(pop(@_)) } = pop(@_); }

    $self->{CONNECTION} = $args{connection};

    bless($self, $proto);

    $self->init();

    return $self;
}


##############################################################################
#
# init - initialize the module to use the privacy lists.
#
##############################################################################
sub init
{
    my $self = shift;

    $self->{CONNECTION}-> SetXPathCallBacks('/iq[@type="result" or @type="set"]/query[@xmlns="jabber:iq:privacy"]'=>sub{ $self->handler(@_) });
}


##############################################################################
#
# debug - print out a representation of the privacy lists.
#
##############################################################################
sub debug
{
    my $self = shift;

    &XML::Stream::printData("\$self->{LISTS}",$self->{LISTS});
}


##############################################################################
#
# addItem - add list item to a list.
#
##############################################################################
sub addItem
{
    my $self = shift;
    my ($list,%item) = @_;

    my $order = delete($item{order});
    $self->{LISTS}->{$list}->{$order} = \%item;
}


###############################################################################
#
# clear - delete all of the JIDs from the DB completely.
#
###############################################################################
sub clear
{
    my $self = shift;

    $self->{CONNECTION}->{DEBUG}->Log3("PrivacyLists::clear: clearing the database");
    foreach my $list ($self->lists())
    {
        $self->remove($list);
    }
    $self->{CONNECTION}->{DEBUG}->Log3("PrivacyLists::clear: database is empty");
}


##############################################################################
#
# exists - allows you to query if the JID exists in the Roster DB.
#
##############################################################################
sub exists
{
    my $self = shift;
    my $list = shift;

    return unless exists($self->{LISTS});
    return unless exists($self->{LISTS}->{$list});
    return 1;
}


##############################################################################
#
# fetch - fetch the privacy lists from the server and populate the database.
#
##############################################################################
sub fetch
{
    my $self = shift;

    my $iq = $self->{CONNECTION}->PrivacyListsGet();
    $self->handleIQ($iq);
}


##############################################################################
#
# fetchList - fetch the privacy list from the server and populate the database.
#
##############################################################################
sub fetchList
{
    my $self = shift;
    my $list = shift;

    my $iq = $self->{CONNECTION}->PrivacyListsGet(list=>$list);
    $self->handleIQ($iq);
}


##############################################################################
#
# lists - returns a list of the current privacy lists.
#
##############################################################################
sub lists
{
    my $self = shift;

    return () unless exists($self->{LISTS});
    return () if (scalar(keys(%{$self->{LISTS}})) == 0);
    return keys(%{$self->{LISTS}});
}


##############################################################################
#
# items - returns a list of all of the items in the specified privacy list.
#
##############################################################################
sub items
{
    my $self = shift;
    my $list = shift;

    my @items;

    return () unless $self->exists($list);
    foreach my $order (sort{ $a <=> $b } keys(%{$self->{LISTS}->{$list}}))
    {
        my %item = %{$self->{LISTS}->{$list}->{$order}};
        $item{order} = $order;
        push(@items,\%item);
    }

    return @items;
}


##############################################################################
#
# handler - takes a packet and calls the correct handler.
#
##############################################################################
sub handler
{
    my $self = shift;
    my $sid = shift;
    my $packet = shift;

    $self->handleIQ($packet) if ($packet->GetTag() eq "iq");
}


##############################################################################
#
# handleIQ - takes an iq packet that contains roster, parses it, and puts
#            the roster into the Roster DB.
#
##############################################################################
sub handleIQ
{
    my $self = shift;
    my $iq = shift;

    print "handleIQ: iq(",$iq->GetXML(),")\n";

    my $type = $iq->GetType();
    return unless (($type eq "set") || ($type eq "result"));

    if ($type eq "result")
    {
        my $query = $iq->GetChild("jabber:iq:privacy");

        my @lists = $query->GetLists();

        return unless ($#lists > -1);

        my @items = $lists[0]->GetItems();

        if (($#lists == 0) && ($#items > -1))
        {
            $self->parseList($lists[0]);
        }
        elsif ($#lists >= -1)
        {
            $self->parseLists(\@lists);
        }
    }
}


sub parseList
{
    my $self = shift;
    my $list = shift;

    my $name = $list->GetName();

    foreach my $item ($list->GetItems())
    {
        my %item = $item->GetItem();

        $self->addItem($name,%item);
    }
}


sub parseLists
{
    my $self = shift;
    my $lists = shift;

    foreach my $list (@{$lists})
    {
        my $name = $list->GetName();
        $self->fetchList($name);
    }
}


##############################################################################
#
# reload - clear and refetch the privacy lists.
#
##############################################################################
sub reload
{
    my $self = shift;

    $self->clear();
    $self->fetch();
}


##############################################################################
#
# remove - removes the list from the database.
#
##############################################################################
sub remove
{
    my $self = shift;
    my $list = shift;

    if ($self->exists($list))
    {
        $self->{CONNECTION}->{DEBUG}->Log3("PrivacyLists::remove: deleting $list from the DB");

        delete($self->{LISTS}->{$list});
        delete($self->{LISTS}) if (scalar(keys(%{$self->{LISTS}})) == 0);
    }
}


sub save
{
    my $self = shift;

    foreach my $list ($self->lists())
    {
        $self->saveList($list);
    }
}


sub saveList
{
    my $self = shift;
    my $list = shift;

    my @items = $self->items($list);
    $self->{CONNECTION}->PrivacyListsSet(list=>$list,
                                         items=>\@items);
}


1;

