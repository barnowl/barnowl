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

package XML::Stream::Parser::DTD;

=head1 NAME

  XML::Stream::Parser::DTD - XML DTD Parser and Verifier

=head1 SYNOPSIS

  This is a work in progress.  I had need for a DTD parser and verifier
  and so am working on it here.  If you are reading this then you are
  snooping.  =)

=head1 DESCRIPTION

  This module provides the initial code for a DTD parser and verifier.

=head1 METHODS

=head1 EXAMPLES

=head1 AUTHOR

By Ryan Eatmon in February of 2001 for http://jabber.org/

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

    $self->{URI} = $args{uri};

    $self->{PARSING} = 0;
    $self->{DOC} = 0;
    $self->{XML} = "";
    $self->{CNAME} = ();
    $self->{CURR} = 0;

    $self->{ENTITY}->{"&lt;"} = "<";
    $self->{ENTITY}->{"&gt;"} = ">";
    $self->{ENTITY}->{"&quot;"} = "\"";
    $self->{ENTITY}->{"&apos;"} = "'";
    $self->{ENTITY}->{"&amp;"} = "&";

    $self->{HANDLER}->{startDocument} = sub{ $self->startDocument(@_); };
    $self->{HANDLER}->{endDocument} = sub{ $self->endDocument(@_); };
    $self->{HANDLER}->{startElement} = sub{ $self->startElement(@_); };
    $self->{HANDLER}->{endElement} = sub{ $self->endElement(@_); };

    $self->{STYLE} = "debug";

    open(DTD,$args{uri});
    my $dtd = join("",<DTD>);
    close(DTD);

    $self->parse($dtd);

    return $self;
}


sub parse
{
    my $self = shift;
    my $xml = shift;

    while($xml =~ s/<\!--.*?-->//gs) {}
    while($xml =~ s/\n//g) {}

    $self->{XML} .= $xml;

    return if ($self->{PARSING} == 1);

    $self->{PARSING} = 1;

    if(!$self->{DOC} == 1)
    {
        my $start = index($self->{XML},"<");

        if (substr($self->{XML},$start,3) =~ /^<\?x$/i)
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
            return;
        }

        my $estart = index($self->{XML},"<");
        if ($estart == -1)
        {
            $self->{PARSING} = 0;
            return;
        }

        my $close = index($self->{XML},">");
        my $dtddata = substr($self->{XML},$estart+1,$close-1);
        my $nextspace = index($dtddata," ");
        my $attribs;

        my $type = substr($dtddata,0,$nextspace);
        $dtddata = substr($dtddata,$nextspace+1,length($dtddata)-$nextspace-1);
        $nextspace = index($dtddata," ");

        if ($type eq "!ENTITY")
        {
            $self->entity($type,$dtddata);
        }
        else
        {
            my $tag = substr($dtddata,0,$nextspace);
            $dtddata = substr($dtddata,$nextspace+1,length($dtddata)-$nextspace-1);
            $nextspace = index($dtddata," ");

            $self->element($type,$tag,$dtddata) if ($type eq "!ELEMENT");
            $self->attlist($type,$tag,$dtddata) if ($type eq "!ATTLIST");
        }

        $self->{XML} = substr($self->{XML},$close+1,length($self->{XML})-$close-1);
        next;
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


sub entity
{
    my $self = shift;
    my ($type, $data) = @_;

    foreach my $entity (keys(%{$self->{ENTITY}}))
    {
        $data =~ s/$entity/$self->{ENTITY}->{$entity}/g;
    }

    my ($symbol,$tag,undef,$string) = ($data =~ /^\s*(\S+)\s+(\S+)\s+(\"|\')([^\3]*)\3\s*$/);
    $self->{ENTITY}->{"${symbol}${tag}\;"} = $string;
}

sub element
{
    my $self = shift;
    my ($type, $tag, $data) = @_;

    foreach my $entity (keys(%{$self->{ENTITY}}))
    {
        $data =~ s/$entity/$self->{ENTITY}->{$entity}/g;
    }

    $self->{COUNTER}->{$tag} = 0 unless exists($self->{COUNTER}->{$tag});

    $self->parsegrouping($tag,\$self->{ELEMENT}->{$tag},$data);
    $self->flattendata(\$self->{ELEMENT}->{$tag});

}


sub flattendata
{
    my $self = shift;
    my $dstr = shift;

    if ($$dstr->{type} eq "list")
    {
        foreach my $index (0..$#{$$dstr->{list}})
        {
            $self->flattendata(\$$dstr->{list}->[$index]);
        }

        if (!exists($$dstr->{repeat}) && ($#{$$dstr->{list}} == 0))
        {
            $$dstr = $$dstr->{list}->[0];
        }
    }
}

sub parsegrouping
{
    my $self = shift;
    my ($tag,$dstr,$data) = @_;

    $data =~ s/^\s*//;
    $data =~ s/\s*$//;

    if ($data =~ /[\*\+\?]$/)
    {
        ($$dstr->{repeat}) = ($data =~ /(.)$/);
        $data =~ s/.$//;
    }

    if ($data =~ /^\(.*\)$/)
    {
        my ($seperator) = ($data =~ /^\(\s*\S+\s*(\,|\|)/);
        $$dstr->{ordered} = "yes" if ($seperator eq ",");
        $$dstr->{ordered} = "no" if ($seperator eq "|");

        my $count = 0;
        $$dstr->{type} = "list";
        foreach my $grouping ($self->groupinglist($data,$seperator))
        {
            $self->parsegrouping($tag,\$$dstr->{list}->[$count],$grouping);
            $count++;
        }
    }
    else
    {
        $$dstr->{type} = "element";
        $$dstr->{element} = $data;
        $self->{COUNTER}->{$data} = 0 unless exists($self->{COUNTER}->{$data});
        $self->{COUNTER}->{$data}++;
        $self->{CHILDREN}->{$tag}->{$data} = 1;
    }
}


sub attlist
{
    my $self = shift;
    my ($type, $tag, $data) = @_;

    foreach my $entity (keys(%{$self->{ENTITY}}))
    {
        $data =~ s/$entity/$self->{ENTITY}->{$entity}/g;
    }

    while($data ne "")
    {
        my ($att) = ($data =~ /^\s*(\S+)/);
        $data =~ s/^\s*\S+\s*//;

        my $value;
        if ($data =~ /^\(/)
        {
            $value = $self->getgrouping($data);
            $data = substr($data,length($value)+1,length($data));
            $data =~ s/^\s*//;
            $self->{ATTLIST}->{$tag}->{$att}->{type} = "list";
            foreach my $val (split(/\s*\|\s*/,substr($value,1,length($value)-2))) {
$self->{ATTLIST}->{$tag}->{$att}->{value}->{$val} = 1;
            }
        }
        else
        {
            ($value) = ($data =~ /^(\S+)/);
            $data =~ s/^\S+\s*//;
            $self->{ATTLIST}->{$tag}->{$att}->{type} = $value;
        }

        my $default;
        if ($data =~ /^\"|^\'/)
        {
            my($sq,$val) = ($data =~ /^(\"|\')([^\"\']*)\1/);
            $default = $val;
            $data =~ s/^$sq$val$sq\s*//;
        }
        else
        {
            ($default) = ($data =~ /^(\S+)/);
            $data =~ s/^\S+\s*//;
        }

        $self->{ATTLIST}->{$tag}->{$att}->{default} = $default;
    }
}



sub getgrouping
{
    my $self = shift;
    my ($data) = @_;

    my $count = 0;
    my $parens = 0;
    foreach my $char (split("",$data))
    {
        $parens++ if ($char eq "(");
        $parens-- if ($char eq ")");
        $count++;
        last if ($parens == 0);
    }
    return substr($data,0,$count);
}


sub groupinglist
{
    my $self = shift;
    my ($grouping,$seperator) = @_;

    my @list;
    my $item = "";
    my $parens = 0;
    my $word = "";
    $grouping = substr($grouping,1,length($grouping)-2) if ($grouping =~ /^\(/);
    foreach my $char (split("",$grouping))
    {
        $parens++ if ($char eq "(");
        $parens-- if ($char eq ")");
        if (($parens == 0) && ($char eq $seperator))
        {
            push(@list,$word);
            $word = "";
        }
        else
        {
            $word .= $char;
        }
    }
    push(@list,$word) unless ($word eq "");
    return @list;
}


sub root
{
    my $self = shift;
    my $tag = shift;
    my @root;
    foreach my $tag (keys(%{$self->{COUNTER}}))
    {
        push(@root,$tag) if ($self->{COUNTER}->{$tag} == 0);
    }

    print "ERROR:  Too many root tags... Check the DTD...\n"
        if ($#root > 0);
    return $root[0];
}


sub children
{
    my $self = shift;
    my ($tag,$tree) = @_;

    return unless exists ($self->{CHILDREN}->{$tag});
    return if (exists($self->{CHILDREN}->{$tag}->{EMPTY}));
    if (defined($tree))
    {
        my @current;
        foreach my $current (&XML::Stream::GetXMLData("tree array",$tree,"*","",""))
        {
            push(@current,$$current[0]);
        }
        return $self->allowedchildren($self->{ELEMENT}->{$tag},\@current);
    }
    return $self->allowedchildren($self->{ELEMENT}->{$tag});
}


sub allowedchildren
{
    my $self = shift;
    my ($dstr,$current) = @_;

    my @allowed;

    if ($dstr->{type} eq "element")
    {
        my $test = (defined($current) && $#{@{$current}} > -1) ? $$current[0] : "";
        shift(@{$current}) if ($dstr->{element} eq $test);
        if ($self->repeatcheck($dstr,$test) == 1)
        {
            return $dstr->{element};
        }
    }
    else
    {
        foreach my $index (0..$#{$dstr->{list}})
        {
            push(@allowed,$self->allowedchildren($dstr->{list}->[$index],$current));
        }
    }

    return @allowed;
}


sub repeatcheck
{
    my $self = shift;
    my ($dstr,$tag) = @_;

    $dstr = $self->{ELEMENT}->{$dstr} if exists($self->{ELEMENT}->{$dstr});

#  print "repeatcheck: tag($tag)\n";
#  print "repeatcheck: repeat($dstr->{repeat})\n"
#    if exists($dstr->{repeat});

    my $return = 0;
    $return = ((!defined($tag) ||
                ($tag eq $dstr->{element})) ?
               0 :
               1)
        if (!exists($dstr->{repeat}) ||
            ($dstr->{repeat} eq "?"));
    $return = ((defined($tag) ||
                (exists($dstr->{ordered}) &&
                ($dstr->{ordered} eq "yes"))) ?
               1 :
               0)
        if (exists($dstr->{repeat}) &&
            (($dstr->{repeat} eq "+") ||
             ($dstr->{repeat} eq "*")));

#  print "repeatcheck: return($return)\n";
    return $return;
}


sub required
{
    my $self = shift;
    my ($dstr,$tag,$count) = @_;

    $dstr = $self->{ELEMENT}->{$dstr} if exists($self->{ELEMENT}->{$dstr});

    if ($dstr->{type} eq "element")
    {
        return 0 if ($dstr->{element} ne $tag);
        return 1 if !exists($dstr->{repeat});
        return 1 if (($dstr->{repeat} eq "+") && ($count == 1)) ;
    }
    else
    {
        return 0 if (($dstr->{repeat} eq "*") || ($dstr->{repeat} eq "?"));
        my $test = 0;
        foreach my $index (0..$#{$dstr->{list}})
        {
            $test = $test | $self->required($dstr->{list}->[$index],$tag,$count);
        }
        return $test;
    }
    return 0;
}


sub addchild
{
    my $self = shift;
    my ($tag,$child,$tree) = @_;

#  print "addchild: tag($tag) child($child)\n";

    my @current;
    if (defined($tree))
    {
#    &Net::Jabber::printData("\$tree",$tree);

        @current = &XML::Stream::GetXMLData("index array",$tree,"*","","");

#    &Net::Jabber::printData("\$current",\@current);
    }

    my @newBranch = $self->addchildrecurse($self->{ELEMENT}->{$tag},$child,\@current);

    return $tree unless ("@newBranch" ne "");

#  &Net::Jabber::printData("\$newBranch",\@newBranch);

    my $location = shift(@newBranch);

    if ($location eq "end")
    {
        splice(@{$$tree[1]},@{$$tree[1]},0,@newBranch);
    }
    else
    {
        splice(@{$$tree[1]},$location,0,@newBranch);
    }
    return $tree;
}


sub addcdata
{
    my $self = shift;
    my ($tag,$child,$tree) = @_;

#  print "addchild: tag($tag) child($child)\n";

    my @current;
    if (defined($tree))
    {
#    &Net::Jabber::printData("\$tree",$tree);

        @current = &XML::Stream::GetXMLData("index array",$tree,"*","","");

#    &Net::Jabber::printData("\$current",\@current);
    }

    my @newBranch = $self->addchildrecurse($self->{ELEMENT}->{$tag},$child,\@current);

    return $tree unless ("@newBranch" ne "");

#  &Net::Jabber::printData("\$newBranch",\@newBranch);

    my $location = shift(@newBranch);

    if ($location eq "end")
    {
        splice(@{$$tree[1]},@{$$tree[1]},0,@newBranch);
    }
    else
    {
        splice(@{$$tree[1]},$location,0,@newBranch);
    }
    return $tree;
}


sub addchildrecurse
{
    my $self = shift;
    my ($dstr,$child,$current) = @_;

#  print "addchildrecurse: child($child) type($dstr->{type})\n";

    if ($dstr->{type} eq "element")
    {
#    print "addchildrecurse: tag($dstr->{element})\n";
        my $count = 0;
        while(($#{@{$current}} > -1) && ($dstr->{element} eq $$current[0]))
        {
            shift(@{$current});
            shift(@{$current});
            $count++;
        }
        if (($dstr->{element} eq $child) &&
            ($self->repeatcheck($dstr,(($count > 0) ? $child : "")) == 1))
        {
            my @return = ( "end" , $self->newbranch($child));
            @return = ($$current[1], $self->newbranch($child))
                if ($#{@{$current}} > -1);
#      print "addchildrecurse: Found the spot! (",join(",",@return),")\n";

            return @return;
        }
    }
    else
    {
        foreach my $index (0..$#{$dstr->{list}})
        {
            my @newBranch = $self->addchildrecurse($dstr->{list}->[$index],$child,$current);
            return @newBranch if ("@newBranch" ne "");
        }
    }
#  print "Let's blow....\n";
    return;
}


sub deletechild
{
    my $self = shift;
    my ($tag,$parent,$parenttree,$tree) = @_;

    return $tree unless exists($self->{ELEMENT}->{$tag});
    return $tree if $self->required($parent,$tag,&XML::Stream::GetXMLData("count",$parenttree,$tag));

    return [];
}



sub newbranch
{
    my $self = shift;
    my $tag = shift;

    $tag = $self->root() unless defined($tag);

    my @tree = ();

    return ("0","") if ($tag eq "#PCDATA");

    push(@tree,$tag);
    push(@tree,[ {} ]);

    foreach my $att ($self->attribs($tag))
    {
        $tree[1]->[0]->{$att} = ""
            if (($self->{ATTLIST}->{$tag}->{$att}->{default} eq "#REQUIRED") &&
                ($self->{ATTLIST}->{$tag}->{$att}->{type} eq "CDATA"));
    }

    push(@{$tree[1]},$self->recursebranch($self->{ELEMENT}->{$tag}));
    return @tree;
}


sub recursebranch
{
    my $self = shift;
    my $dstr = shift;

    my @tree;
    if (($dstr->{type} eq "element") &&
        ($dstr->{element} ne "EMPTY"))
    {
        @tree = $self->newbranch($dstr->{element})
            if (!exists($dstr->{repeat}) ||
                ($dstr->{repeat} eq "+"));
    }
    else
    {
        foreach my $index (0..$#{$dstr->{list}})
        {
            push(@tree,$self->recursebranch($dstr->{list}->[$index]))
if (!exists($dstr->{repeat}) ||
        ($dstr->{repeat} eq "+"));
        }
    }
    return @tree;
}


sub attribs
{
    my $self = shift;
    my ($tag,$tree) = @_;

    return unless exists ($self->{ATTLIST}->{$tag});

    if (defined($tree))
    {
        my %current = &XML::Stream::GetXMLData("attribs",$tree,"","","");
        return $self->allowedattribs($tag,\%current);
    }
    return $self->allowedattribs($tag);
}


sub allowedattribs
{
    my $self = shift;
    my ($tag,$current) = @_;

    my %allowed;
    foreach my $att (keys(%{$self->{ATTLIST}->{$tag}}))
    {
        $allowed{$att} = 1 unless (defined($current) &&
                                   exists($current->{$att}));
    }
    return sort {$a cmp $b} keys(%allowed);
}


sub attribvalue
{
    my $self = shift;
    my $tag = shift;
    my $att = shift;

    return $self->{ATTLIST}->{$tag}->{$att}->{type}
        if ($self->{ATTLIST}->{$tag}->{$att}->{type} ne "list");
    return sort {$a cmp $b} keys(%{$self->{ATTLIST}->{$tag}->{$att}->{value}});
}


sub addattrib
{
    my $self = shift;
    my ($tag,$att,$tree) = @_;

    return $tree unless exists($self->{ATTLIST}->{$tag});
    return $tree unless exists($self->{ATTLIST}->{$tag}->{$att});

    my $default = $self->{ATTLIST}->{$tag}->{$att}->{default};
    $default = "" if ($default eq "#REQUIRED");
    $default = "" if ($default eq "#IMPLIED");

    $$tree[1]->[0]->{$att} = $default;

    return $tree;
}


sub attribrequired
{
    my $self = shift;
    my ($tag,$att) = @_;

    return 0 unless exists($self->{ATTLIST}->{$tag});
    return 0 unless exists($self->{ATTLIST}->{$tag}->{$att});

    return 1 if ($self->{ATTLIST}->{$tag}->{$att}->{default} eq "#REQUIRED");
    return 0;
}


sub deleteattrib
{
    my $self = shift;
    my ($tag,$att,$tree) = @_;

    return $tree unless exists($self->{ATTLIST}->{$tag});
    return $tree unless exists($self->{ATTLIST}->{$tag}->{$att});

    return if $self->attribrequired($tag,$att);

    delete($$tree[1]->[0]->{$att});

    return $tree;
}

