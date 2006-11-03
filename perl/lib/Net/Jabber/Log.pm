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

package Net::Jabber::Log;

=head1 NAME

Net::Jabber::Log - Jabber Log Module

=head1 SYNOPSIS

  Net::Jabber::Log is a companion to the Net::Jabber module.
  It provides the user a simple interface to set and retrieve all 
  parts of a Jabber Log.

=head1 DESCRIPTION

  To initialize the Log with a Jabber <log/> you must pass it the 
  XML::Parser Tree array.  For example:

    my $log = new Net::Jabber::Log(@tree);

  There has been a change from the old way of handling the callbacks.
  You no longer have to do the above, a Net::Jabber::Log object is passed
  to the callback function for the log:

    use Net::Jabber;

    sub log {
      my ($Log) = @_;
      .
      .
      .
    }

  You now have access to all of the retrieval functions available.

  To create a new log to send to the server:

    use Net::Jabber;

    $Log = new Net::Jabber::Log();

  Now you can call the creation functions below to populate the tag before
  sending it.

  For more information about the array format being passed to the CallBack
  please read the Net::Jabber::Client documentation.

=head2 Retrieval functions

    $from       = $Log->GetFrom();
    $fromJID    = $Log->GetFrom("jid");
    $type       = $Log->GetType();
    $data       = $Log->GetData();

    $str        = $Log->GetXML();
    @log        = $Log->GetTree();

=head2 Creation functions

    $Log->SetLog(type=>"error",
		 from=>"users.jabber.org",
		 data=>"The moon is full... I can't run anymore.");
    $Log->SetFrom("foo.jabber.org");
    $Log->SetType("warn");
    $Log->SetData("I can't find a config file.  Using defaults.");

=head2 Test functions

    $test = $Log->DefinedFrom();
    $test = $Log->DefinedType();

=head1 METHODS

=head2 Retrieval functions

  GetFrom()      -  returns either a string with the Jabber Identifier,
  GetFrom("jid")    or a Net::Jabber::JID object for the person who
                    sent the <log/>.  To get the JID object set 
                    the string to "jid", otherwise leave blank for the 
                    text string.

  GetType() - returns a string with the type <log/> this is.

  GetData() - returns a string with the cdata of the <log/>.

  GetXML() - returns the XML string that represents the <log/>.
             This is used by the Send() function in Client.pm to send
             this object as a Jabber Log.

  GetTree() - returns an array that contains the <log/> tag
              in XML::Parser Tree format.

=head2 Creation functions

  SetLog(from=>string|JID, - set multiple fields in the <log/>
         type=>string,       at one time.  This is a cumulative
         data=>string)       and over writing action.  If you set
                             the "from" attribute twice, the second
                             setting is what is used.  If you set
                             the type, and then set the data
                             then both will be in the <log/>
                             tag.  For valid settings read the
                             specific Set functions below.

  SetFrom(string) - sets the from attribute.  You can either pass a string
  SetFrom(JID)      or a JID object.  They must be valid Jabber 
                    Identifiers or the server will return an error log.
                    (ie.  jabber:bob@jabber.org/Silent Bob, etc...)

  SetType(string) - sets the type attribute.  Valid settings are:

                    notice     general logging
                    warn       warning
                    alert      critical error (can still run but not 
                               correctly)
                    error      fatal error (cannot run anymore)

  SetData(string) - sets the cdata of the <log/>.

=head2 Test functions

  DefinedFrom() - returns 1 if the from attribute is defined in the 
                  <log/>, 0 otherwise.

  DefinedType() - returns 1 if the type attribute is defined in the 
                  <log/>, 0 otherwise.

=head1 AUTHOR

By Ryan Eatmon in May of 2000 for http://jabber.org..

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.003;
use strict;
use Carp;
use vars qw($VERSION $AUTOLOAD %FUNCTIONS);

$VERSION = "2.0";

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = { };
    
    $self->{VERSION} = $VERSION;
    $self->{TIMESTAMP} = &Net::Jabber::GetTimeStamp("local");

    bless($self, $proto);

    $self->{DEBUG} = new Net::Jabber::Debug(usedefault=>1,
                                                    header=>"NJ::Log");

    if ("@_" ne (""))
    {
        if (ref($_[0]) eq "Net::Jabber::Log")
        {
            return $_[0];
        }
        else
        {
            my @temp = @_;
            $self->{LOG} = \@temp;
        }
    }
    else
    {
        $self->{LOG} = [ "log" , [{}]];
    }

    return $self;
}


##############################################################################
#
# AUTOLOAD - This function calls the delegate with the appropriate function
#            name and argument list.
#
##############################################################################
sub AUTOLOAD
{
    my $self = shift;
    return if ($AUTOLOAD =~ /::DESTROY$/);
    $AUTOLOAD =~ s/^.*:://;
    my ($type,$value) = ($AUTOLOAD =~ /^(Get|Set|Defined)(.*)$/);
    $type = "" unless defined($type);
    my $treeName = "LOG";
    
    return "log" if ($AUTOLOAD eq "GetTag");
    return &XML::Stream::BuildXML(@{$self->{$treeName}}) if ($AUTOLOAD eq "GetXML");
    return @{$self->{$treeName}} if ($AUTOLOAD eq "GetTree");
    return &Net::Jabber::Get($self,$self,$value,$treeName,$FUNCTIONS{get}->{$value},@_) if ($type eq "Get");
    return &Net::Jabber::Set($self,$self,$value,$treeName,$FUNCTIONS{set}->{$value},@_) if ($type eq "Set");
    return &Net::Jabber::Defined($self,$self,$value,$treeName,$FUNCTIONS{defined}->{$value},@_) if ($type eq "Defined");
    return &Net::Jabber::debug($self,$treeName) if ($AUTOLOAD eq "debug");
    &Net::Jabber::MissingFunction($self,$AUTOLOAD);
}


$FUNCTIONS{get}->{From} = ["value","","from"];
$FUNCTIONS{get}->{Type} = ["value","","type"];
$FUNCTIONS{get}->{Data} = ["value","",""];

$FUNCTIONS{set}->{Type} = ["single","","","type","*"];
$FUNCTIONS{set}->{Data} = ["single","","*","",""];

$FUNCTIONS{defined}->{From} = ["existence","","from"];
$FUNCTIONS{defined}->{Type} = ["existence","","type"];


##############################################################################
#
# GetXML -  returns the XML string that represents the data in the XML::Parser
#          Tree.
#
##############################################################################
sub GetXML
{
    my $self = shift;
    $self->MergeX();
    return &XML::Stream::BuildXML(@{$self->{LOG}});
}


##############################################################################
#
# GetTree - returns the XML::Parser Tree that is stored in the guts of
#              the object.
#
##############################################################################
sub GetTree
{
    my $self = shift;
    $self->MergeX();
    return %{$self->{LOG}};
}


##############################################################################
#
# SetLog - takes a hash of all of the things you can set on a <log/>
#              and sets each one.
#
##############################################################################
sub SetLog
{
    my $self = shift;
    my %log;
    while($#_ >= 0) { $log{ lc pop(@_) } = pop(@_); }

    $self->SetFrom($log{from}) if exists($log{from});
    $self->SetType($log{type}) if exists($log{type});
    $self->SetData($log{data}) if exists($log{data});
}


##############################################################################
#
# SetFrom - sets the from attribute in the <log/>
#
##############################################################################
sub SetFrom
{
    my $self = shift;
    my ($from) = @_;
    if (ref($from) eq "Net::Jabber::JID")
    {
        $from = $from->GetJID("full");
    }
    return unless ($from ne "");
    &XML::Stream::SetXMLData("single",$self->{LOG},"","",{from=>$from});
}


1;
