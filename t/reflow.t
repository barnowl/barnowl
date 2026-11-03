#!/usr/bin/env perl
use strict;
use warnings;
use utf8;

use Test::More qw(no_plan);
use Time::HiRes qw(time);

use BarnOwl;

binmode(Test::More->builder->$_, ':utf8') for qw(output failure_output todo_output);

# Test wrapping of over-wide message lines in the default style
# (BarnOwl::Style::Reflow and the wrap_long_lines variable).

my $R = \&BarnOwl::Style::Reflow::reflow;

# A line that fits is kept exactly; a line that does not is wrapped.
is($R->("a   b  \@b(c)", 20), "a   b  \@b(c)", 'a line that fits is kept exactly');
is($R->("aaa bbb ccc ddd eee fff", 10), "aaa bbb\nccc ddd\neee fff",
   'a long line is wrapped at spaces');
is($R->("one\n\ntwo\n", 20), "one\n\ntwo\n", 'blank lines and the final newline are kept');

# Lists and quotes.
is($R->("- aaa bbb ccc ddd eee\n  | - aaa bbb ccc ddd eee\n> aaa bbb ccc ddd eee", 20),
   "- aaa bbb ccc ddd\n  eee\n  | - aaa bbb ccc\n  |   ddd eee\n> aaa bbb ccc ddd\n> eee",
   'a list item hangs; quote marks continue');
is($R->("12. aaa bbb ccc ddd eee", 15), "12. aaa bbb ccc\n    ddd eee", 'a numbered item hangs');
is($R->("\x{FF11}\x{FF12}. aaa bbb ccc ddd eee fff", 15),
   "\x{FF11}\x{FF12}. aaa bbb\nccc ddd eee fff",
   'fullwidth digits do not start a numbered item');

# Words.
is($R->('x' x 25, 20), ('x' x 20) . "\n" . ('x' x 5), 'a word wider than a line is cut');
is($R->('see https://example.com/' . ('a' x 30) . ' ok', 20),
   "see\nhttps://example.com/" . ('a' x 30) . "\nok", 'a URL is never split');
is($R->('see www.example.com/' . ('a' x 30), 20),
   "see\nwww.example.com/" . ('a' x 30), 'a www. URL is never split');
is($R->('火' x 15, 20), ('火' x 10) . "\n" . ('火' x 5), 'wide characters take two columns');
is($R->("\x01" x 15, 20), ("\x01" x 10) . "\n" . ("\x01" x 5),
   'a control character takes two columns (^A)');
is($R->("a\t" . ('b ' x 30), 20), "a\t" . ('b ' x 30), 'a line with a tab is left alone');

# ztext.
is($R->('@b(' . ('x' x 20) . ')', 20), '@b(' . ('x' x 20) . ')', 'markup takes no columns');
is($R->('@b(aaaa bbbb cccc dddd eeee)', 10), "\@b(aaaa bbbb\ncccc dddd\neeee)",
   'markup is never split');
is($R->('@@' x 15, 10), ('@@' x 10) . "\n" . ('@@' x 5), '@@ is one column and never split');
is($R->("\@b(start\n)" . ('x' x 20), 20), "\@b(start\n)" . ('x' x 20),
   'a span opened on an earlier line');
is($R->("\@color(red)" . ('x' x 20), 20), "\@color(red)" . ('x' x 20), '@color takes no columns');

# Where the text is drawn differently from what it looks like: the expected
# strings here are worked out by hand from owl_fmtext_append_ztext (fmtext.c).
# A ">" that closes a "<" span is markup, not a quote mark, and a wrapped line
# must not start with one.
is($R->('> @b<aaa bbb ccc ddd eee fff ggg> tail', 14),
   "> \@b<aaa bbb ccc\n  ddd eee fff\n  ggg> tail", 'a quote mark is not carried into a "<" span');
is($R->("\@b<aaa\n> bbb ccc ddd eee fff ggg", 10),
   "\@b<aaa\n> bbb ccc\nddd eee\nfff ggg", 'a ">" that closes a "<" span is not a quote mark');
is($R->('> @b(aaa bbb ccc ddd eee fff ggg) tail', 14),
   "> \@b(aaa bbb ccc\n> ddd eee fff\n> ggg) tail", 'a quote mark is carried into other spans');
is($R->('| > @b<aaa bbb ccc ddd eee fff ggg> tail', 14),
   "| > \@b<aaa bbb\n|   ccc ddd\n|   eee fff\n|   ggg> tail",
   'only the ">" of a quote prefix is left out inside a "<" span');

# fmtext.c opens at most 32 spans; the 33rd and later @b( are drawn as text.
is($R->(('@b(' x 40) . 'aaaa', 20),
   ('@b(' x 38) . "\@b\n(\@b(aaaa", 'with 32 spans open, @ and openers are text');
is($R->(('@b(' x 32) . ('@@' x 12), 10),
   ('@b(' x 32) . ('@' x 10) . "\n" . ('@' x 10) . "\n" . ('@' x 4),
   'with 32 spans open, @@ is two columns');

# An @color( that colorztext draws as nothing takes 7 characters and no columns
# even when nothing closes it; with colorztext off it is text.
is($R->('@color(aaaa bbbb cccc dddd', 10), "\@color(aaaa bbbb\ncccc dddd",
   'an unclosed @color( takes no columns');
is($R->('@color(red) aaaa bbbb cccc', 10), "\@color(red) aaaa bbbb\ncccc",
   'a closed @color() takes no columns');

# fmtext.c looks for the ">" of a @color< in the whole text, across lines, so a
# ">" quote prefix put in front of text after an unclosed @color< would end
# it and hide the text up to there. Such a line gets a prefix without the ">".
is($R->('> @color<aaa bbb ccc ddd eee fff ggg', 14),
   "> \@color<aaa bbb ccc\n  ddd eee fff\n  ggg", 'no ">" prefix after an unclosed @color<');
is($R->('> @color<aaa bbb ccc ddd) eee fff ggg hhh', 14),
   "> \@color<aaa bbb ccc\n  ddd) eee fff\n> ggg hhh",
   'the ">" prefix is back once another special character has ended the @color< scan');
is($R->('> @color(aaa bbb ccc ddd eee fff ggg', 14),
   "> \@color(aaa bbb ccc\n> ddd eee fff\n> ggg", 'only @color< is ended by a ">" prefix');
BarnOwl::set('-q', 'colorztext', 'off');
is($R->('@color(aaaa bbbb cccc dddd', 10), "\@color(aaa\na bbbb\ncccc dddd",
   'with colorztext off, @color( is text');
is($R->('> @color<aaa bbb ccc ddd eee fff ggg', 14),
   "> \@color<aaa\n> bbb ccc ddd\n> eee fff ggg",
   'with colorztext off, @color< is text and quotes continue');
BarnOwl::set('-q', 'colorztext', 'on');

# The default style, with wrap_long_lines off and on.
BarnOwl::Style::Reflow::_register_variables();
is(BarnOwl::getvar('wrap_long_lines'), 'off', 'wrap_long_lines is off by default');

my $width = BarnOwl::Style::Reflow::wrap_width();
my $text = join(' ', ('word') x (int($width / 5) + 5));
sub drawn {
    my %fields = @_;
    my $m = BarnOwl::Message->new(body => $text, %fields);
    return BarnOwl::Style::Default->indent_body($m);
}
sub lines { return scalar(() = $_[0] =~ /\n/g) + 1 }

cmp_ok(length($text), '>', $width, 'the test body is wider than the screen');
is(drawn(), "    $text", 'off: a zephyr is left as sent');
is(lines(drawn(should_wordwrap => 1)), 2, "off: the wordwrap filter still wraps");

BarnOwl::set('-q', 'wrap_long_lines', 'bogus');
is(BarnOwl::getvar('wrap_long_lines'), 'off', 'a bogus value is refused');
BarnOwl::set('-q', 'wrap_long_lines', 'on');
is(BarnOwl::getvar('wrap_long_lines'), 'on', 'wrap_long_lines can be set');

my $on = drawn();
is(lines($on), 2, 'on: a zephyr is wrapped');
is(non_space($on), non_space($text), 'on: nothing is lost or changed');
is(drawn(should_wordwrap => 1), $on, 'on: the wordwrap filter does not wrap a second time');
is(drawn(body => "short\nlines\n"), "    short\n    lines", 'on: short lines are left as sent');
my $half = int($width / 2);
is(drawn(body => ('é' x 30) . ' ' . ('火' x ($half + 5))),
   "    " . ('é' x 30) . "\n    " . ('火' x $half) . "\n    " . ('火' x 5),
   'on: columns, not characters or bytes, decide where a line breaks');

# A message of more than 64 KB is left as sent, and the wordwrap filter then wraps it as before.
my $huge = 'word ' x 20_000;
is($R->($huge, 30), $huge, 'a message over 64 KB is left as sent');
is(drawn(body => $huge), "    $huge", 'on: a message over 64 KB is left as sent');
cmp_ok(lines(drawn(body => $huge, should_wordwrap => 1)), '>', 100,
       'on: a message over 64 KB that the wordwrap filter matches is still wrapped by it');
BarnOwl::set('-q', 'wrap_long_lines', 'off');
is(BarnOwl::getvar('wrap_long_lines'), 'off', 'wrap_long_lines can be unset');
is(drawn(), "    $text", 'off again: a zephyr is left as sent');

# Hostile bodies: Zephyr text is chosen by the sender. Whatever the input, wrapping
# must not die and must keep every non-space character in order, with colorztext
# on or off. Where the text has no markup, no tab and no URL and is printable
# ASCII, every character is one column, so no line may be wider than asked.
sub non_space { my $s = shift; $s =~ s/\s+//g; return $s }
my %hostile = (
    'unclosed markup'        => '@b(' x 50 . 'word ' x 40,
    'closers only'           => ') } ] > ' x 40,
    'bare @'                 => '@ ' x 60,
    'deeply nested markup'   => ('@b[' x 500) . 'x ' x 100 . (']' x 500),
    'color without closer'   => '@color(red ' . 'word ' x 40,
    'bidi and zero width'    => ("\x{202E}ab\x{200B}cd " x 30),
    'control characters'     => ("a\x00b\x1b[31m\x7f\r " x 20),
    'combining marks'        => ("e\x{301}" x 80),
    'non-characters'         => ("\x{FFFF}\x{10FFFF}\x{E000} " x 20),
    'one huge word'          => 'x' x 20000,
    'many short words'       => 'a ' x 10000,
    'only spaces'            => ' ' x 500,
    'quote marks only'       => '> ' x 100,
    'list marker only'       => '- ' x 100,
);
my %plain = map { $_ => 1 } ('closers only', 'bare @', 'one huge word', 'many short words',
                             'only spaces', 'quote marks only', 'list marker only');
for my $color (qw(on off)) {
    BarnOwl::set('-q', 'colorztext', $color);
    for my $name (sort keys %hostile) {
        my $in = $hostile{$name};
        my $out = eval { $R->($in, 30) };
        is($@, '', "hostile ($name, colorztext $color): no error");
        next unless defined $out;
        is(non_space($out), non_space($in), "hostile ($name, colorztext $color): nothing lost");
        next unless $plain{$name};
        my $wide = grep { length($_) > 30 } split /\n/, $out;
        is($wide, 0, "hostile ($name, colorztext $color): no line is wider than asked");
    }
}
BarnOwl::set('-q', 'colorztext', 'on');

# A sender can make one line as long as the message allows, so wrapping must
# take time in proportion to its size: a 200 KB line, in one word or in many,
# plain or not, is wrapped in under a second here, where a quadratic loop needs
# minutes. The limit is far above that, so that a slow builder is not mistaken
# for a quadratic loop. The 64 KB limit is lifted, or nothing would be wrapped.
local $BarnOwl::Style::Reflow::MAX_TEXT = 10_000_000;
my $LIMIT = 30;
my %big = (
    'ascii word'     => 'x' x 200_000,
    'non-ASCII word' => 'é' x 200_000,
    'non-ASCII text' => 'é ' x 100_000,
    'markup'         => '@b(é)' x 40_000,
);
for my $color (qw(on off)) {
    BarnOwl::set('-q', 'colorztext', $color);
    for my $name (sort keys %big) {
        my $in = $big{$name};
        my $start = time;
        my $out = $R->($in, 30);
        my $took = time - $start;
        cmp_ok($took, '<', $LIMIT, "200 KB $name, colorztext $color: wrapped in under $LIMIT s");
        is(non_space($out), non_space($in), "200 KB $name, colorztext $color: nothing lost");
    }
}
BarnOwl::set('-q', 'colorztext', 'on');

1;
