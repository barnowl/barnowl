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

package Net::XMPP::Roster;

=head1 NAME

Net::XMPP::Roster - XMPP Roster Object

=head1 SYNOPSIS

  Net::XMPP::Roster is a module that provides a developer an easy
  interface to an XMPP roster.  It provides high level functions to
  query, update, and manage a user's roster.

=head1 DESCRIPTION

  The Roster object seeks to provide an easy to use API for interfacing
  with a user's roster.  When you instantiate it, it automatically
  registers with the connection to receivce the correct packets so
  that it can track all roster updates, and presence packets.

=head2 Basic Functions

  my $Client = Net::XMPP::Client->new(...);

  my $Roster = Net::XMPP::Roster->new(connection=>$Client);
    or
  my $Roster = $Client->Roster();

  $Roster->clear();

  if ($Roster->exists('bob@jabber.org')) { ... }
  if ($Roster->exists(Net::XMPP::JID)) { ... }

  if ($Roster->groupExists("Friends")) { ... }

  my @groups = $Roster->groups();

  my @jids    = $Roster->jids();
  my @friends = $Roster->jids("group","Friends");
  my @unfiled = $Roster->jids("nogroup");

  if ($Roster->online('bob@jabber.org')) { ... }
  if ($Roster->online(Net::XMPP::JID)) { ... }

  my %hash = $Roster->query('bob@jabber.org');
  my %hash = $Roster->query(Net::XMPP::JID);

  my $name = $Roster->query('bob@jabber.org',"name");
  my $ask  = $Roster->query(Net::XMPP::JID,"ask");

  my $resource = $Roster->resource('bob@jabber.org');
  my $resource = $Roster->resource(Net::XMPP::JID);

  my %hash = $Roster->resourceQuery('bob@jabber.org',"Home");
  my %hash = $Roster->resourceQuery(Net::XMPP::JID,"Club");

  my $show   = $Roster->resourceQuery('bob@jabber.org',"Home","show");
  my $status = $Roster->resourceQuery(Net::XMPP::JID,"Work","status");

  my @resource = $Roster->resources('bob@jabber.org');
  my @resource = $Roster->resources(Net::XMPP::JID);

  $Roster->resourceStore('bob@jabber.org',"Home","gpgkey",key);
  $Roster->resourceStore(Net::XMPP::JID,"logged on","2004/04/07 ...");

  $Roster->store('bob@jabber.org',"avatar",avatar);
  $Roster->store(Net::XMPP::JID,"display_name","Bob");

=head2 Advanced Functions

  These functions are only needed if you want to manually control
  the Roster.

  $Roster->add('bob@jabber.org',
               name=>"Bob",
               groups=>["Friends"]
              );
  $Roster->add(Net::XMPP::JID);

  $Roster->addResource('bob@jabber.org',
                       "Home",
                       show=>"dnd",
                       status=>"Working"
                      );
  $Roster->addResource(Net::XMPP::JID,"Work");

  $Roster->remove('bob@jabber.org');
  $Roster->remove(Net::XMPP::JID);

  $Roster->removeResource('bob@jabber.org',"Home");
  $Roster->removeResource(Net::XMPP::JID,"Work");

  $Roster->handler(Net::XMPP::IQ);
  $Roster->handler(Net::XMPP::Presence);

=head1 METHODS

=head2 Basic Functions


  new(connection=>object) - This creates and initializes the Roster
                            object.  The connection object is required
                            so that the Roster can interact with the
                            main connection object.  It needs to be an
                            object that inherits from
                            Net::XMPP::Connection.

  clear() - removes everything from the database.

  exists(jid) - return 1 if the JID exists in the database, undef
                otherwise.  The jid can either be a string, or a
                Net::XMPP::JID object.

  groupExists(group) - return 1 if the group exists in the database,
                       undef otherwise.

  groups() - returns a list of all of the roster groups.

  jids([type,    - returns a list of all of the matching JIDs.  The valid
       [group]])   types are:

                    all     - return all JIDs in the roster. (default)
                    nogroup - return all JIDs not in a roster group.
                    group   - return all of the JIDs in the specified
                              roster group.

  online(jid) - return 1 if the JID is online, undef otherwise.  The
                jid can either be a string, or a Net::XMPP::JID object.

  query(jid,   - return a hash representing all of the data in the
        [key])   DB for this JID.  The jid can either be a string,
                 or a Net::XMPP::JID object.  If you specify a key,
                 then only the value for that key is returned.

  resource(jid) - return the string representing the resource with the
                  highest priority for the JID.  The jid can either be
                  a string, or a Net::XMPP::JID object.

  resourceQuery(jid,      - return a hash representing all of the data
                resource,   the DB for the resource for this JID.  The
                [key])      jid can either be a string, or a
                            Net::XMPP::JID object.  If you specify a
                            key, then only the value for that key is
                            returned.

  resources(jid) - returns the list of resources for the JID in order
                   of highest priority to lowest priority.  The jid can
                   either be a string, or a Net::XMPP::JID object.

  resourceStore(jid,      - store the specified value in the DB under
                resource,   the specified key for the resource for this
                key,        JID.  The jid can either be a string, or a
                value)      Net::XMPP::JID object.

  store(jid,      - store the specified value in the DB under the
        key,        specified key for this JID.  The jid can either
        value)      be a string, or a Net::XMPP::JID object.



=head2 Advanced Functions

add(jid,                 - Manually adds the JID to the Roster with the
    ask=>string,           specified roster item settings.  This does not
    groups=>arrayref       handle subscribing to other users, only
    name=>string,          manipulating the Roster object.  The jid
    subscription=>string)  can either be a string or a Net::XMPP::JID.

addResource(jid,            - Manually add the resource to the JID in the
            resource,         Roster with the specified presence settings.
            priority=>int,    This does not handle subscribing to other
            show=>string,     users, only manipulating the Roster object.
            status=>string)   The jid can either be a string or a
                              Net::XMPP::JID.

remove(jid) - Removes all reference to the JID from the Roster object.
              The jid can either be a string or a Net::XMPP::JID.

removeResource(jid,      - Removes the resource from the jid in the
               resource)   Roster object.  The jid can either be a string
                           or a Net::XMPP::JID.

handler(packet) - Take either a Net::XMPP::IQ or Net::XMPP::Presence
                  packet and parse them according to the rules of the
                  Roster object.  Note, that it will only waste CPU time
                  if you pass in IQs or Presences that are not roster
                  related.

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

    if (!exists($args{connection}) ||
        !$args{connection}->isa("Net::XMPP::Connection"))
    {
        croak("You must pass Net::XMPP::Roster a valid connection object.");
    }

    $self->{CONNECTION} = $args{connection};

    bless($self, $proto);

    $self->init();

    return $self;
}


##############################################################################
#
# init - initialize the module to use the roster database
#
##############################################################################
sub init
{
    my $self = shift;

    $self->{CONNECTION}-> SetXPathCallBacks('/iq[@type="result" or @type="set"]/query[@xmlns="jabber:iq:roster"]'=>sub{ $self->handler(@_) });
    $self->{CONNECTION}-> SetXPathCallBacks('/presence'=>sub{ $self->handler(@_) });
}


##############################################################################
#
# add - adds the entry to the Roster DB.
#
##############################################################################
sub add
{
    my $self = shift;
    my ($jid,%item) = @_;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");
    if (exists $self->{JIDS}->{$jid})
    {
        foreach my $key (keys %item)
        {
            $self->{JIDS}->{$jid}->{$key} = $item{$key};
        }
    }
    else
    {
        $self->{JIDS}->{$jid} = \%item;
    }

    foreach my $group (keys %{ $self->{GROUPS} })
    {
        # Clear user from old groups.
        delete $self->{GROUPS}->{$group}->{$jid};
        # Clean up empty groups.
        delete $self->{GROUPS}->{$group} unless scalar keys %{ $self->{GROUPS}->{$group} };
    }

    foreach my $group (@{$item{groups}})
    {
        $self->{GROUPS}->{$group}->{$jid} = 1;
    }
}



##############################################################################
#
# addResource - adds the resource to the JID in the Roster DB.
#
##############################################################################
sub addResource
{
    my $self = shift;
    my $jid = shift;
    my $resource = shift;
    my (%item) = @_;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    my $priority = $item{priority};
    $priority = 0 unless defined($priority);

    $self->{CONNECTION}->{DEBUG}->Log3("Roster::addResource: add $jid/$resource with priority $priority to the DB");

    my $loc = -1;
    $self->{JIDS}->{$jid}->{priorities}->{$priority} = []
        unless exists($self->{JIDS}->{$jid}->{priorities}->{$priority});
    foreach my $index (0..$#{$self->{JIDS}->{$jid}->{priorities}->{$priority}})
    {
        $loc = $index
            if ($self->{JIDS}->{$jid}->{priorities}->{$priority}->[$index]->{resource} eq $resource);
    }
    $loc = $#{$self->{JIDS}->{$jid}->{priorities}->{$priority}} + 1 if ($loc == -1);

    $self->{JIDS}->{$jid}->{resources}->{$resource}->{priority} = $priority;
    $self->{JIDS}->{$jid}->{resources}->{$resource}->{status} = $item{status}
        if exists($item{status});
    $self->{JIDS}->{$jid}->{resources}->{$resource}->{show} = $item{show}
        if exists($item{show});
    $self->{JIDS}->{$jid}->{priorities}->{$priority}->[$loc]->{resource} = $resource;
}


###############################################################################
#
# clear - delete all of the JIDs from the DB completely.
#
###############################################################################
sub clear
{
    my $self = shift;

    $self->{CONNECTION}->{DEBUG}->Log3("Roster::clear: clearing the database");
    foreach my $jid ($self->jids())
    {
        $self->remove($jid);
    }
    $self->{CONNECTION}->{DEBUG}->Log3("Roster::clear: database is empty");
}


##############################################################################
#
# exists - allows you to query if the JID exists in the Roster DB.
#
##############################################################################
sub exists
{
    my $self = shift;
    my ($jid) = @_;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless exists($self->{JIDS});
    return unless exists($self->{JIDS}->{$jid});
    return 1;
}


sub fetch
{
    my $self = shift;

    my %newroster = $self->{CONNECTION}->RosterGet();

    $self->handleRoster(\%newroster);
}


##############################################################################
#
# groupExists - allows you to query if the group exists in the Roster
#                       DB.
#
##############################################################################
sub groupExists
{
    my $self = shift;
    my ($group) = @_;

    return unless exists($self->{GROUPS});
    return unless exists($self->{GROUPS}->{$group});
    return 1;
}


##############################################################################
#
# groups - returns a list of the current groups in your roster.
#
##############################################################################
sub groups
{
    my $self = shift;

    return () unless exists($self->{GROUPS});
    return () if (scalar(keys(%{$self->{GROUPS}})) == 0);
    return keys(%{$self->{GROUPS}});
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
    $self->handlePresence($packet) if ($packet->GetTag() eq "presence");
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

    my $type = $iq->GetType();
    return unless (($type eq "set") || ($type eq "result"));

    my %newroster = $self->{CONNECTION}->RosterParse($iq);

    $self->handleRoster(\%newroster);
}


sub handleRoster
{
    my $self = shift;
    my $roster = shift;

    foreach my $jid (keys(%{$roster}))
    {
        if ($roster->{$jid}->{subscription} ne "remove")
        {
            $self->add($jid, %{$roster->{$jid}});
        }
        else
        {
            $self->remove($jid);
        }
    }
}


##############################################################################
#
# handlePresence - takes a presence packet and groks the presence.
#
##############################################################################
sub handlePresence
{
    my $self = shift;
    my $presence = shift;

    my $type = $presence->GetType();
    $type = "" unless defined($type);
    return unless (($type eq "") ||
                   ($type eq "available") ||
                   ($type eq "unavailable"));

    my $jid = $presence->GetFrom("jid");

    my $resource = $jid->GetResource();
    $resource = " " unless ($resource ne "");

    $jid = $jid->GetJID();
    $jid = "" unless defined($jid);

    return unless $self->exists($jid);
    #XXX if it doesn't exist... is it us?
    #XXX is this a presence based roster?

    $self->{CONNECTION}->{DEBUG}->Log3("Roster::PresenceDBParse: fromJID(",$presence->GetFrom(),") resource($resource) type($type)");
    $self->{CONNECTION}->{DEBUG}->Log4("Roster::PresenceDBParse: xml(",$presence->GetXML(),")");

    $self->removeResource($jid,$resource);

    if (($type eq "") || ($type eq "available"))
    {
        my %item;

        $item{priority} = $presence->GetPriority();
        $item{priority} = 0 unless defined($item{priority});

        $item{show} = $presence->GetShow();
        $item{show} = "" unless defined($item{show});

        $item{status} = $presence->GetStatus();
        $item{status} = "" unless defined($item{status});

        $self->addResource($jid,$resource,%item);
    }
}


##############################################################################
#
# jids - returns a list of all of the JIDs in your roster.
#
##############################################################################
sub jids
{
    my $self = shift;
    my $type = shift;
    my $group = shift;

    $type = "all" unless defined($type);

    my @jids;

    if (($type eq "all") || ($type eq "nogroup"))
    {
        return () unless exists($self->{JIDS});
        foreach my $jid (keys(%{$self->{JIDS}}))
        {
            next if (($type eq "nogroup") &&
                     exists($self->{JIDS}->{$jid}->{groups}) &&
                     ($#{$self->{JIDS}->{$jid}->{groups}} > -1));

            push(@jids, Net::XMPP::JID->new($jid));
        }
    }

    if ($type eq "group")
    {
        return () unless exists($self->{GROUPS});
        if (defined($group) && $self->groupExists($group))
        {
            foreach my $jid (keys(%{$self->{GROUPS}->{$group}}))
            {
                push(@jids, Net::XMPP::JID->new($jid));
            }
        }
    }

    return @jids;
}


###############################################################################
#
# online - returns if the jid is online or not.
#
###############################################################################
sub online
{
    my $self = shift;
    my $jid = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless $self->exists($jid);

    my @resources = $self->resources($jid);

    return ($#resources > -1);
}


##############################################################################
#
# priority - return the highest priority for the jid, or for the specified
#            resource.
#
##############################################################################
sub priority
{
    my $self = shift;
    my $jid = shift;
    my $resource = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    if (defined($resource))
    {
        return unless $self->resourceExists($jid,$resource);
        return unless exists($self->{JIDS}->{$jid}->{resources}->{$resource}->{priority});
        return $self->{JIDS}->{$jid}->{resources}->{$resource}->{priority};
    }

    return unless exists($self->{JIDS}->{$jid}->{priorities});
    my @priorities = sort{ $b <=> $a } keys(%{$self->{JIDS}->{$jid}->{priorities}});
    return $priorities[0];
}


##############################################################################
#
# query - allows you to get one of the pieces of info from the Roster DB.
#
##############################################################################
sub query
{
    my $self = shift;
    my $jid = shift;
    my $key = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless $self->exists($jid);
    if (defined($key))
    {
        return unless exists($self->{JIDS}->{$jid}->{$key});
        return $self->{JIDS}->{$jid}->{$key};
    }
    return %{$self->{JIDS}->{$jid}};
}


##############################################################################
#
# remove - removes the JID from the Roster DB.
#
##############################################################################
sub remove
{
    my $self = shift;
    my $jid = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    if ($self->exists($jid))
    {
        $self->{CONNECTION}->{DEBUG}->Log3("Roster::remove: deleting $jid from the DB");

        if (defined($self->query($jid,"groups")))
        {
            foreach my $group (@{$self->query($jid,"groups")})
            {
                delete($self->{GROUPS}->{$group}->{$jid});
                delete($self->{GROUPS}->{$group})
                    if (scalar(keys(%{$self->{GROUPS}->{$group}})) == 0);
                delete($self->{GROUPS})
                    if (scalar(keys(%{$self->{GROUPS}})) == 0);
            }
        }

        delete($self->{JIDS}->{$jid});
        delete($self->{JIDS}) if (scalar(keys(%{$self->{JIDS}})) == 0);
    }
}


##############################################################################
#
# removeResource - removes the resource from the JID from the Roster DB.
#
##############################################################################
sub removeResource
{
    my $self = shift;
    my $jid = shift;
    my $resource = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    if ($self->resourceExists($jid,$resource))
    {
        $self->{CONNECTION}->{DEBUG}->Log3("Roster::removeResource: remove $jid/$resource from the DB");

        my $oldPriority = $self->priority($jid,$resource);
        $oldPriority = "" unless defined($oldPriority);

        if (exists($self->{JIDS}->{$jid}->{priorities}->{$oldPriority}))
        {
            my $loc = 0;
            foreach my $index (0..$#{$self->{JIDS}->{$jid}->{priorities}->{$oldPriority}})
            {
                $loc = $index
                    if ($self->{JIDS}->{$jid}->{priorities}->{$oldPriority}->[$index]->{resource} eq $resource);
            }

            splice(@{$self->{JIDS}->{$jid}->{priorities}->{$oldPriority}},$loc,1);

            delete($self->{JIDS}->{$jid}->{priorities}->{$oldPriority})
                if (exists($self->{JIDS}->{$jid}->{priorities}->{$oldPriority}) &&
                    ($#{$self->{JIDS}->{$jid}->{priorities}->{$oldPriority}} == -1));
        }

        delete($self->{JIDS}->{$jid}->{resources}->{$resource});

    }
}


###############################################################################
#
# resource - retrieve the resource with the highest priority.
#
###############################################################################
sub resource
{
    my $self = shift;
    my $jid = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless $self->exists($jid);

    my $priority = $self->priority($jid);

    return unless defined($priority);

    return $self->{JIDS}->{$jid}->{priorities}->{$priority}->[0]->{resource};
}


##############################################################################
#
# resourceExists - check that the specified resource exists.
#
##############################################################################
sub resourceExists
{
    my $self = shift;
    my $jid = shift;
    my $resource = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless $self->exists($jid);
    return unless exists($self->{JIDS}->{$jid}->{resources});
    return unless exists($self->{JIDS}->{$jid}->{resources}->{$resource});
}


##############################################################################
#
# resourceQuery - allows you to get one of the pieces of info from the Roster
#                 DB.
#
##############################################################################
sub resourceQuery
{
    my $self = shift;
    my $jid = shift;
    my $resource = shift;
    my $key = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless $self->resourceExists($jid,$resource);
    if (defined($key))
    {
        return unless exists($self->{JIDS}->{$jid}->{resources}->{$resource}->{$key});
        return $self->{JIDS}->{$jid}->{resources}->{$resource}->{$key};
    }
    return %{$self->{JIDS}->{$jid}->{resources}->{$resource};}
}


###############################################################################
#
# resources - returns a list of the resources from highest priority to lowest.
#
###############################################################################
sub resources
{
    my $self = shift;
    my $jid = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return () unless $self->exists($jid);

    my @resources;

    foreach my $priority (sort {$b cmp $a} keys(%{$self->{JIDS}->{$jid}->{priorities}}))
    {
        foreach my $index (0..$#{$self->{JIDS}->{$jid}->{priorities}->{$priority}})
        {
            next if ($self->{JIDS}->{$jid}->{priorities}->{$priority}->[$index]->{resource} eq " ");
            push(@resources,$self->{JIDS}->{$jid}->{priorities}->{$priority}->[$index]->{resource});
        }
    }
    return @resources;
}


##############################################################################
#
# resourceStore - allows you to store anything on the item that you want to.
#                 The only drawback is that when the item is removed, the data
#                 is not kept.  You must restore it in the DB.
#
##############################################################################
sub resourceStore
{
    my $self = shift;
    my $jid = shift;
    my $resource = shift;
    my $key = shift;
    my $value = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless defined($key);
    return unless defined($value);
    return unless $self->resourceExists($jid,$resource);

    $self->{JIDS}->{$jid}->{resources}->{$resource}->{$key} = $value;
}


##############################################################################
#
# store - allows you to store anything on the item that you want to.  The
#         only drawback is that when the item is removed, the data is not
#         kept.  You must restore it in the DB.
#
##############################################################################
sub store
{
    my $self = shift;
    my $jid = shift;
    my $key = shift;
    my $value = shift;

    $jid = $jid->GetJID() if UNIVERSAL::isa($jid,"Net::XMPP::JID");

    return unless defined($key);
    return unless defined($value);
    return unless $self->exists($jid);

    $self->{JIDS}->{$jid}->{$key} = $value;
}


1;

