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


##############################################################################
#
# Op - Base Op class
#
##############################################################################
package XML::Stream::XPath::Op;

use 5.006_001;
use strict;
use vars qw( $VERSION );

$VERSION = "1.22";

sub new
{
    my $proto = shift;
    return &allocate($proto,@_);
}

sub allocate
{
    my $proto = shift;
    my $self = { };

    bless($self,$proto);

    $self->{TYPE} = shift;
    $self->{VALUE} = shift;
    
    return $self;
}

sub getValue
{
    my $self = shift;
    return $self->{VALUE};
}

sub calcStr
{
    my $self = shift;
    return $self->{VALUE};
}

sub getType
{
    my $self = shift;
    return $self->{TYPE};
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;
    return 1;
}

sub display
{
    my $self = shift;
    my $space = shift;
    $space = "" unless defined($space);

    print $space,"OP: type($self->{TYPE}) value($self->{VALUE})\n";
}



##############################################################################
#
# PositionOp - class to handle [0] ops
#
##############################################################################
package XML::Stream::XPath::PositionOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $self = $proto->allocate("POSITION","");
    $self->{POS} = shift;

    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my @elems = $$ctxt->getList();
    my @valid_elems;
    if ($#elems+1 < $self->{POS})
    {
        return;
    }

    push(@valid_elems, $elems[$self->{POS}-1]);

    $$ctxt->setList(@valid_elems);

    return 1;
}



##############################################################################
#
# ContextOp - class to handle [...] ops
#
##############################################################################
package XML::Stream::XPath::ContextOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $self = $proto->allocate("CONTEXT","");
    $self->{OP} = shift;
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my @elems = $$ctxt->getList();
    my @valid_elems;
    foreach my $elem (@elems)
    {
        my $tmp_ctxt = new XML::Stream::XPath::Value($elem);
        $tmp_ctxt->in_context(1);
        if ($self->{OP}->isValid(\$tmp_ctxt))
        {
            push(@valid_elems,$elem);
        }   
    }

    $$ctxt->setList(@valid_elems);
    
    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub display
{
    my $self = shift;
    my $space = shift;
    $space = "" unless defined($space);

    print "${space}OP: type(CONTEXT) op: \n";
    $self->{OP}->display("$space    ");
}




##############################################################################
#
# AllOp - class to handle // ops
#
##############################################################################
package XML::Stream::XPath::AllOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $name = shift;
    my $self = $proto->allocate("ALL",$name);
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my @elems = $$ctxt->getList();

    if ($#elems == -1)
    {
        return;
    }

    my @valid_elems;
    
    foreach my $elem (@elems)
    {
        push(@valid_elems,$self->descend($elem));
    }
    
    $$ctxt->setList(@valid_elems);

    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub descend
{
    my $self = shift;
    my $elem = shift;

    my @valid_elems;
    
    if (($self->{VALUE} eq "*") || (&XML::Stream::GetXMLData("tag",$elem) eq $self->{VALUE}))
    {
        push(@valid_elems,$elem);
    }
    
    foreach my $child (&XML::Stream::GetXMLData("child array",$elem,"*"))
    {
        push(@valid_elems,$self->descend($child));
    }
    
    return @valid_elems;
}



##############################################################################
#
# NodeOp - class to handle ops based on node names
#
##############################################################################
package XML::Stream::XPath::NodeOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $name = shift;
    my $is_root = shift;
    $is_root = 0 unless defined($is_root);
    my $self = $proto->allocate("NODE",$name);
    $self->{ISROOT} = $is_root;
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    if ($self->{ISROOT})
    {
        my $elem = $$ctxt->getFirstElem();
        if (&XML::Stream::GetXMLData("tag",$elem) ne $self->{VALUE})
        {
            return;
        }
        return 1;
    }

    my @valid_elems;

    foreach my $elem ($$ctxt->getList())
    {
        my $valid = 0;

        foreach my $child (&XML::Stream::GetXMLData("child array",$elem,"*"))
        {
            if (($self->{VALUE} eq "*") ||
                (&XML::Stream::GetXMLData("tag",$child) eq $self->{VALUE}))
            {
                if ($$ctxt->in_context())
                {
                    $valid = 1;
                }
                else
                {
                    push(@valid_elems,$child);
                }
            }
        }
        if ($valid)
        {
            push(@valid_elems,$elem);
        }
    }
    
    $$ctxt->setList(@valid_elems);

    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub calcStr
{
    my $self = shift;
    my $elem = shift;
    return &XML::Stream::GetXMLData("value",$elem);
} 


##############################################################################
#
# EqualOp - class to handle [ x = y ] ops
#
##############################################################################
package XML::Stream::XPath::EqualOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $self = $proto->allocate("EQUAL","");
    $self->{OP_L} = shift;
    $self->{OP_R} = shift;
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my $tmp_ctxt = new XML::Stream::XPath::Value();
    $tmp_ctxt->setList($$ctxt->getList());
    $tmp_ctxt->in_context(0);
    
    if (!$self->{OP_L}->isValid(\$tmp_ctxt) || !$self->{OP_R}->isValid(\$tmp_ctxt))
    {
        return;
    }

    my @valid_elems;
    foreach my $elem ($tmp_ctxt->getList)
    {
        if ($self->{OP_L}->calcStr($elem) eq $self->{OP_R}->calcStr($elem))
        {
            push(@valid_elems,$elem);
        }
    }

    if ( $#valid_elems > -1)
    {
        @valid_elems = $$ctxt->getList();
    }
    
    $$ctxt->setList(@valid_elems);

    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub display
{
    my $self = shift;
    my $space = shift;
    $space = "" unless defined($space);

    print $space,"OP: type(EQUAL)\n";
    print $space,"    op_l: ";
    $self->{OP_L}->display($space."    ");
    
    print $space,"    op_r: ";
    $self->{OP_R}->display($space."    ");
}



##############################################################################
#
# NotEqualOp - class to handle [ x != y ] ops
#
##############################################################################
package XML::Stream::XPath::NotEqualOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $self = $proto->allocate("NOTEQUAL","");
    $self->{OP_L} = shift;
    $self->{OP_R} = shift;
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my $tmp_ctxt = new XML::Stream::XPath::Value();
    $tmp_ctxt->setList($$ctxt->getList());
    $tmp_ctxt->in_context(0);
    
    if (!$self->{OP_L}->isValid(\$tmp_ctxt) || !$self->{OP_R}->isValid(\$tmp_ctxt))
    {
        return;
    }

    my @valid_elems;
    foreach my $elem ($tmp_ctxt->getList)
    {
        if ($self->{OP_L}->calcStr($elem) ne $self->{OP_R}->calcStr($elem))
        {
            push(@valid_elems,$elem);
        }
    }

    if ( $#valid_elems > -1)
    {
        @valid_elems = $$ctxt->getList();
    }
    
    $$ctxt->setList(@valid_elems);

    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub display
{
    my $self = shift;
    my $space = shift;
    $space = "" unless defined($space);

    print $space,"OP: type(NOTEQUAL)\n";
    print $space,"    op_l: ";
    $self->{OP_L}->display($space."    ");
    
    print $space,"    op_r: ";
    $self->{OP_R}->display($space."    ");
}



##############################################################################
#
# AttributeOp - class to handle @foo ops.
#
##############################################################################
package XML::Stream::XPath::AttributeOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $name = shift;
    my $self = $proto->allocate("ATTRIBUTE",$name);
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my @elems = $$ctxt->getList();
    my @valid_elems;
    my @values;
    my %attribs;
    
    foreach my $elem (@elems)
    {
        if ($self->{VALUE} ne "*")
        {
            if (&XML::Stream::GetXMLData("value",$elem,"",$self->{VALUE}))
            {
                $self->{VAL} = $self->calcStr($elem);
                push(@valid_elems,$elem);
                push(@values,$self->{VAL});
            }
        }
        else
        {
            my %attrib = &XML::Stream::GetXMLData("attribs",$elem);
            if (scalar(keys(%attrib)) > 0)
            {
                push(@valid_elems,$elem);
                foreach my $key (keys(%attrib))
                {
                    $attribs{$key} = $attrib{$key};
                }
            }
        }
    }

    $$ctxt->setList(@valid_elems);
    $$ctxt->setValues(@values);
    $$ctxt->setAttribs(%attribs);

    if ($#valid_elems == -1)
    {
        return;
    }
    
    return 1;
}


sub getValue
{
    my $self = shift;
    return $self->{VAL};
}


sub calcStr
{
    my $self = shift;
    my $elem = shift;
    return &XML::Stream::GetXMLData("value",$elem,"",$self->{VALUE});
}




##############################################################################
#
# AndOp - class to handle [ .... and .... ] ops
#
##############################################################################
package XML::Stream::XPath::AndOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $self = $proto->allocate("AND","and");
    $self->{OP_L} = shift;
    $self->{OP_R} = shift;
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my $opl = $self->{OP_L}->isValid($ctxt);
    my $opr = $self->{OP_R}->isValid($ctxt);
    
    if ($opl && $opr)
    {
        return 1;
    }
    else
    {
        return;
    }
}


sub display
{
    my $self = shift;
    my $space = shift;
    $space = "" unless defined($space);

    print $space,"OP: type(AND)\n";
    print $space,"  op_l: \n";
    $self->{OP_L}->display($space."    ");
    
    print $space,"  op_r: \n";
    $self->{OP_R}->display($space."    ");
}



##############################################################################
#
# OrOp - class to handle [ .... or .... ] ops
#
##############################################################################
package XML::Stream::XPath::OrOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $self = $proto->allocate("OR","or");
    $self->{OP_L} = shift;
    $self->{OP_R} = shift;
    return $self;
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my @elems = $$ctxt->getList();
    my @valid_elems;

    foreach my $elem (@elems)
    {
        my $tmp_ctxt_l = new XML::Stream::XPath::Value($elem);
        $tmp_ctxt_l->in_context(1);
        my $tmp_ctxt_r = new XML::Stream::XPath::Value($elem);
        $tmp_ctxt_r->in_context(1);

        my $opl = $self->{OP_L}->isValid(\$tmp_ctxt_l);
        my $opr = $self->{OP_R}->isValid(\$tmp_ctxt_r);
    
        if ($opl || $opr)
        {
            push(@valid_elems,$elem);
        }   
    }

    $$ctxt->setList(@valid_elems);
    
    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub display
{
    my $self = shift;
    my $space = shift;
    $space = "" unless defined($space);

    print "${space}OP: type(OR)\n";
    print "$space    op_l: ";
    $self->{OP_L}->display("$space    ");
    
    print "$space    op_r: ";
    $self->{OP_R}->display("$space    ");
}



##############################################################################
#
# FunctionOp - class to handle xxxx(...) ops
#
##############################################################################
package XML::Stream::XPath::FunctionOp;

use vars qw (@ISA);
@ISA = ( "XML::Stream::XPath::Op" );

sub new
{
    my $proto = shift;
    my $function = shift;
    my $self = $proto->allocate("FUNCTION",$function);
    $self->{CLOSED} = 0;
    return $self;
}


sub addArg
{
    my $self = shift;
    my $arg = shift;

    push(@{$self->{ARGOPS}},$arg);
}


sub isValid
{
    my $self = shift;
    my $ctxt = shift;

    my $result;
    eval("\$result = &{\$XML::Stream::XPath::FUNCTIONS{\$self->{VALUE}}}(\$ctxt,\@{\$self->{ARGOPS}});");
    return $result;
}


sub calcStr
{
    my $self = shift;
    my $elem = shift;
    
    my $result;
    eval("\$result = &{\$XML::Stream::XPath::VALUES{\$self->{VALUE}}}(\$elem);");
    return $result;

}


sub display
{
    my $self = shift;
    my $space = shift;
    $space = "" unless defined($space);

    print $space,"OP: type(FUNCTION)\n";
    print $space,"    $self->{VALUE}(\n";
    foreach my $arg (@{$self->{ARGOPS}})
    {
        print $arg,"\n";
        $arg->display($space."        ");
    }
    print "$space    )\n";
}


sub function_name
{
    my $ctxt = shift;
    my (@args) = @_;

    my @elems = $$ctxt->getList();
    my @valid_elems;
    my @valid_values;
    foreach my $elem (@elems)
    {
        my $text = &value_name($elem);
        if (defined($text))
        {
            push(@valid_elems,$elem);
            push(@valid_values,$text);
        }   
    }

    $$ctxt->setList(@valid_elems);
    $$ctxt->setValues(@valid_values);
    
    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub function_not
{
    my $ctxt = shift;
    my (@args) = @_;

    my @elems = $$ctxt->getList();
    my @valid_elems;
    foreach my $elem (@elems)
    {
        my $tmp_ctxt = new XML::Stream::XPath::Value($elem);
        $tmp_ctxt->in_context(1);
        if (!($args[0]->isValid(\$tmp_ctxt)))
        {
            push(@valid_elems,$elem);
        }   
    }

    $$ctxt->setList(@valid_elems);
    
    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub function_text
{
    my $ctxt = shift;
    my (@args) = @_;

    my @elems = $$ctxt->getList();
    my @valid_elems;
    my @valid_values;
    foreach my $elem (@elems)
    {
        my $text = &value_text($elem);
        if (defined($text))
        {
            push(@valid_elems,$elem);
            push(@valid_values,$text);
        }   
    }

    $$ctxt->setList(@valid_elems);
    $$ctxt->setValues(@valid_values);
    
    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub function_startswith
{
    my $ctxt = shift;
    my (@args) = @_;

    my @elems = $$ctxt->getList();
    my @valid_elems;
    foreach my $elem (@elems)
    {
        my $val1 = $args[0]->calcStr($elem);
        my $val2 = $args[1]->calcStr($elem);

        if (substr($val1,0,length($val2)) eq $val2)
        {
            push(@valid_elems,$elem);
        }   
    }

    $$ctxt->setList(@valid_elems);
    
    if ($#valid_elems == -1)
    {
        return;
    }

    return 1;
}


sub value_name
{
    my $elem = shift;
    return &XML::Stream::GetXMLData("tag",$elem);
}


sub value_text
{
    my $elem = shift;
    return &XML::Stream::GetXMLData("value",$elem);
}



$XML::Stream::XPath::FUNCTIONS{'name'} = \&function_name;
$XML::Stream::XPath::FUNCTIONS{'not'} = \&function_not;
$XML::Stream::XPath::FUNCTIONS{'text'} = \&function_text;
$XML::Stream::XPath::FUNCTIONS{'starts-with'} = \&function_startswith;

$XML::Stream::XPath::VALUES{'name'} = \&value_name;
$XML::Stream::XPath::VALUES{'text'} = \&value_text;

1;


