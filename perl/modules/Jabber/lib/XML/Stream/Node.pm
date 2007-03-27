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

package XML::Stream::Node;

=head1 NAME

XML::Stream::Node - Functions to make building and parsing the tree easier
to work with.

=head1 SYNOPSIS

  Just a collection of functions that do not need to be in memory if you
choose one of the other methods of data storage.

  This creates a hierarchy of Perl objects and provides various methods
to manipulate the structure of the tree.  It is much like the C library
libxml.

=head1 FORMAT

The result of parsing:

  <foo><head id="a">Hello <em>there</em></head><bar>Howdy<ref/></bar>do</foo>

would be:

  [ tag:       foo
    att:       {}
    children:  [ tag:      head
                 att:      {id=>"a"}
                 children: [ tag:      "__xmlstream__:node:cdata"
                             children: "Hello "
                           ]
                           [ tag:      em
                             children: [ tag:      "__xmlstream__:node:cdata"
                                         children: "there"
                                       ]
                           ]
               ]
               [ tag:      bar
                 children: [ tag:      "__xmlstream__:node:cdata"
                             children: "Howdy "
                           ]
                           [ tag:      ref
                           ]
               ]
               [ tag:      "__xmlstream__:node:cdata"
                 children: "do"
               ]
  ]

=head1 METHODS

  new()          - creates a new node.  If you specify tag, then the root
  new(tag)         tag is set.  If you specify data, then cdata is added
  new(tag,cdata)   to the node as well.  Returns the created node.

  get_tag() - returns the root tag of the node.

  set_tag(tag) - set the root tag of the node to tag.

  add_child(node)      - adds the specified node as a child to the current
  add_child(tag)         node, or creates a new node with the specified tag
  add_child(tag,cdata)   as the root node.  Returns the node added.

  remove_child(node) - removes the child node from the current node.
  
  remove_cdata() - removes all of the cdata children from the current node.

  add_cdata(string) - adds the string as cdata onto the current nodes
                      child list.

  get_cdata() - returns all of the cdata children concatenated together
                into one string.

  get_attrib(attrib) - returns the value of the attrib if it is valid,
                       or returns undef is attrib is not a real
                       attribute.

  put_attrib(hash) - for each key/value pair specified, create an
                     attribute in the node.

  remove_attrib(attrib) - remove the specified attribute from the node.

  add_raw_xml(string,[string,...]) - directly add a string into the XML
                                     packet as the last child, with no
                                     translation.

  get_raw_xml() - return all of the XML in a single string, undef if there
                  is no raw XML to include.

  remove_raw_xml() - remove all raw XML strings.

  children() - return all of the children of the node in a list.

  attrib() - returns a hash containing all of the attributes on this
             node.

  copy() - return a recursive copy of the node.

  XPath(path) - run XML::Stream::XPath on this node.
  
  XPathCheck(path) - run XML::Stream::XPath on this node and return 1 or 0
                     to see if it matches or not.

  GetXML() - return the node in XML string form.

=head1 AUTHOR

By Ryan Eatmon in June 2002 for http://jabber.org/

=head1 COPYRIGHT

This module is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

use vars qw( $VERSION $LOADED );

$VERSION = "1.22";
$LOADED = 1;

sub new
{
    my $proto = shift;
    my $class = ref($proto) || $proto;

    if (ref($_[0]) eq "XML::Stream::Node")
    {
        return $_[0];
    }

    my $self = {};
    bless($self, $proto);

    my ($tag,$data) = @_;

    $self->set_tag($tag) if defined($tag);
    $self->add_cdata($data) if defined($data);
    $self->remove_raw_xml();

    return $self;
}


sub debug
{
    my $self = shift;
    my ($indent) = @_;

    $indent = "" unless defined($indent);

    if ($self->{TAG} eq "__xmlstream__:node:cdata")
    {
        print        $indent,"cdata(",join("",@{$self->{CHILDREN}}),")\n";
    }
    else
    {
        print        $indent,"packet($self):\n";
        print        $indent,"tag:     <$self->{TAG}\n";
        if (scalar(keys(%{$self->{ATTRIBS}})) > 0)
        {
            print      $indent,"attribs:\n";
            foreach my $key (sort {$a cmp $b} keys(%{$self->{ATTRIBS}}))
            {
                print    $indent,"           $key = '$self->{ATTRIBS}->{$key}'\n";
            }
        }
        if ($#{$self->{CHILDREN}} == -1)
        {
            print      $indent,"         />\n";
        }
        else
        {
            print      $indent,"         >\n";
            print      $indent,"children:\n";
            foreach my $child (@{$self->{CHILDREN}})
            {
                $child->debug($indent."  ");
            }
        }
        print      $indent,"         </$self->{TAG}>\n";
    }
}


sub children
{
    my $self = shift;

    return () unless exists($self->{CHILDREN});
    return @{$self->{CHILDREN}};
}


sub add_child
{
    my $self = shift;

    my $child = new XML::Stream::Node(@_);
    push(@{$self->{CHILDREN}},$child);
    return $child;
}


sub remove_child
{
    my $self = shift;
    my $child = shift;

    foreach my $index (0..$#{$self->{CHILDREN}})
    {
        if ($child == $self->{CHILDREN}->[$index])
        {
            splice(@{$self->{CHILDREN}},$index,1);
            last;
        }
    }
}


sub add_cdata
{
    my $self = shift;
    my $child = new XML::Stream::Node("__xmlstream__:node:cdata");
    foreach my $cdata (@_)
    {
        push(@{$child->{CHILDREN}},$cdata);
    }
    push(@{$self->{CHILDREN}},$child);
    return $child;
}


sub get_cdata
{
    my $self = shift;

    my $cdata = "";
    foreach my $child (@{$self->{CHILDREN}})
    {
        $cdata .= join("",$child->children())
            if ($child->get_tag() eq "__xmlstream__:node:cdata");
    }

    return $cdata;
} 


sub remove_cdata
{
    my $self = shift;

    my @remove = ();
    foreach my $index (0..$#{$self->{CHILDREN}})
    {
        if ($self->{CHILDREN}->[$index]->get_tag() eq "__xmlstream__:node:cdata")
        {

            unshift(@remove,$index);
        }
    }
    foreach my $index (@remove)
    {
        splice(@{$self->{CHILDREN}},$index,1);
    }
} 


sub attrib
{
    my $self = shift;
    return () unless exists($self->{ATTRIBS});
    return %{$self->{ATTRIBS}};
} 


sub get_attrib
{
    my $self = shift;
    my ($key) = @_;

    return unless exists($self->{ATTRIBS}->{$key});
    return $self->{ATTRIBS}->{$key};
}


sub put_attrib
{ 
    my $self = shift;
    my (%att) = @_;

    foreach my $key (keys(%att))
    {
        $self->{ATTRIBS}->{$key} = $att{$key};
    }
}


sub remove_attrib
{
    my $self = shift;
    my ($key) = @_;

    return unless exists($self->{ATTRIBS}->{$key});
    delete($self->{ATTRIBS}->{$key});
}


sub add_raw_xml
{
    my $self = shift;
    my (@raw) = @_;

    push(@{$self->{RAWXML}},@raw);
}

sub get_raw_xml
{
    my $self = shift;

    return if ($#{$self->{RAWXML}} == -1);
    return join("",@{$self->{RAWXML}});
}


sub remove_raw_xml
{
    my $self = shift;
    $self->{RAWXML} = [];
}


sub get_tag
{
    my $self = shift;

    return $self->{TAG};
}


sub set_tag
{
    my $self = shift;
    my ($tag) = @_;

    $self->{TAG} = $tag; 
}


sub XPath
{
    my $self = shift;
    my @results = &XML::Stream::XPath($self,@_);
    return unless ($#results > -1);
    return $results[0] unless wantarray;
    return @results;
}


sub XPathCheck
{
    my $self = shift;
    return &XML::Stream::XPathCheck($self,@_);
}


sub GetXML
{
    my $self = shift;

    return &BuildXML($self,@_);
}


sub copy
{
    my $self = shift;

    my $new_node = new XML::Stream::Node();
    $new_node->set_tag($self->get_tag());
    $new_node->put_attrib($self->attrib());

    foreach my $child ($self->children())
    {
        if ($child->get_tag() eq "__xmlstream__:node:cdata")
        {
            $new_node->add_cdata($self->get_cdata());
        }
        else
        {
            $new_node->add_child($child->copy());
        }
    }

    return $new_node;
}





##############################################################################
#
# _handle_element - handles the main tag elements sent from the server.
#                   On an open tag it creates a new XML::Parser::Node so
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

    $self->debug(2,"Node: _handle_element: sid($sid) sax($sax) tag($tag) att(",%att,")");

    my $node = new XML::Stream::Node($tag);
    $node->put_attrib(%att);

    $self->debug(2,"Node: _handle_element: check(",$#{$self->{SIDS}->{$sid}->{node}},")");

    if ($#{$self->{SIDS}->{$sid}->{node}} >= 0)
    {
        $self->{SIDS}->{$sid}->{node}->[$#{$self->{SIDS}->{$sid}->{node}}]->
            add_child($node);
    }

    push(@{$self->{SIDS}->{$sid}->{node}},$node);
}


##############################################################################
#
# _handle_cdata - handles the CDATA that is encountered.  Also, in the
#                      spirit of XML::Parser::Node it combines any sequential
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

    $self->debug(2,"Node: _handle_cdata: sid($sid) sax($sax) cdata($cdata)");

    return if ($#{$self->{SIDS}->{$sid}->{node}} == -1);

    $self->debug(2,"Node: _handle_cdata: sax($sax) cdata($cdata)");

    $self->{SIDS}->{$sid}->{node}->[$#{$self->{SIDS}->{$sid}->{node}}]->
        add_cdata($cdata);
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

    $self->debug(2,"Node: _handle_close: sid($sid) sax($sax) tag($tag)");

    $self->debug(2,"Node: _handle_close: check(",$#{$self->{SIDS}->{$sid}->{node}},")");

    if ($#{$self->{SIDS}->{$sid}->{node}} == -1)
    {
        $self->debug(2,"Node: _handle_close: rootTag($self->{SIDS}->{$sid}->{rootTag}) tag($tag)");
        if ($self->{SIDS}->{$sid}->{rootTag} ne $tag)
        {
            $self->{SIDS}->{$sid}->{streamerror} = "Root tag mis-match: <$self->{SIDS}->{$sid}->{rootTag}> ... </$tag>\n";
        }
        return;
    }

    my $CLOSED = pop @{$self->{SIDS}->{$sid}->{node}};

    $self->debug(2,"Node: _handle_close: check2(",$#{$self->{SIDS}->{$sid}->{node}},")");
    
    if($#{$self->{SIDS}->{$sid}->{node}} == -1)
    {
        push @{$self->{SIDS}->{$sid}->{node}}, $CLOSED;

        if (ref($self) ne "XML::Stream::Parser")
        {
            my $stream_prefix = $self->StreamPrefix($sid);
            
            if(defined($self->{SIDS}->{$sid}->{node}->[0]) &&
               ($self->{SIDS}->{$sid}->{node}->[0]->get_tag() =~ /^${stream_prefix}\:/))
            {
                my $node = $self->{SIDS}->{$sid}->{node}->[0];
                $self->{SIDS}->{$sid}->{node} = [];
                $self->ProcessStreamPacket($sid,$node);
            }
            else
            {
                my $node = $self->{SIDS}->{$sid}->{node}->[0];
                $self->{SIDS}->{$sid}->{node} = [];

                my @special =
                    &XML::Stream::XPath(
                        $node,
                        '[@xmlns="'.&XML::Stream::ConstXMLNS("xmpp-sasl").'" or @xmlns="'.&XML::Stream::ConstXMLNS("xmpp-tls").'"]'
                    );
                if ($#special > -1)
                {
                    my $xmlns = $node->get_attrib("xmlns");
                    
                    $self->ProcessSASLPacket($sid,$node)
                        if ($xmlns eq &XML::Stream::ConstXMLNS("xmpp-sasl"));
                    $self->ProcessTLSPacket($sid,$node)
                        if ($xmlns eq &XML::Stream::ConstXMLNS("xmpp-tls"));
                }
                else
                {
                    &{$self->{CB}->{node}}($sid,$node);
                }
            }
        }
    }
}


##############################################################################
#
# SetXMLData - takes a host of arguments and sets a portion of the specified
#              XML::Parser::Node object with that data.  The function works
#              in two modes "single" or "multiple".  "single" denotes that
#              the function should locate the current tag that matches this
#              data and overwrite it's contents with data passed in.
#              "multiple" denotes that a new tag should be created even if
#              others exist.
#
#              type    - single or multiple
#              XMLTree - pointer to XML::Stream Node object
#              tag     - name of tag to create/modify (if blank assumes
#                        working with top level tag)
#              data    - CDATA to set for tag
#              attribs - attributes to ADD to tag
#
##############################################################################
sub SetXMLData
{
    my ($type,$XMLTree,$tag,$data,$attribs) = @_;

    if ($tag ne "")
    {
        if ($type eq "single")
        {
            foreach my $child ($XMLTree->children())
            {
                if ($$XMLTree[1]->[$child] eq $tag)
                {
                    $XMLTree->remove_child($child);

                    my $newChild = $XMLTree->add_child($tag);
                    $newChild->put_attrib(%{$attribs});
                    $newChild->add_cdata($data) if ($data ne "");
                    return;
                }
            }
        }
        my $newChild = $XMLTree->add_child($tag);
        $newChild->put_attrib(%{$attribs});
        $newChild->add_cdata($data) if ($data ne "");
    }
    else
    {
        $XMLTree->put_attrib(%{$attribs});
        $XMLTree->add_cdata($data) if ($data ne "");
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
#                     "tree" - returns an XML::Parser::Node object with the
#                              specified tag as the root tag.
#                     "tree array" - returns an array of XML::Parser::Node
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
#              XMLTree - pointer to XML::Parser::Node object
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

    #-------------------------------------------------------------------------
    # Check if a child tag in the root tag is being requested.
    #-------------------------------------------------------------------------
    if ($tag ne "")
    {
        my $count = 0;
        my @array;
        foreach my $child ($XMLTree->children())
        {
            if (($child->get_tag() eq $tag) || ($tag eq "*"))
            {
                #-------------------------------------------------------------
                # Filter out tags that do not contain the attribute and value.
                #-------------------------------------------------------------
                next if (($value ne "") && ($attrib ne "") && $child->get_attrib($attrib) && ($XMLTree->get_attrib($attrib) ne $value));
                next if (($attrib ne "") && !$child->get_attrib($attrib));

                #-------------------------------------------------------------
                # Check for existence
                #-------------------------------------------------------------
                if ($type eq "existence")
                {
                    return 1;
                }
                #-------------------------------------------------------------
                # Return the raw CDATA value without mark ups, or the value of
                # the requested attribute.
                #-------------------------------------------------------------
                if ($type eq "value")
                {
                    if ($attrib eq "")
                    {
                        my $str = $child->get_cdata();
                        return $str;
                    }
                    return $XMLTree->get_attrib($attrib)
                        if defined($XMLTree->get_attrib($attrib));
                }
                #-------------------------------------------------------------
                # Return an array of values that represent the raw CDATA without
                # mark up tags for the requested tags.
                #-------------------------------------------------------------
                if ($type eq "value array")
                {
                    if ($attrib eq "")
                    {
                        my $str = $child->get_cdata();
                        push(@array,$str);
                    }
                    else
                    {
                        push(@array, $XMLTree->get_attrib($attrib))
                        if defined($XMLTree->get_attrib($attrib));
                    }
                }
                #-------------------------------------------------------------
                # Return a pointer to a new XML::Parser::Tree object that has
                # the requested tag as the root tag.
                #-------------------------------------------------------------
                if ($type eq "tree")
                {
                    return $child;
                }
                #-------------------------------------------------------------
                # Return an array of pointers to XML::Parser::Tree objects
                # that have the requested tag as the root tags.
                #-------------------------------------------------------------
                if ($type eq "tree array")
                {
                    push(@array,$child);
                }
                #-------------------------------------------------------------
                # Return an array of pointers to XML::Parser::Tree objects
                # that have the requested tag as the root tags.
                #-------------------------------------------------------------
                if ($type eq "child array")
                {
                    push(@array,$child) if ($child->get_tag() ne "__xmlstream__:node:cdata");
                }
                #-------------------------------------------------------------
                # Return a count of the number of tags that match
                #-------------------------------------------------------------
                if ($type eq "count")
                {
                    $count++;
                }
                #-------------------------------------------------------------
                # Return the attribute hash that matches this tag
                #-------------------------------------------------------------
                if ($type eq "attribs")
                {
                    return $XMLTree->attrib();
                }
            }
        }
        #---------------------------------------------------------------------
        # If we are returning arrays then return array.
        #---------------------------------------------------------------------
        if (($type eq "tree array") || ($type eq "value array") ||
            ($type eq "child array"))
        {
            return @array;
        }

        #---------------------------------------------------------------------
        # If we are returning then count, then do so
        #---------------------------------------------------------------------
        if ($type eq "count")
        {
            return $count;
        }
    }
    else
    {
        #---------------------------------------------------------------------
        # This is the root tag, so handle things a level up.
        #---------------------------------------------------------------------

        #---------------------------------------------------------------------
        # Return the raw CDATA value without mark ups, or the value of the
        # requested attribute.
        #---------------------------------------------------------------------
        if ($type eq "value")
        {
            if ($attrib eq "")
            {
                my $str = $XMLTree->get_cdata();
                return $str;
            }
            return $XMLTree->get_attrib($attrib)
                if $XMLTree->get_attrib($attrib);
        }
        #---------------------------------------------------------------------
        # Return a pointer to a new XML::Parser::Tree object that has the
        # requested tag as the root tag.
        #---------------------------------------------------------------------
        if ($type eq "tree")
        {
            return $XMLTree;
        }

        #---------------------------------------------------------------------
        # Return the 1 if the specified attribute exists in the root tag.
        #---------------------------------------------------------------------
        if ($type eq "existence")
        {
            if ($attrib ne "")
            {
                return ($XMLTree->get_attrib($attrib) eq $value) if ($value ne "");
                return defined($XMLTree->get_attrib($attrib));
            }
            return 0;
        }

        #---------------------------------------------------------------------
        # Return the attribute hash that matches this tag
        #---------------------------------------------------------------------
        if ($type eq "attribs")
        {
            return $XMLTree->attrib();
        }
        #---------------------------------------------------------------------
        # Return the tag of this node
        #---------------------------------------------------------------------
        if ($type eq "tag")
        {
            return $XMLTree->get_tag();
        }
    }
    #-------------------------------------------------------------------------
    # Return 0 if this was a request for existence, or "" if a request for
    # a "value", or [] for "tree", "value array", and "tree array".
    #-------------------------------------------------------------------------
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
    my ($node,$rawXML) = @_;

    my $str = "<".$node->get_tag();

    my %attrib = $node->attrib();

    foreach my $att (sort {$a cmp $b} keys(%attrib))
    {
        $str .= " ".$att."='".&XML::Stream::EscapeXML($attrib{$att})."'";
    }

    my @children = $node->children();
    if (($#children > -1) ||
        (defined($rawXML) && ($rawXML ne "")) ||
        (defined($node->get_raw_xml()) && ($node->get_raw_xml() ne ""))
       )
    {
        $str .= ">";
        foreach my $child (@children)
        {
            if ($child->get_tag() eq "__xmlstream__:node:cdata")
            {
                $str .= &XML::Stream::EscapeXML(join("",$child->children()));
            }
            else
            {
                $str .= &XML::Stream::Node::BuildXML($child);
            }
        }
        $str .= $node->get_raw_xml()
            if (defined($node->get_raw_xml()) &&
                ($node->get_raw_xml() ne "")
               );
        $str .= $rawXML if (defined($rawXML) && ($rawXML ne ""));
        $str .= "</".$node->get_tag().">";
    }
    else
    {
        $str .= "/>";
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
        if ($tree->get_tag() eq "__xmlstream__:node:cdata")
        {
            my $str = join("",$tree->children());
            return $str unless ($str =~ /^\s*$/);
        }
        else
        {
            if (&XML::Stream::GetXMLData("count",$XMLTree,$tree->get_tag()) > 1)
            {
                push(@{$hash{$tree->get_tag()}},&XML::Stream::XML2Config($tree));
            }
            else
            {
                $hash{$tree->get_tag()} = &XML::Stream::XML2Config($tree);
            }
        }
    }
    return \%hash;
}


1;
