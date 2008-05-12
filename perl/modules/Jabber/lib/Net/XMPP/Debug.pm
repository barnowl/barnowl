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

package Net::XMPP::Debug;

=head1 NAME

Net::XMPP::Debug - XMPP Debug Module

=head1 SYNOPSIS

  Net::XMPP::Debug is a module that provides a developer easy access
  to logging debug information.

=head1 DESCRIPTION

  Debug is a helper module for the Net::XMPP modules.  It provides
  the Net::XMPP modules with an object to control where, how, and
  what is logged.

=head2 Basic Functions

    $Debug = Net::XMPP::Debug->new();

    $Debug->Init(level=>2,
	             file=>"stdout",
  	             header=>"MyScript");

    $Debug->Log0("Connection established");

=head1 METHODS

=head2 Basic Functions

    new(hash) - creates the Debug object.  The hash argument is passed
                to the Init function.  See that function description
                below for the valid settings.

    Init(level=>integer,  - initializes the debug object.  The level
         file=>string,      determines the maximum level of debug
         header=>string,    messages to log:
         setdefault=>0|1,     0 - Base level Output (default)
         usedefault=>0|1,     1 - High level API calls
         time=>0|1)           2 - Low level API calls
                              ...
                              N - Whatever you want....
                            The file determines where the debug log
                            goes.  You can either specify a path to
                            a file, or "stdout" (the default).  "stdout"
                            tells Debug to send all of the debug info
                            sent to this object to go to stdout.
                            header is a string that will preappended
                            to the beginning of all log entries.  This
                            makes it easier to see what generated the
                            log entry (default is "Debug").
                            setdefault saves the current filehandle
                            and makes it available for other Debug
                            objects to use.  To use the default set
                            usedefault to 1.  The time parameter
                            specifies whether or not to add a timestamp
                            to the beginning of each logged line.

    LogN(array) - Logs the elements of the array at the corresponding
                  debug level N.  If you pass in a reference to an
                  array or hash then they are printed in a readable
                  way.  (ie... Log0, Log2, Log100, etc...)

=head1 EXAMPLE

  $Debug = Net::XMPP:Debug->new(level=>2,
                               header=>"Example");

    $Debug->Log0("test");

    $Debug->Log2("level 2 test");

    $hash{a} = "atest";
    $hash{b} = "btest";

    $Debug->Log1("hashtest",\%hash);

  You would get the following log:

    Example: test
    Example: level 2 test
    Example: hashtest { a=>"atest" b=>"btest" }

  If you had set the level to 1 instead of 2 you would get:

    Example: test
    Example: hashtest { a=>"atest" b=>"btest" }

=head1 AUTHOR

Ryan Eatmon

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

require 5.003;
use strict;
use FileHandle;
use Carp;
use vars qw( %HANDLES $DEFAULT $DEFAULTLEVEL $DEFAULTTIME $AUTOLOAD );

$DEFAULTLEVEL = -1;

sub new
{
    my $proto = shift;
    my $self = { };
    bless($self, $proto);

    $self->Init(@_);

    return $self;
}


##############################################################################
#
# Init - opens the fielhandle and initializes the Debug object.
#
##############################################################################
sub Init
{
    my $self = shift;

    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }
    
    delete($args{file}) if (lc($args{file}) eq "stdout");

    $args{time} = 0 if !exists($args{time});
    $args{setdefault} = 0 if !exists($args{setdefault});
    $args{usedefault} = 0 if !exists($args{usedefault});

    $self->{TIME} = $args{time};

    if ($args{usedefault} == 1)
    {
        $args{setdefault} = 0;
        $self->{USEDEFAULT} = 1;
    }
    else
    {
        $self->{LEVEL} = 0;
        $self->{LEVEL} = $args{level} if exists($args{level});

        $self->{HANDLE} = new FileHandle(">&STDERR");
        $self->{HANDLE}->autoflush(1);
        if (exists($args{file}))
        {
            if (exists($Net::XMPP::Debug::HANDLES{$args{file}}))
            {
                $self->{HANDLE} = $Net::XMPP::Debug::HANDLES{$args{file}};
                $self->{HANDLE}->autoflush(1);
            }
            else
            {
                if (-e $args{file})
                {
                    if (-w $args{file})
                    {
                        $self->{HANDLE} = new FileHandle(">$args{file}");
                        if (defined($self->{HANDLE}))
                        {
                            $self->{HANDLE}->autoflush(1);
                            binmode $self->{HANDLE}, ":utf8";
                            $Net::XMPP::Debug::HANDLES{$args{file}} = $self->{HANDLE};
                        }
                        else
                        {
                            print STDERR "ERROR: Debug filehandle could not be opened.\n";
                            print STDERR"        Debugging disabled.\n";
                            print STDERR "       ($!)\n";
                            $self->{LEVEL} = -1;
                        }
                    }
                    else
                    {
                        print STDERR "ERROR: You do not have permission to write to $args{file}.\n";
                        print STDERR"        Debugging disabled.\n";
                        $self->{LEVEL} = -1;
                    }
                }
                else
                {
                    $self->{HANDLE} = new FileHandle(">$args{file}");
                    if (defined($self->{HANDLE}))
                    {
                        $self->{HANDLE}->autoflush(1);
                        $Net::XMPP::Debug::HANDLES{$args{file}} = $self->{HANDLE};
                    }
                    else
                    {
                        print STDERR "ERROR: Debug filehandle could not be opened.\n";
                        print STDERR"        Debugging disabled.\n";
                        print STDERR "       ($!)\n";
                        $self->{LEVEL} = -1;
                    }
                }
            }
        }
    }
    if ($args{setdefault} == 1)
    {
        $Net::XMPP::Debug::DEFAULT = $self->{HANDLE};
        $Net::XMPP::Debug::DEFAULTLEVEL = $self->{LEVEL};
        $Net::XMPP::Debug::DEFAULTTIME = $self->{TIME};
    }

    $self->{HEADER} = "Debug";
    $self->{HEADER} = $args{header} if exists($args{header});
}


##############################################################################
#
# Log - takes the limit and the array to log and logs them
#
##############################################################################
sub Log
{
    my $self = shift;
    my (@args) = @_;

    my $fh = $self->{HANDLE};
    $fh = $Net::XMPP::Debug::DEFAULT if exists($self->{USEDEFAULT});

    my $string = "";

    my $testTime = $self->{TIME};
    $testTime = $Net::XMPP::Debug::DEFAULTTIME if exists($self->{USEDEFAULT});

    $string .= "[".&Net::XMPP::GetTimeStamp("local",time,"short")."] "
        if ($testTime == 1);
    $string .= $self->{HEADER}.": ";

    my $arg;

    foreach $arg (@args)
    {
        if (ref($arg) eq "HASH")
        {
            $string .= " {";
            my $key;
            foreach $key (sort {$a cmp $b} keys(%{$arg}))
            {
                $string .= " ".$key."=>'".$arg->{$key}."'";
            }
            $string .= " }";
        }
        else
        {
            if (ref($arg) eq "ARRAY")
            {
                $string .= " [ ".join(" ",@{$arg})." ]";
            }  else {
                $string .= $arg;
            }
        }
    }
    print $fh "$string\n";
    return 1;
}


##############################################################################
#
# AUTOLOAD - if a function is called that is not defined then this function
#            will examine the function name and either give an error or call
#            the appropriate function.
#
##############################################################################
sub AUTOLOAD
{
    my $self = shift;
    return if ($AUTOLOAD =~ /::DESTROY$/);
    my ($function) = ($AUTOLOAD =~ /\:\:(.*)$/);
    croak("$function not defined") if !($function =~ /Log\d+/);
    my ($level) = ($function =~ /Log(\d+)/);
    return 0 if ($level > (exists($self->{USEDEFAULT}) ? $Net::XMPP::Debug::DEFAULTLEVEL : $self->{LEVEL}));
    $self->Log(@_);
}


##############################################################################
#
# GetHandle - returns the filehandle being used by this object.
#
##############################################################################
sub GetHandle
{
    my $self = shift;
    return $self->{HANDLE};
}


##############################################################################
#
# GetLevel - returns the debug level used by this object.
#
##############################################################################
sub GetLevel
{
    my $self = shift;
    return $self->{LEVEL};
}


##############################################################################
#
# GetTime - returns the debug time used by this object.
#
##############################################################################
sub GetTime
{
    my $self = shift;
    return $self->{TIME};
}


1;
