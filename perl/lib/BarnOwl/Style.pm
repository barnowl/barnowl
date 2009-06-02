use strict;
use warnings;

package BarnOwl::Style;

use BarnOwl::Style::Basic;
use BarnOwl::Style::Default;
use BarnOwl::Style::Legacy;
use BarnOwl::Style::OneLine;

# This takes a zephyr to be displayed and modifies it to be displayed
# entirely in bold.
sub boldify
{
    local $_ = shift;
    if ( !(/\)/) ) {
        return '@b(' . $_ . ')';
    } elsif ( !(/\>/) ) {
        return '@b<' . $_ . '>';
    } elsif ( !(/\}/) ) {
        return '@b{' . $_ . '}';
    } elsif ( !(/\]/) ) {
        return '@b[' . $_ . ']';
    } else {
        my $txt = "\@b($_";
        $txt =~ s/\)/\)\@b\[\)\]\@b\(/g;
        return $txt . ')';
    }
}

sub style_command {
    my $command = shift;
    if(scalar @_ != 3 || $_[1] ne 'perl') {
        die("Usage: style <name> perl <function>\n");
    }
    my $name = shift;
    my $perl = shift;
    my $fn   = shift;
    {
        # For historical reasons, assume unqualified references are
        # in main::
        package main;
        no strict 'refs';
        unless(*{$fn}{CODE}) {
            die("Unable to create style '$name': no perl function '$fn'\n");
        }
    }
    BarnOwl::create_style($name, BarnOwl::Style::Legacy->new($fn));
}


1;
