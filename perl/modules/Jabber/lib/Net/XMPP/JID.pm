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

package Net::XMPP::JID;

=head1 NAME

Net::XMPP::JID - XMPP JID Module

=head1 SYNOPSIS

  Net::XMPP::JID is a companion to the Net::XMPP module.
  It provides the user a simple interface to set and retrieve all
  parts of a Jabber ID (userid on a server).

=head1 DESCRIPTION

  To initialize the JID you must pass it the string that represents the
  jid from the XML packet.  Inside the XMPP modules this is done
  automatically and the JID object is returned instead of a string.
  For example, in the callback function for the XMPP object foo:

    use Net::XMPP;

    sub foo {
      my $foo = Net::XMPP::Foo->new(@_);
      my $from = $foo->GetFrom();
      my $JID = Net::XMPP::JID->new($from);
      .
      .
      .
    }

  You now have access to all of the retrieval functions available.

  To create a new JID to send to the server:

    use Net::XMPP;

    $JID = Net::XMPP::JID->new();

  Now you can call the creation functions below to populate the tag
  before sending it.

=head2 Retrieval functions

    $userid   = $JID->GetUserID();
    $server   = $JID->GetServer();
    $resource = $JID->GetResource();

    $JID      = $JID->GetJID();
    $fullJID  = $JID->GetJID("full");
    $baseJID  = $JID->GetJID("base");

=head2 Creation functions

    $JID->SetJID(userid=>"bob",
                 server=>"jabber.org",
                 resource=>"Work");

    $JID->SetJID('blue@moon.org/Home');

    $JID->SetUserID("foo");
    $JID->SetServer("bar.net");
    $JID->SetResource("Foo Bar");

=head1 METHODS

=head2 Retrieval functions

  GetUserID() - returns a string with the userid of the JID.
                If the string is an address (bob%jabber.org) then
                the function will return it as an address
                (bob@jabber.org).

  GetServer() - returns a string with the server of the JID.

  GetResource() - returns a string with the resource of the JID.

  GetJID()       - returns a string that represents the JID stored
  GetJID("full")   within.  If the "full" string is specified, then
  GetJID("base")   you get the full JID, including Resource, which
                   should be used to send to the server.  If the "base",
                   string is specified, then you will just get
                   user@server, or the base JID.

=head2 Creation functions

  SetJID(userid=>string,   - set multiple fields in the jid at
         server=>string,     one time.  This is a cumulative
         resource=>string)   and over writing action.  If you set
  SetJID(string)             the "userid" attribute twice, the second
                             setting is what is used.  If you set
                             the server, and then set the resource
                             then both will be in the jid.  If all
                             you pass is a string, then that string
                             is used as the JID.  For valid settings
                             read the specific Set functions below.

  SetUserID(string) - sets the userid.  Must be a valid userid or the
                      server will complain if you try to use this JID
                      to talk to the server.  If the string is an
                      address then it will be converted to the %
                      form suitable for using as a User ID.

  SetServer(string) - sets the server.  Must be a valid host on the
                      network or the server will not be able to talk
                      to it.

  SetResource(string) - sets the resource of the userid to talk to.

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.003;
use strict;
use Carp;

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = { };

    bless($self, $proto);

    if ("@_" ne (""))
    {
        my ($jid) = @_;
        return $jid if ((ref($jid) ne "") && ($jid->isa("Net::XMPP::JID")));
        $self->{JID} = $jid;
    }
    else
    {
        $self->{JID} = "";
    }
    $self->ParseJID();

    return $self;
}


##############################################################################
#
# ParseJID - private helper function that takes the JID and sets the
#            the three parts of it.
#
##############################################################################
sub ParseJID
{
    my $self = shift;

    my $userid;
    my $server;
    my $resource;

    ($userid,$server,$resource) =
        ($self->{JID} =~ /^([^\@\/'"&:<>]*)\@([A-Za-z0-9\.\-\_]+)\/?(.*?)$/);
    if (!defined($server))
    {
        ($server,$resource) =
            ($self->{JID} =~ /^([A-Za-z0-9\.\-\_]+)\/?(.*?)$/);
    }

    $userid = "" unless defined($userid);
    $server = "" unless defined($server);
    $resource = "" unless defined($resource);

    $self->{USERID} = $userid;
    $self->{SERVER} = $server;
    $self->{RESOURCE} = $resource;
}


##############################################################################
#
# BuildJID - private helper function that takes the three parts and sets the
#            JID from them.
#
##############################################################################
sub BuildJID
{
    my $self = shift;
    $self->{JID} = $self->{USERID};
    $self->{JID} .= "\@" if ($self->{USERID} ne "");
    $self->{JID} .= $self->{SERVER} if (exists($self->{SERVER}) &&
                        defined($self->{SERVER}));
    $self->{JID} .= "/".$self->{RESOURCE} if (exists($self->{RESOURCE}) &&
                        defined($self->{RESOURCE}) &&
                        ($self->{RESOURCE} ne ""));
}


##############################################################################
#
# GetUserID - returns the userid of the JID.
#
##############################################################################
sub GetUserID
{
    my $self = shift;
    my $userid = $self->{USERID};
    $userid =~ s/\%/\@/;
    return $userid;
}


##############################################################################
#
# GetServer - returns the server of the JID.
#
##############################################################################
sub GetServer
{
    my $self = shift;
    return $self->{SERVER};
}


##############################################################################
#
# GetResource - returns the resource of the JID.
#
##############################################################################
sub GetResource
{
    my $self = shift;
    return $self->{RESOURCE};
}


##############################################################################
#
# GetJID - returns the full jid of the JID.
#
##############################################################################
sub GetJID
{
    my $self = shift;
    my $type = shift;
    $type = "" unless defined($type);
    return $self->{JID} if ($type eq "full");
    return $self->{USERID}."\@".$self->{SERVER} if ($self->{USERID} ne "");
    return $self->{SERVER};
}


##############################################################################
#
# SetJID - takes a hash of all of the things you can set on a JID and sets
#          each one.
#
##############################################################################
sub SetJID
{
    my $self = shift;
    my %jid;

    if ($#_ > 0 ) {
        while($#_ >= 0) { $jid{ lc pop(@_) } = pop(@_); }

        $self->SetUserID($jid{userid}) if exists($jid{userid});
        $self->SetServer($jid{server}) if exists($jid{server});
        $self->SetResource($jid{resource}) if exists($jid{resource});
    } else {
        ($self->{JID}) = @_;
        $self->ParseJID();
    }
}


##############################################################################
#
# SetUserID - sets the userid of the JID.
#
##############################################################################
sub SetUserID
{
    my $self = shift;
    my ($userid) = @_;
    $userid =~ s/\@/\%/;
    $self->{USERID} = $userid;
    $self->BuildJID();
}


##############################################################################
#
# SetServer - sets the server of the JID.
#
##############################################################################
sub SetServer
{
    my $self = shift;
    my ($server) = @_;
    $self->{SERVER} = $server;
    $self->BuildJID();
}


##############################################################################
#
# SetResource - sets the resource of the JID.
#
##############################################################################
sub SetResource
{
    my $self = shift;
    my ($resource) = @_;
    $self->{RESOURCE} = $resource;
    $self->BuildJID();
}


##############################################################################
#
# debug - prints out the contents of the JID
#
##############################################################################
sub debug
{
    my $self = shift;

    print "debug JID: $self\n";
    print "UserID:   (",$self->{USERID},")\n";
    print "Server:   (",$self->{SERVER},")\n";
    print "Resource: (",$self->{RESOURCE},")\n";
    print "JID:      (",$self->{JID},")\n";
}


1;
