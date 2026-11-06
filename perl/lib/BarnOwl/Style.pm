use strict;
use warnings;

package BarnOwl::Style;

use BarnOwl::Style::Basic;
use BarnOwl::Style::Default;
use BarnOwl::Style::Legacy;
use BarnOwl::Style::OneLine;

# This takes a zephyr to be displayed and modifies it to be displayed
# entirely in bold.
#
# The text is ztext, not plain text: callers wrap their own markup
# (e.g. "@b(PING)") around it, so a closer in the text may be markup.
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
        return _boldify_all_closers($_);
    }
}

# Every closer occurs in the text, so there is no delimiter that can
# wrap it untouched.  Walk the text the way owl_fmtext_append_ztext
# does, to tell the ")" that closes markup inside the text from the
# ones that are literal at the top level (those would end our "@b(")
# and are written as "@b[)]" instead.  Everything else is copied, so
# the text draws as it did before.  Unclosed markup is closed so that
# it can neither swallow our closer nor bleed into what follows.
#
# If the result would not draw the same text (e.g. nesting so deep that
# the extra level makes the 32-level ztext limit bite), return the text
# unchanged rather than garbled.
sub _boldify_all_closers
{
    my $text = shift;
    my $color = BarnOwl::getvar('colorztext') eq 'on';
    my %closer = ('(' => ')', '<' => '>', '[' => ']', '{' => '}');
    my @stack;
    my $out = '';

    pos($text) = 0;
    while (pos($text) < length($text)) {
        if ($text =~ /\G([^\@{\[<()>\]}]+)/gc) {
            $out .= $1;
        } elsif (substr($text, pos($text), 1) eq '@') {
            # With one more level than the original, the parser would
            # stop recognising markup earlier.
            return $text if @stack >= 31;
            if ($text =~ /\G\@\@/gc) {
                $out .= '@@';
            } elsif ($text =~ /\G(\@(?:bold|b|i|italic)?([(<\[{]))/gci) {
                push @stack, $2;
                $out .= $1;
            } elsif ($color && $text =~ /\G(\@color([(<\[{]))/gci) {
                # A color command takes up to its own closer, and does
                # not nest.
                # Unterminated, it draws nothing, but would take the
                # first closer we append as its terminator: drop it.
                my $cmd = $1;
                my $end = $closer{$2};
                if ($text =~ /\G([^\@{\[<()>\]}]*\Q$end\E)/gc) {
                    $out .= $cmd . $1;
                }
            } else {
                # A literal @.
                pos($text) = pos($text) + 1;
                $out .= '@@';
            }
        } elsif ($text =~ /\G([)>\]}])/gc) {
            my $c = $1;
            if (@stack && $closer{$stack[-1]} eq $c) {
                pop @stack;
                $out .= $c;
            } elsif (!@stack && $c eq ')') {
                $out .= '@b[)]';
            } else {
                $out .= $c;
            }
        } else {
            # A literal opener.
            $text =~ /\G(.)/gcs;
            $out .= $1;
        }
    }
    $out .= $closer{$_} for reverse @stack;

    my $bold = '@b(' . $out . ')';
    return $text unless BarnOwl::ztext_stylestrip($bold) eq BarnOwl::ztext_stylestrip($text);
    return $bold;
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
