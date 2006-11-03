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

package XML::Stream::Tree;

=head1 NAME

XML::Stream::Tree - Functions to make building and parsing the tree easier
to work with.

=head1 SYNOPSIS

  Just a collection of functions that do not need to be in memory if you
choose one of the other methods of data storage.

=head1 FORMAT

The result of parsing:

  <foo><head id="a">Hello <em>there</em></head><bar>Howdy<ref/></bar>do</foo>

would be:
         Tag   Content
  ==================================================================
  [foo, [{},
         head, [{id => "a"},
                0, "Hello ",
                em, [{},
                     0, "there"
                    ]
               ],
         bar, [{},
               0, "Howdy",
               ref, [{}]
              ],
         0, "do"
        ]
  ]

The above was copied from the XML::Parser man page.  Many thanks to
Larry and Clark.

=head1 AUTHOR

By Ryan Eatmon in March 2001 for http://jabber.org/

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use vars qw( $VERSION $LOADED );

$VERSION = "1.22";
$LOADED = 1;

##############################################################################
#
# _handle_element - handles the main tag elements sent from the server.
#                   On an open tag it creates a new XML::Parser::Tree so
#                   that _handle_cdata and _handle_element can add data
#                   and tags to it later.
#
##############################################################################
sub _handle_element
{
    my $self;
    $self = $_[0] if (ref($_[0]) eq "XML::Stream::Parser");
    $self = shift unless (ref($_[0]) eq "XML::Stream::Parser");
    my ($sax, $tag, %att) = @_;
    my $sid = $sax->getSID();

    $self->debug(2,"_handle_element: sid($sid) sax($sax) tag($tag) att(",%att,")");

    my @NEW;
    if($#{$self->{SIDS}->{$sid}->{tree}} < 0)
    {
        push @{$self->{SIDS}->{$sid}->{tree}}, $tag;
    }
    else
    {
        push @{ $self->{SIDS}->{$sid}->{tree}[ $#{$self->{SIDS}->{$sid}->{tree}}]}, $tag;
    }
    push @NEW, \%att;
    push @{$self->{SIDS}->{$sid}->{tree}}, \@NEW;
}


##############################################################################
#
# _handle_cdata - handles the CDATA that is encountered.  Also, in the
#                      spirit of XML::Parser::Tree it combines any sequential
#                      CDATA into one tag.
#
##############################################################################
sub _handle_cdata
{
    my $self;
    $self = $_[0] if (ref($_[0]) eq "XML::Stream::Parser");
    $self = shift unless (ref($_[0]) eq "XML::Stream::Parser");
    my ($sax, $cdata) = @_;
    my $sid = $sax->getSID();

    $self->debug(2,"_handle_cdata: sid($sid) sax($sax) cdata($cdata)");

    return if ($#{$self->{SIDS}->{$sid}->{tree}} == -1);

    $self->debug(2,"_handle_cdata: sax($sax) cdata($cdata)");

    my $pos = $#{$self->{SIDS}->{$sid}->{tree}};
    $self->debug(2,"_handle_cdata: pos($pos)");

    if ($pos > 0 && $self->{SIDS}->{$sid}->{tree}[$pos - 1] eq "0")
    {
        $self->debug(2,"_handle_cdata: append cdata");
        $self->{SIDS}->{$sid}->{tree}[$pos - 1] .= $cdata;
    }
    else
    {
        $self->debug(2,"_handle_cdata: new cdata");
        push @{$self->{SIDS}->{$sid}->{tree}[$#{$self->{SIDS}->{$sid}->{tree}}]}, 0;
        push @{$self->{SIDS}->{$sid}->{tree}[$#{$self->{SIDS}->{$sid}->{tree}}]}, $cdata;
    }  
}


##############################################################################
#
# _handle_close - when we see a close tag we need to pop the last element
#                      from the list and push it onto the end of the previous
#                      element.  This is how we build our hierarchy.
#
##############################################################################
sub _handle_close
{
    my $self;
    $self = $_[0] if (ref($_[0]) eq "XML::Stream::Parser");
    $self = shift unless (ref($_[0]) eq "XML::Stream::Parser");
    my ($sax, $tag) = @_;
    my $sid = $sax->getSID();

    $self->debug(2,"_handle_close: sid($sid) sax($sax) tag($tag)");

    my $CLOSED = pop @{$self->{SIDS}->{$sid}->{tree}};

    $self->debug(2,"_handle_close: check(",$#{$self->{SIDS}->{$sid}->{tree}},")");

    if ($#{$self->{SIDS}->{$sid}->{tree}} == -1)
    {
        if ($self->{SIDS}->{$sid}->{rootTag} ne $tag)
        {
            $self->{SIDS}->{$sid}->{streamerror} = "Root tag mis-match: <$self->{SIDS}->{$sid}->{rootTag}> ... </$tag>\n";
        }
        return;
    }

    if($#{$self->{SIDS}->{$sid}->{tree}} < 1)
    {

        push @{$self->{SIDS}->{$sid}->{tree}}, $CLOSED;

        if (ref($self) ne "XML::Stream::Parser")
        {
            my $stream_prefix = $self->StreamPrefix($sid);

            if(defined($self->{SIDS}->{$sid}->{tree}->[0]) &&
               ($self->{SIDS}->{$sid}->{tree}->[0] =~ /^${stream_prefix}\:/))
            { 
                my @tree = @{$self->{SIDS}->{$sid}->{tree}};
                $self->{SIDS}->{$sid}->{tree} = [];
                $self->ProcessStreamPacket($sid,\@tree);
            }
            else
            {
                my @tree = @{$self->{SIDS}->{$sid}->{tree}};
                $self->{SIDS}->{$sid}->{tree} = [];
                
                my @special =
                    &XML::Stream::XPath(
                        \@tree,
                        '[@xmlns="'.&XML::Stream::ConstXMLNS("xmpp-sasl").'" or @xmlns="'.&XML::Stream::ConstXMLNS("xmpp-tls").'"]'
                    );
                if ($#special > -1)
                {
                    my $xmlns = &GetXMLData("value",\@tree,"","xmlns");

                    $self->ProcessSASLPacket($sid,\@tree)
                        if ($xmlns eq &XML::Stream::ConstXMLNS("xmpp-sasl"));
                    $self->ProcessTLSPacket($sid,\@tree)
                        if ($xmlns eq &XML::Stream::ConstXMLNS("xmpp-tls"));
                }
                else
                {
                    &{$self->{CB}->{node}}($sid,\@tree);
                }
            }
        }
    }
    else
    {
        push @{$self->{SIDS}->{$sid}->{tree}[$#{$self->{SIDS}->{$sid}->{tree}}]}, $CLOSED;
    }
}


##############################################################################
#
# SetXMLData - takes a host of arguments and sets a portion of the specified
#              XML::Parser::Tree object with that data.  The function works
#              in two modes "single" or "multiple".  "single" denotes that
#              the function should locate the current tag that matches this
#              data and overwrite it's contents with data passed in.
#              "multiple" denotes that a new tag should be created even if
#              others exist.
#
#              type    - single or multiple
#              XMLTree - pointer to XML::Stream Tree object
#              tag     - name of tag to create/modify (if blank assumes
#                        working with top level tag)
#              data    - CDATA to set for tag
#              attribs - attributes to ADD to tag
#
##############################################################################
sub SetXMLData
{
    my ($type,$XMLTree,$tag,$data,$attribs) = @_;
    my ($key);

    if ($tag ne "")
    {
        if ($type eq "single")
        {
            my ($child);
            foreach $child (1..$#{$$XMLTree[1]})
            {
                if ($$XMLTree[1]->[$child] eq $tag)
                {
                    if ($data ne "")
                    {
                        #todo: add code to handle writing the cdata again and appending it.
                        $$XMLTree[1]->[$child+1]->[1] = 0;
                        $$XMLTree[1]->[$child+1]->[2] = $data;
                    }
                    foreach $key (keys(%{$attribs}))
                    {
                        $$XMLTree[1]->[$child+1]->[0]->{$key} = $$attribs{$key};
                    }
                    return;
                }
            }
        }
        $$XMLTree[1]->[($#{$$XMLTree[1]}+1)] = $tag;
        $$XMLTree[1]->[($#{$$XMLTree[1]}+1)]->[0] = {};
        foreach $key (keys(%{$attribs}))
        {
            $$XMLTree[1]->[$#{$$XMLTree[1]}]->[0]->{$key} = $$attribs{$key};
        }
        if ($data ne "")
        {
            $$XMLTree[1]->[$#{$$XMLTree[1]}]->[1] = 0;
            $$XMLTree[1]->[$#{$$XMLTree[1]}]->[2] = $data;
        }
    }
    else
    {
        foreach $key (keys(%{$attribs}))
        {
            $$XMLTree[1]->[0]->{$key} = $$attribs{$key};
        }
        if ($data ne "")
        {
            if (($#{$$XMLTree[1]} > 0) &&
                ($$XMLTree[1]->[($#{$$XMLTree[1]}-1)] eq "0"))
            {
                $$XMLTree[1]->[$#{$$XMLTree[1]}] .= $data;
            }
            else
            {
                $$XMLTree[1]->[($#{$$XMLTree[1]}+1)] = 0;
                $$XMLTree[1]->[($#{$$XMLTree[1]}+1)] = $data;
            }
        }
    }
}


##############################################################################
#
# GetXMLData - takes a host of arguments and returns various data structures
#              that match them.
#
#              type - "existence" - returns 1 or 0 if the tag exists in the
#                                   top level.
#                     "value" - returns either the CDATA of the tag, or the
#                               value of the attribute depending on which is
#                               sought.  This ignores any mark ups to the data
#                               and just returns the raw CDATA.
#                     "value array" - returns an array of strings representing
#                                     all of the CDATA in the specified tag.
#                                     This ignores any mark ups to the data
#                                     and just returns the raw CDATA.
#                     "tree" - returns an XML::Parser::Tree object with the
#                              specified tag as the root tag.
#                     "tree array" - returns an array of XML::Parser::Tree
#                                    objects each with the specified tag as
#                                    the root tag.
#                     "child array" - returns a list of all children nodes
#                                     not including CDATA nodes.
#                     "attribs" - returns a hash with the attributes, and
#                                 their values, for the things that match
#                                 the parameters
#                     "count" - returns the number of things that match
#                               the arguments
#                     "tag" - returns the root tag of this tree
#              XMLTree - pointer to XML::Parser::Tree object
#              tag     - tag to pull data from.  If blank then the top level
#                        tag is accessed.
#              attrib  - attribute value to retrieve.  Ignored for types
#                        "value array", "tree", "tree array".  If paired
#                        with value can be used to filter tags based on
#                        attributes and values.
#              value   - only valid if an attribute is supplied.  Used to
#                        filter for tags that only contain this attribute.
#                        Useful to search through multiple tags that all
#                        reference different name spaces.
#
##############################################################################
sub GetXMLData
{
    my ($type,$XMLTree,$tag,$attrib,$value) = @_;

    $tag = "" if !defined($tag);
    $attrib = "" if !defined($attrib);
    $value = "" if !defined($value);

    my $skipthis = 0;

    #---------------------------------------------------------------------------
    # Check if a child tag in the root tag is being requested.
    #---------------------------------------------------------------------------
    if ($tag ne "")
    {
        my $count = 0;
        my @array;
        foreach my $child (1..$#{$$XMLTree[1]})
        {
            next if (($child/2) !~ /\./);
            if (($$XMLTree[1]->[$child] eq $tag) || ($tag eq "*"))
            {
                next if (ref($$XMLTree[1]->[$child]) eq "ARRAY");

                #---------------------------------------------------------------------
                # Filter out tags that do not contain the attribute and value.
                #---------------------------------------------------------------------
                next if (($value ne "") && ($attrib ne "") && exists($$XMLTree[1]->[$child+1]->[0]->{$attrib}) && ($$XMLTree[1]->[$child+1]->[0]->{$attrib} ne $value));
                next if (($attrib ne "") && ((ref($$XMLTree[1]->[$child+1]) ne "ARRAY") || !exists($$XMLTree[1]->[$child+1]->[0]->{$attrib})));

                #---------------------------------------------------------------------
                # Check for existence
                #---------------------------------------------------------------------
                if ($type eq "existence")
                {
                    return 1;
                }
                
                #---------------------------------------------------------------------
                # Return the raw CDATA value without mark ups, or the value of the
                # requested attribute.
                #---------------------------------------------------------------------
                if ($type eq "value")
                {
                    if ($attrib eq "")
                    {
                        my $str = "";
                        my $next = 0;
                        my $index;
                        foreach $index (1..$#{$$XMLTree[1]->[$child+1]}) {
                            if ($next == 1) { $next = 0; next; }
                            if ($$XMLTree[1]->[$child+1]->[$index] eq "0") {
                                $str .= $$XMLTree[1]->[$child+1]->[$index+1];
                                $next = 1;
                            }
                        }
                        return $str;
                    }
                    return $$XMLTree[1]->[$child+1]->[0]->{$attrib}
                        if (exists $$XMLTree[1]->[$child+1]->[0]->{$attrib});
                }
                #---------------------------------------------------------------------
                # Return an array of values that represent the raw CDATA without
                # mark up tags for the requested tags.
                #---------------------------------------------------------------------
                if ($type eq "value array")
                {
                    if ($attrib eq "")
                    {
                        my $str = "";
                        my $next = 0;
                        my $index;
                        foreach $index (1..$#{$$XMLTree[1]->[$child+1]})
                        {
                            if ($next == 1) { $next = 0;  next; }
                            if ($$XMLTree[1]->[$child+1]->[$index] eq "0")
                            {
                                $str .= $$XMLTree[1]->[$child+1]->[$index+1];
                                $next = 1;
                            }
                        }
                        push(@array,$str);
                    }
                    else
                    {
                        push(@array,$$XMLTree[1]->[$child+1]->[0]->{$attrib})
                            if (exists $$XMLTree[1]->[$child+1]->[0]->{$attrib});
                    }
                }
                #---------------------------------------------------------------------
                # Return a pointer to a new XML::Parser::Tree object that has the
                # requested tag as the root tag.
                #---------------------------------------------------------------------
                if ($type eq "tree")
                {
                    my @tree = ( $$XMLTree[1]->[$child] , $$XMLTree[1]->[$child+1] );
                    return @tree;
                }
                #---------------------------------------------------------------------
                # Return an array of pointers to XML::Parser::Tree objects that have
                # the requested tag as the root tags.
                #---------------------------------------------------------------------
                if ($type eq "tree array")
                {
                    my @tree = ( $$XMLTree[1]->[$child] , $$XMLTree[1]->[$child+1] );
                    push(@array,\@tree);
                }
                #---------------------------------------------------------------------
                # Return a count of the number of tags that match
                #---------------------------------------------------------------------
                if ($type eq "count")
                {
                    if ($$XMLTree[1]->[$child] eq "0")
                    {
                        $skipthis = 1;
                        next;
                    }
                    if ($skipthis == 1)
                    {
                        $skipthis = 0;
                        next;
                    }
                    $count++;
                }
                #---------------------------------------------------------------------
                # Return a count of the number of tags that match
                #---------------------------------------------------------------------
                if ($type eq "child array")
                {
                    my @tree = ( $$XMLTree[1]->[$child] , $$XMLTree[1]->[$child+1] );
                    push(@array,\@tree) if ($tree[0] ne "0");
                }
                #---------------------------------------------------------------------
                # Return the attribute hash that matches this tag
                #---------------------------------------------------------------------
                if ($type eq "attribs")
                {
                    return (%{$$XMLTree[1]->[$child+1]->[0]});
                }
            }
        }
        #-------------------------------------------------------------------------
        # If we are returning arrays then return array.
        #-------------------------------------------------------------------------
        if (($type eq "tree array") || ($type eq "value array") ||
            ($type eq "child array"))
        {
            return @array;
        }

        #-------------------------------------------------------------------------
        # If we are returning then count, then do so
        #-------------------------------------------------------------------------
        if ($type eq "count")
        {
            return $count;
        }
    }
    else
    {
        #-------------------------------------------------------------------------
        # This is the root tag, so handle things a level up.
        #-------------------------------------------------------------------------

        #-------------------------------------------------------------------------
        # Return the raw CDATA value without mark ups, or the value of the
        # requested attribute.
        #-------------------------------------------------------------------------
        if ($type eq "value")
        {
            if ($attrib eq "")
            {
                my $str = "";
                my $next = 0;
                my $index;
                foreach $index (1..$#{$$XMLTree[1]})
                {
                    if ($next == 1) { $next = 0; next; }
                    if ($$XMLTree[1]->[$index] eq "0")
                    {
                        $str .= $$XMLTree[1]->[$index+1];
                        $next = 1;
                    }
                }
                return $str;
            }
            return $$XMLTree[1]->[0]->{$attrib}
                if (exists $$XMLTree[1]->[0]->{$attrib});
        }
        #-------------------------------------------------------------------------
        # Return a pointer to a new XML::Parser::Tree object that has the
        # requested tag as the root tag.
        #-------------------------------------------------------------------------
        if ($type eq "tree")
        {
            my @tree =  @{$$XMLTree};
            return @tree;
        }

        #-------------------------------------------------------------------------
        # Return the 1 if the specified attribute exists in the root tag.
        #-------------------------------------------------------------------------
        if ($type eq "existence")
        {
            return 1 if (($attrib ne "") && (exists($$XMLTree[1]->[0]->{$attrib})));
        }

        #-------------------------------------------------------------------------
        # Return the attribute hash that matches this tag
        #-------------------------------------------------------------------------
        if ($type eq "attribs")
        {
            return %{$$XMLTree[1]->[0]};
        }
        #-------------------------------------------------------------------------
        # Return the tag of this node
        #-------------------------------------------------------------------------
        if ($type eq "tag")
        {
            return $$XMLTree[0];
        }
    }
    #---------------------------------------------------------------------------
    # Return 0 if this was a request for existence, or "" if a request for
    # a "value", or [] for "tree", "value array", and "tree array".
    #---------------------------------------------------------------------------
    return 0 if ($type eq "existence");
    return "" if ($type eq "value");
    return [];
}


##############################################################################
#
# BuildXML - takes an XML::Parser::Tree object and builds the XML string
#                 that it represents.
#
##############################################################################
sub BuildXML
{
    my ($parseTree,$rawXML) = @_;

    return "" if $#{$parseTree} == -1;

    my $str = "";
    if (ref($parseTree->[0]) eq "") 
    {
        if ($parseTree->[0] eq "0")
        {
            return &XML::Stream::EscapeXML($parseTree->[1]);
        }

        $str = "<".$parseTree->[0];
        foreach my $att (sort {$a cmp $b} keys(%{$parseTree->[1]->[0]}))
        {
            $str .= " ".$att."='".&XML::Stream::EscapeXML($parseTree->[1]->[0]->{$att})."'";
        }

        if (($#{$parseTree->[1]} > 0) || (defined($rawXML) && ($rawXML ne "")))
        {
            $str .= ">";
            
            my $index = 1;
            while($index <= $#{$parseTree->[1]})
            {
                my @newTree = ( $parseTree->[1]->[$index], $parseTree->[1]->[$index+1] );
                $str .= &XML::Stream::Tree::BuildXML(\@newTree);
                $index += 2;
            }
            
            $str .= $rawXML if defined($rawXML);
            $str .= "</".$parseTree->[0].">";
        }
        else
        {
            $str .= "/>";
        }

    }

    return $str;
}


##############################################################################
#
# XML2Config - takes an XML data tree and turns it into a hash of hashes.
#              This only works for certain kinds of XML trees like this:
#
#                <foo>
#                  <bar>1</bar>
#                  <x>
#                    <y>foo</y>
#                  </x>
#                  <z>5</z>
#                </foo>
#
#              The resulting hash would be:
#
#                $hash{bar} = 1;
#                $hash{x}->{y} = "foo";
#                $hash{z} = 5;
#
#              Good for config files.
#
##############################################################################
sub XML2Config
{
    my ($XMLTree) = @_;

    my %hash;
    foreach my $tree (&XML::Stream::GetXMLData("tree array",$XMLTree,"*"))
    {
        if ($tree->[0] eq "0")
        {
            return $tree->[1] unless ($tree->[1] =~ /^\s*$/);
        }
        else
        {
            if (&XML::Stream::GetXMLData("count",$XMLTree,$tree->[0]) > 1)
            {
                push(@{$hash{$tree->[0]}},&XML::Stream::XML2Config($tree));
            }
            else
            {
                $hash{$tree->[0]} = &XML::Stream::XML2Config($tree);
            }
        }
    }
    return \%hash;
}


1;
