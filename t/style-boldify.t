#!/usr/bin/env perl
use strict;
use warnings;
use utf8;

use Test::More qw(no_plan);
binmode(Test::More->builder->$_, ':utf8')
    for qw(output failure_output todo_output);

use BarnOwl;

# boldify takes ztext and must draw exactly that text, in bold, and
# leave the markup around it alone.  We can't see attributes from here,
# but ztext_stylestrip gives the text that ztext draws, which is where
# stray characters show up.

sub drawn { return BarnOwl::ztext_stylestrip(shift); }

is(BarnOwl::Style::boldify('abc'), '@b(abc)', 'no closers: @b()');
is(BarnOwl::Style::boldify('a)b'), '@b<a)b>', 'no >: @b<>');
is(BarnOwl::Style::boldify('a)>b'), '@b{a)>b}', 'no }: @b{}');
is(BarnOwl::Style::boldify('a)>}b'), '@b[a)>}b]', 'no ]: @b[]');

# Texts with all four closers, which have no delimiter that can wrap
# them as they are.  Some are ztext of the kind callers build.
my @hostile = (
    'a)b>c}d]e',
    ')>}]',
    '@b(PING) [-i x)>}]] from @b(bob)',
    '@b(PING) [-i x)>}]] from @b(bob)@b(',
    '@b(a) @i<b> @bold{c} @italic[d] @(e)',
    ")>}]\n\@b(two\nlines)\n",
    'a@)>}]',
    'a)>}]@',
    '@@)>}]@@',
    '@@@)>}]@@@',
    '@foo(bar)>}]',
    '@b)>}]',
    '@b (x)>}]',
    '@@b(x)>}]',
    '@color(red)x)>}]',
    '@color(x>)}]',
    '@color(red}x)>]',
    ')>}]@color(',
    ')>}]@b{@color{',
    ')>}]@color(red',
    ')>}]@i[@color<x@@',
    ')>}]@i{unclosed',
    ')>}]@i<unclosed @b(and more',
    ')>}]@b(@b(@b(',
    '(((<<<[[[{{{)>}]',
    '{[<()>]}',
    '}>)]@b(',
    "“quoted” (é) <ü> {ß} [ø]",
    '@' x 40 . ')>}]',
    ('@b(' x 29) . 'x)>}]',
    ('@b(' x 30) . 'x)>}]',
    ('@b(' x 31) . 'x)>}]',
    ('@' . '(' x 3) . ')>}]',
    ')' x 100 . '>}]',
    '@b[)]@b[>]@b[}]@b[]]',
);

# Markup nested so deeply that the extra level of the wrapper would hit
# the 32-level ztext limit can't be wrapped correctly: it must come back
# as it was, not garbled.
my @deep = (
    ('@b(' x 32) . 'x)>}]',
    ('@b(' x 40) . 'x)>}]',
    ('@i[' x 35) . 'x)>}]' . (']' x 35),
);

for my $colorztext ('on', 'off') {
    BarnOwl::set('-q', 'colorztext', $colorztext);
    for my $text (@hostile) {
        my $label = "colorztext $colorztext: " . substr($text, 0, 40);
        $label =~ s/\n/\\n/g;
        my $bold = BarnOwl::Style::boldify($text);
        like($bold, qr/^\@b\(.*\)$/s, "$label: wrapped in bold");
        is(drawn($bold), drawn($text), "$label: draws the same text");
        is(drawn("pre) $bold post)"), "pre) " . drawn($text) . " post)",
           "$label: outside text is not disturbed");
        # Unclosed markup must not swallow our closer: if it does, the
        # bold stays open and eats this ) (stylestrip can't see bold).
        is(drawn($bold . ')'), drawn($text) . ')',
           "$label: bold does not leak");
        is(drawn("\@b(pre) [$bold] \@i<post>"),
           "pre [" . drawn($text) . "] post",
           "$label: outside markup is not disturbed");
    }
    for my $text (@deep) {
        is(BarnOwl::Style::boldify($text), $text,
           "colorztext $colorztext: too deeply nested markup is left plain");
    }
}

# Through the default style: an incoming personal ping whose instance
# holds all four closers.
{
    package TestPing;
    our @ISA = ('BarnOwl::Message::Zephyr');
    sub is_personal { return 1; }
    sub pretty_sender { return shift->sender; }
}

sub ping_with_instance {
    my $instance = shift;
    return TestPing->new(
        type => 'zephyr',
        direction => 'in',
        login => 'none',
        class => 'message',
        instance => $instance,
        opcode => 'ping',
        sender => 'bob',
        recipient => 'alice',
        body => '',
    );
}

BarnOwl::set('-q', 'colorztext', 'off');
my $style = 'BarnOwl::Style::Default';
my $m = ping_with_instance('x)>}]');
is(drawn($style->format_message($m)), 'PING [-i x)>}]] from bob',
   'incoming personal ping draws its text without a stray )');

for my $instance ('x)>}]', 'a)>}]@b(', '@b(x)>}]', 'a@)>}]', "a b)>}]'\"", ')>}]@i{',
                  "x)>}]\n\@b(") {
    my $m = ping_with_instance($instance);
    my $label = "ping instance $instance";
    $label =~ s/\n/\\n/g;
    is(drawn($style->format_message($m)), drawn($style->format_ping($m)),
       "$label: the ping draws the same bold or not");
}
