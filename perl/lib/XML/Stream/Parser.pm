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

package XML::Stream::Parser;

=head1 NAME

  XML::Stream::Parser - SAX XML Parser for XML Streams

=head1 SYNOPSIS

  Light weight XML parser that builds XML::Parser::Tree objects from the
  incoming stream and passes them to a function to tell whoever is using
  it that there are new packets.

=head1 DESCRIPTION

  This module provides a very light weight parser

=head1 METHODS

=head1 EXAMPLES

=head1 AUTHOR

By Ryan Eatmon in January of 2001 for http://jabber.org/

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use strict;
use vars qw( $VERSION );

$VERSION = "1.22";

sub new
{
    my $self = { };

    bless($self);

    my %args;
    while($#_ >= 0) { $args{ lc pop(@_) } = pop(@_); }

    $self->{PARSING} = 0;
    $self->{DOC} = 0;
    $self->{XML} = "";
    $self->{CNAME} = ();
    $self->{CURR} = 0;

    $args{nonblocking} = 0 unless exists($args{nonblocking});

    $self->{NONBLOCKING} = delete($args{nonblocking});

    $self->{DEBUGTIME} = 0;
    $self->{DEBUGTIME} = $args{debugtime} if exists($args{debugtime});

    $self->{DEBUGLEVEL} = 0;
    $self->{DEBUGLEVEL} = $args{debuglevel} if exists($args{debuglevel});

    $self->{DEBUGFILE} = "";

    if (exists($args{debugfh}) && ($args{debugfh} ne ""))
    {
        $self->{DEBUGFILE} = $args{debugfh};
        $self->{DEBUG} = 1;
    }

    if ((exists($args{debugfh}) && ($args{debugfh} eq "")) ||
        (exists($args{debug}) && ($args{debug} ne "")))
        {
        $self->{DEBUG} = 1;
        if (lc($args{debug}) eq "stdout")
        {
            $self->{DEBUGFILE} = new FileHandle(">&STDERR");
            $self->{DEBUGFILE}->autoflush(1);
        }
        else
        {
            if (-e $args{debug})
            {
                if (-w $args{debug})
                {
                    $self->{DEBUGFILE} = new FileHandle(">$args{debug}");
                    $self->{DEBUGFILE}->autoflush(1);
                }
                else
                {
                    print "WARNING: debug file ($args{debug}) is not writable by you\n";
                    print "         No debug information being saved.\n";
                    $self->{DEBUG} = 0;
                }
            }
            else
            {
                $self->{DEBUGFILE} = new FileHandle(">$args{debug}");
                if (defined($self->{DEBUGFILE}))
                {
                    $self->{DEBUGFILE}->autoflush(1);
                }
                else
                {
                    print "WARNING: debug file ($args{debug}) does not exist \n";
                    print "         and is not writable by you.\n";
                    print "         No debug information being saved.\n";
                    $self->{DEBUG} = 0;
                }
            }
        }
    }

    $self->{SID} = exists($args{sid}) ? $args{sid} : "__xmlstream__:sid";

    $self->{STYLE} = (exists($args{style}) ? lc($args{style}) : "tree");
    $self->{DTD} = (exists($args{dtd}) ? lc($args{dtd}) : 0);

    if ($self->{STYLE} eq "tree")
    {
        $self->{HANDLER}->{startDocument} = sub{ $self->startDocument(@_); };
        $self->{HANDLER}->{endDocument} = sub{ $self->endDocument(@_); };
        $self->{HANDLER}->{startElement} = sub{ &XML::Stream::Tree::_handle_element(@_); };
        $self->{HANDLER}->{endElement} = sub{ &XML::Stream::Tree::_handle_close(@_); };
        $self->{HANDLER}->{characters} = sub{ &XML::Stream::Tree::_handle_cdata(@_); };
    }
    elsif ($self->{STYLE} eq "node")
    {
        $self->{HANDLER}->{startDocument} = sub{ $self->startDocument(@_); };
        $self->{HANDLER}->{endDocument} = sub{ $self->endDocument(@_); };
        $self->{HANDLER}->{startElement} = sub{ &XML::Stream::Node::_handle_element(@_); };
        $self->{HANDLER}->{endElement} = sub{ &XML::Stream::Node::_handle_close(@_); };
        $self->{HANDLER}->{characters} = sub{ &XML::Stream::Node::_handle_cdata(@_); };
    }
    $self->setHandlers(%{$args{handlers}});

    $self->{XMLONHOLD} = "";

    return $self;
}


###########################################################################
#
# debug - prints the arguments to the debug log if debug is turned on.
#
###########################################################################
sub debug
{
    return if ($_[1] > $_[0]->{DEBUGLEVEL});
    my $self = shift;
    my ($limit,@args) = @_;
    return if ($self->{DEBUGFILE} eq "");
    my $fh = $self->{DEBUGFILE};
    if ($self->{DEBUGTIME} == 1)
    {
        my ($sec,$min,$hour) = localtime(time);
        print $fh sprintf("[%02d:%02d:%02d] ",$hour,$min,$sec);
    }
    print $fh "XML::Stream::Parser: $self->{STYLE}: @args\n";
}


sub setSID
{
    my $self = shift;
    my $sid = shift;
    $self->{SID} = $sid;
}


sub getSID
{
    my $self = shift;
    return $self->{SID};
}


sub setHandlers
{
    my $self = shift;
    my (%handlers) = @_;

    foreach my $handler (keys(%handlers))
    {
        $self->{HANDLER}->{$handler} = $handlers{$handler};
    }
}


sub parse
{
    my $self = shift;
    my $xml = shift;

    return unless defined($xml);
    return if ($xml eq "");

    if ($self->{XMLONHOLD} ne "")
    {
        $self->{XML} = $self->{XMLONHOLD};
        $self->{XMLONHOLD} = "";
    }

    # XXX change this to not use regex?
    while($xml =~ s/<\!--.*?-->//gs) {}

    $self->{XML} .= $xml;

    return if ($self->{PARSING} == 1);

    $self->{PARSING} = 1;

    if(!$self->{DOC} == 1)
    {
        my $start = index($self->{XML},"<");

        if ((substr($self->{XML},$start,3) eq "<?x") ||
            (substr($self->{XML},$start,3) eq "<?X"))
        {
            my $close = index($self->{XML},"?>");
            if ($close == -1)
            {
                $self->{PARSING} = 0;
                return;
            }
            $self->{XML} = substr($self->{XML},$close+2,length($self->{XML})-$close-2);
        }

        &{$self->{HANDLER}->{startDocument}}($self);
        $self->{DOC} = 1;
    }

    while(1)
    {
        if (length($self->{XML}) == 0)
        {
            $self->{PARSING} = 0;
            return $self->returnData(0);
        }
        my $eclose = -1;
        $eclose = index($self->{XML},"</".$self->{CNAME}->[$self->{CURR}].">")
            if ($#{$self->{CNAME}} > -1);

        if ($eclose == 0)
        {
            $self->{XML} = substr($self->{XML},length($self->{CNAME}->[$self->{CURR}])+3,length($self->{XML})-length($self->{CNAME}->[$self->{CURR}])-3);

            $self->{PARSING} = 0 if ($self->{NONBLOCKING} == 1);
            &{$self->{HANDLER}->{endElement}}($self,$self->{CNAME}->[$self->{CURR}]);
            $self->{PARSING} = 1 if ($self->{NONBLOCKING} == 1);

            $self->{CURR}--;
            if ($self->{CURR} == 0)
            {
                $self->{DOC} = 0;
                $self->{PARSING} = 0;
                &{$self->{HANDLER}->{endDocument}}($self);
                return $self->returnData(0);
            }
            next;
        }

        my $estart = index($self->{XML},"<");
        my $cdatastart = index($self->{XML},"<![CDATA[");
        if (($estart == 0) && ($cdatastart != 0))
        {
            my $close = index($self->{XML},">");
            if ($close == -1)
            {
                $self->{PARSING} = 0;
                return $self->returnData(0);
            }
            my $empty = (substr($self->{XML},$close-1,1) eq "/");
            my $starttag = substr($self->{XML},1,$close-($empty ? 2 : 1));
            my $nextspace = index($starttag," ");
            my $attribs;
            my $name;
            if ($nextspace != -1)
            {
                $name = substr($starttag,0,$nextspace);
                $attribs = substr($starttag,$nextspace+1,length($starttag)-$nextspace-1);
            }
            else
            {
                $name = $starttag;
            }

            my %attribs = $self->attribution($attribs);
            if (($self->{DTD} == 1) && (exists($attribs{xmlns})))
            {
            }

            &{$self->{HANDLER}->{startElement}}($self,$name,%attribs);

            if($empty == 1)
            {
                &{$self->{HANDLER}->{endElement}}($self,$name);
            }
            else
            {
                $self->{CURR}++;
                $self->{CNAME}->[$self->{CURR}] = $name;
            }
    
            $self->{XML} = substr($self->{XML},$close+1,length($self->{XML})-$close-1);
            next;
        }

        if ($cdatastart == 0)
        {
            my $cdataclose = index($self->{XML},"]]>");
            if ($cdataclose == -1)
            {
                $self->{PARSING} = 0;
                return $self->returnData(0);
            }
            
            &{$self->{HANDLER}->{characters}}($self,substr($self->{XML},9,$cdataclose-9));
            
            $self->{XML} = substr($self->{XML},$cdataclose+3,length($self->{XML})-$cdataclose-3);
            next;
         }

        if ($estart == -1)
        {
            $self->{XMLONHOLD} = $self->{XML};
            $self->{XML} = "";
        }
        elsif (($cdatastart == -1) || ($cdatastart > $estart))
        {
            &{$self->{HANDLER}->{characters}}($self,$self->entityCheck(substr($self->{XML},0,$estart)));
            $self->{XML} = substr($self->{XML},$estart,length($self->{XML})-$estart);
        }
    }
}


sub attribution
{
    my $self = shift;
    my $str = shift;

    $str = "" unless defined($str);

    my %attribs;

    while(1)
    {
        my $eq = index($str,"=");
        if((length($str) == 0) || ($eq == -1))
        {
            return %attribs;
        }

        my $ids;
        my $id;
        my $id1 = index($str,"\'");
        my $id2 = index($str,"\"");
        if((($id1 < $id2) && ($id1 != -1)) || ($id2 == -1))
        {
            $ids = $id1;
            $id = "\'";
        }
        if((($id2 < $id1) && ($id1 == -1)) || ($id2 != -1))
        {
            $ids = $id2;
            $id = "\"";
        }

        my $nextid = index($str,$id,$ids+1);
        my $val = substr($str,$ids+1,$nextid-$ids-1);
        my $key = substr($str,0,$eq);

        while($key =~ s/\s//) {}

        $attribs{$key} = $self->entityCheck($val);
        $str = substr($str,$nextid+1,length($str)-$nextid-1);
    }

    return %attribs;
}


sub entityCheck
{
    my $self = shift;
    my $str = shift;

    while($str =~ s/\&lt\;/\</) {}
    while($str =~ s/\&gt\;/\>/) {}
    while($str =~ s/\&quot\;/\"/) {}
    while($str =~ s/\&apos\;/\'/) {}
    while($str =~ s/\&amp\;/\&/) {}

    return $str;
}


sub parsefile
{
    my $self = shift;
    my $fileName = shift;

    open(FILE,"<",$fileName);
    my $file;
    while(<FILE>) { $file .= $_; }
    $self->parse($file);
    close(FILE);

    return $self->returnData();
}


sub returnData
{
    my $self = shift;
    my $clearData = shift;
    $clearData = 1 unless defined($clearData);

    my $sid = $self->{SID};

    if ($self->{STYLE} eq "tree")
    {
        return unless exists($self->{SIDS}->{$sid}->{tree});
        my @tree = @{$self->{SIDS}->{$sid}->{tree}};
        delete($self->{SIDS}->{$sid}->{tree}) if ($clearData == 1);
        return ( \@tree );
    }
    if ($self->{STYLE} eq "node")
    {
        return unless exists($self->{SIDS}->{$sid}->{node});
        my $node = $self->{SIDS}->{$sid}->{node}->[0];
        delete($self->{SIDS}->{$sid}->{node}) if ($clearData == 1);
        return $node;
    }
}


sub startDocument
{
    my $self = shift;
}


sub endDocument
{
    my $self = shift;
}


sub startElement
{
    my $self = shift;
    my ($sax, $tag, %att) = @_;

    return unless ($self->{DOC} == 1);

    if ($self->{STYLE} eq "debug")
    {
        print "$self->{DEBUGHEADER} \\\\ (",join(" ",%att),")\n";
        $self->{DEBUGHEADER} .= $tag." ";
    }
    else
    {
        my @NEW;
        if($#{$self->{TREE}} < 0)
        {
            push @{$self->{TREE}}, $tag;
        }
        else
        {
            push @{ $self->{TREE}[ $#{$self->{TREE}}]}, $tag;
        }
        push @NEW, \%att;
        push @{$self->{TREE}}, \@NEW;
    }
}


sub characters
{
    my $self = shift;
    my ($sax, $cdata) = @_;

    return unless ($self->{DOC} == 1);

    if ($self->{STYLE} eq "debug") 
    {
        my $str = $cdata;
        $str =~ s/\n/\#10\;/g;
        print "$self->{DEBUGHEADER} || $str\n";
    }
    else
    {
        return if ($#{$self->{TREE}} == -1);

        my $pos = $#{$self->{TREE}};

        if ($pos > 0 && $self->{TREE}[$pos - 1] eq "0")
        {
            $self->{TREE}[$pos - 1] .= $cdata;
        }
        else
        {
            push @{$self->{TREE}[$#{$self->{TREE}}]}, 0;
            push @{$self->{TREE}[$#{$self->{TREE}}]}, $cdata;
        }  
    }
}


sub endElement
{
    my $self = shift;
    my ($sax, $tag) = @_;

    return unless ($self->{DOC} == 1);

    if ($self->{STYLE} eq "debug")
    {
        $self->{DEBUGHEADER} =~ s/\S+\ $//;
        print "$self->{DEBUGHEADER} //\n";
    }
    else
    {
        my $CLOSED = pop @{$self->{TREE}};

        if($#{$self->{TREE}} < 1)
        {
            push @{$self->{TREE}}, $CLOSED;

            if($self->{TREE}->[0] eq "stream:error")
            {
                $self->{STREAMERROR} = $self->{TREE}[1]->[2];
            }
        }
        else
        {
            push @{$self->{TREE}[$#{$self->{TREE}}]}, $CLOSED;
        }
    }
}


1;
