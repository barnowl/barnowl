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

package XML::Stream::XPath::Query;

use 5.006_001;
use strict;
use Carp;
use vars qw( $VERSION );

$VERSION = "1.22";

sub new
{
    my $proto = shift;
    my $self = { };

    bless($self,$proto);

    $self->{TOKENS} = [ '/','[',']','@','"',"'",'=','!','(',')',':',' ',',']; 
    $self->{QUERY} = shift;
    
    if (!defined($self->{QUERY}) || ($self->{QUERY} eq ""))
    {
        confess("No query string specified");
    }
    
    $self->parseQuery();
    
    return $self;
}


sub getNextToken
{ 
    my $self = shift;
    my $pos = shift;

    my @toks = grep{ $_ eq substr($self->{QUERY},$$pos,1)} @{$self->{TOKENS}};
    while( $#toks == -1 )
    {
        $$pos++;
        if ($$pos > length($self->{QUERY}))
        {
            $$pos = length($self->{QUERY});
            return 0;
        }
        @toks = grep{ $_ eq substr($self->{QUERY},$$pos,1)} @{$self->{TOKENS}};
    }

    return $toks[0];
}


sub getNextIdentifier
{
    my $self = shift;
    my $pos = shift;
    my $sp = $$pos;
    $self->getNextToken($pos);
    return substr($self->{QUERY},$sp,$$pos-$sp);
}


sub getOp
{
    my $self = shift;
    my $pos = shift;
    my $in_context = shift;
    $in_context = 0 unless defined($in_context);

    my $ret_op;

    my $loop = 1;
    while( $loop )
    {
        my $pos_start = $$pos;

        my $token = $self->getNextToken($pos);
        if (($token eq "0") && $in_context)
        {
            return;
        }

        my $token_start = ++$$pos;
        my $ident;
     
        if (defined($token))
        {

            if ($pos_start != ($token_start-1))
            {
                $$pos = $pos_start;
                my $temp_ident = $self->getNextIdentifier($pos);
                $ret_op = new XML::Stream::XPath::NodeOp($temp_ident,"0");
            }
            elsif ($token eq "/")
            {
                if (substr($self->{QUERY},$token_start,1) eq "/")
                {
                    $$pos++;
                    my $temp_ident = $self->getNextIdentifier($pos);
                    $ret_op = new XML::Stream::XPath::AllOp($temp_ident);
                }
                else
                {
                    my $temp_ident = $self->getNextIdentifier($pos);
                    if ($temp_ident ne "")
                    {
                        $ret_op = new XML::Stream::XPath::NodeOp($temp_ident,($pos_start == 0 ? "1" : "0"));
                    }
                }
            }
            elsif ($token eq "\@")
            {
                $ret_op = new XML::Stream::XPath::AttributeOp($self->getNextIdentifier($pos));
            }
            elsif ($token eq "]")
            {
                if ($in_context eq "[")
                {
                    $ret_op = pop(@{$self->{OPS}});
                    $in_context = 0;
                }
                else
                {
                    confess("Found ']' but not in context");
                    return;
                }
            }
            elsif (($token eq "\"") || ($token eq "\'"))
            {
                $$pos = index($self->{QUERY},$token,$token_start);
                $ret_op = new XML::Stream::XPath::Op("LITERAL",substr($self->{QUERY},$token_start,$$pos-$token_start));
                $$pos++;
            }
            elsif ($token eq " ")
            {
                $ident = $self->getNextIdentifier($pos);
                if ($ident eq "and")
                {
                    $$pos++;
                    my $tmp_op = $self->getOp($pos,$in_context);
                    if (!defined($tmp_op))
                    {
                        confess("Invalid 'and' operation");
                        return;
                    }
                    $ret_op = new XML::Stream::XPath::AndOp($self->{OPS}->[$#{$self->{OPS}}],$tmp_op);
                    $in_context = 0;
                    pop(@{$self->{OPS}});
                }
                elsif ($ident eq "or")
                {
                    $$pos++;
                    my $tmp_op = $self->getOp($pos,$in_context);
                    if (!defined($tmp_op))
                    {
                        confess("Invalid 'or' operation");
                        return;
                    }
                    $ret_op = new XML::Stream::XPath::OrOp($self->{OPS}->[$#{$self->{OPS}}],$tmp_op);
                    $in_context = 0;
                    pop(@{$self->{OPS}});
                }
            }
            elsif ($token eq "[")
            {
                if ($self->getNextToken($pos) eq "]")
                {
                    if ($$pos == $token_start)
                    {
                        confess("Nothing in the []");
                        return;
                    }
                    
                    $$pos = $token_start;
                    my $val = $self->getNextIdentifier($pos);
                    if ($val =~ /^\d+$/)
                    {
                        $ret_op = new XML::Stream::XPath::PositionOp($val);
                        $$pos++;
                    }
                    else
                    {
                        $$pos = $pos_start + 1;
                        $ret_op = new XML::Stream::XPath::ContextOp($self->getOp($pos,$token));
                    }
                }
                else
                {
                    $$pos = $pos_start + 1;
                    $ret_op = new XML::Stream::XPath::ContextOp($self->getOp($pos,$token));
                }
            }
            elsif ($token eq "(")
            {
                #-------------------------------------------------------------
                # The function name would have been mistaken for a NodeOp.
                # Pop it off the back and get the function name.
                #-------------------------------------------------------------
                my $op = pop(@{$self->{OPS}});
                if ($op->getType() ne "NODE")
                {
                    confess("No function name specified.");
                }
                my $function = $op->getValue();
                if (!exists($XML::Stream::XPath::FUNCTIONS{$function}))
                {
                    confess("Undefined function \"$function\"");
                }
                $ret_op = new XML::Stream::XPath::FunctionOp($function);

                my $op_pos = $#{$self->{OPS}} + 1;

                $self->getOp($pos,$token);
                
                foreach my $arg ($op_pos..$#{$self->{OPS}})
                {
                    $ret_op->addArg($self->{OPS}->[$arg]);
                }

                splice(@{$self->{OPS}},$op_pos);
                
            }
            elsif ($token eq ")")
            {
                if ($in_context eq "(")
                {
                    $ret_op = undef;
                    $in_context = 0;
                }
                else
                {
                    confess("Found ')' but not in context");
                }
            }
            elsif ($token eq ",")
            {
                if ($in_context ne "(")
                {
                    confess("Found ',' but not in a function");
                }
  
            }
            elsif ($token eq "=")
            {
                my $tmp_op;
                while(!defined($tmp_op))
                {
                    $tmp_op = $self->getOp($pos);
                }
                $ret_op = new XML::Stream::XPath::EqualOp($self->{OPS}->[$#{$self->{OPS}}],$tmp_op);
                pop(@{$self->{OPS}});
            }
            elsif ($token eq "!")
            {
                if (substr($self->{QUERY},$token_start,1) ne "=")
                {
                    confess("Badly formed !=");
                }
                $$pos++;
                
                my $tmp_op;
                while(!defined($tmp_op))
                {
                    $tmp_op = $self->getOp($pos);
                }
                $ret_op = new XML::Stream::XPath::NotEqualOp($self->{OPS}->[$#{$self->{OPS}}],$tmp_op);
                pop(@{$self->{OPS}});
            }
            else
            {
                confess("Unhandled \"$token\"");
            }

            if ($in_context)
            {
                if (defined($ret_op))
                {
                    push(@{$self->{OPS}},$ret_op);
                }
                $ret_op = undef;
            }
        }
        else
        {
            confess("Token undefined");
        }
        
        $loop = 0 unless $in_context;
    }

    return $ret_op;
}


sub parseQuery
{
    my $self = shift;
    my $query = shift;

    my $op;
    my $pos = 0;
    while($pos < length($self->{QUERY}))
    {
        $op = $self->getOp(\$pos);
        if (defined($op))
        {
            push(@{$self->{OPS}},$op);
        }
    }

    #foreach my $op (@{$self->{OPS}})
    #{
    #    $op->display();
    #}

    return 1;
}


sub execute
{
    my $self = shift;
    my $root = shift;

    my $ctxt = new XML::Stream::XPath::Value($root);

    foreach my $op (@{$self->{OPS}})
    {
        if (!$op->isValid(\$ctxt))
        {
            $ctxt->setValid(0);
            return $ctxt;
        }
    }

    $ctxt->setValid(1);
    return $ctxt;
}


sub check
{
    my $self = shift;
    my $root = shift;

    my $ctxt = $self->execute($root);
    return $ctxt->check();
}


1;

