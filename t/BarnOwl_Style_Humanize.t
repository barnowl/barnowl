#!/usr/bin/env perl
use strict;
use warnings;

use Test::More qw(no_plan);

use BarnOwl;

=head1 DESCRIPTION

humanize makes characters that would do something funky with the terminal
visible.  Besides control characters, that has to include characters that
draw nothing or reorder text (bidirectional embeddings, overrides and
isolates, Unicode tag characters, zero width space, word joiner and byte
order mark), since a sender could otherwise make a header look different
from what it is.  Ordinary text in any script, including the format
characters that scripts and emoji need, must come out as it went in.

=cut

my $colorztext = 'off';
{
    no warnings 'redefine';
    my $real_getvar = \&BarnOwl::getvar;
    *BarnOwl::getvar = sub {
        return $colorztext if $_[0] eq 'colorztext';
        return $real_getvar->(@_);
    };
    *BarnOwl::zephyr_getrealm = sub () { 'ATHENA.MIT.EDU' };
    *BarnOwl::Style::Default::format_time = sub { '12:00' };
}

my $style = 'BarnOwl::Style::Default';

sub shown {
    return join('', map { sprintf('@b(\\x{%x})', $_) } @_);
}

# The characters that are shown, one per line.
my @hidden = (
    0x200B,                                  # zero width space
    0x2060,                                  # word joiner
    0xFEFF,                                  # zero width no-break space, byte order mark
    0x202A .. 0x202E,                        # bidi embeddings and overrides
    0x2066 .. 0x2069,                        # bidi isolates
    0x2061 .. 0x2064,                        # invisible function application, times, ...
    0x206A .. 0x206F,                        # deprecated format controls
    0xE0000, 0xE0001, 0xE0020, 0xE0041, 0xE0067, 0xE007E, 0xE007F,   # tag characters
);

# Ordinary text.  None of these may change, in either mode.
my @ordinary = (
    "caf\x{E9}", "na\x{EF}ve \x{F1}",
    "\x{65E5}\x{672C}\x{8A9E}",
    "\x{43F}\x{440}\x{438}\x{432}\x{435}\x{442}",
    "\x{5E9}\x{5DC}\x{5D5}\x{5DD}",
    "e\x{301}", "\x{1F600}", "plain ascii; 100% @ {x}", "a\x{A0}b",
    # Khmer, Thai, Lao and Myanmar: the zero width space separates words.
    "\x{1781}\x{17D2}\x{1789}\x{17BB}\x{17C6}\x{200B}\x{179F}\x{17D2}\x{179A}\x{17A1}\x{17B6}\x{1789}\x{17CB}\x{200B}\x{17A2}\x{17D2}\x{1793}\x{1780}",
    "\x{E20}\x{E32}\x{E29}\x{E32}\x{200B}\x{E44}\x{E17}\x{E22}",
    "\x{E9E}\x{EB2}\x{EAA}\x{EB2}\x{200B}\x{EA5}\x{EB2}\x{EA7}",
    "\x{1019}\x{103C}\x{1014}\x{103A}\x{1019}\x{102C}\x{200B}\x{1018}\x{102C}\x{101E}\x{102C}",
    # Persian: the zero width non-joiner is part of the spelling.
    "\x{645}\x{6CC}\x{200C}\x{62E}\x{648}\x{627}\x{647}\x{645}",
    "\x{645}\x{6CC}\x{200C}\x{200C}\x{62E}",
    # Hebrew and Arabic with left-to-right, right-to-left and Arabic letter marks.
    "\x{5E9}\x{5DC}\x{5D5}\x{5DD}\x{200F} (abc)",
    "abc\x{200E} \x{5E9}\x{5DC}\x{5D5}\x{5DD}\x{200F}:",
    "\x{627}\x{644}\x{633}\x{644}\x{627}\x{645}\x{61C}) (abc",
    # Devanagari conjunct with a zero width joiner, and Bengali with a non-joiner.
    "\x{915}\x{94D}\x{200D}\x{937}",
    "\x{995}\x{9CD}\x{200C}\x{9B7}",
    # Soft hyphen.
    "co\x{AD}operate",
    # Ideographic and Mongolian variation selectors.
    "\x{845B}\x{E0100}", "\x{845B}\x{E01EF}", "\x{1820}\x{180B}", "\x{1820}\x{180D}",
    # Emoji: presentation selectors on bases that are not \p{So}, ZWJ sequences, keycaps.
    "\x{203C}\x{FE0F}", "\x{2049}\x{FE0F}", "\x{2194}\x{FE0F}", "\x{3030}\x{FE0F}",
    "\x{303D}\x{FE0F}", "\x{2764}\x{FE0F}", "\x{263A}\x{FE0E}", "1\x{FE0F}\x{20E3}",
    "\x{1F642}\x{200D}\x{2194}\x{FE0F}",                       # head shaking horizontally
    "\x{1F468}\x{200D}\x{1F469}\x{200D}\x{1F467}",             # family
    "\x{1F469}\x{1F3FD}\x{200D}\x{1F4BB}",                     # woman technologist, skin tone
    "\x{1F3F3}\x{FE0F}\x{200D}\x{1F308}",                      # rainbow flag
    "\x{1F468}\x{200D}\x{2695}\x{FE0F}",                       # man health worker
    "\x{1F1EB}\x{1F1F7}",                                      # flag of France
    # The flags of England, Scotland and Wales, in the middle of text.
    "a\x{1F3F4}\x{E0067}\x{E0062}\x{E0065}\x{E006E}\x{E0067}\x{E007F}b",
    "\x{1F3F4}\x{E0067}\x{E0062}\x{E0073}\x{E0063}\x{E0074}\x{E007F}",
    "\x{1F3F4}\x{E0067}\x{E0062}\x{E0077}\x{E006C}\x{E0073}\x{E007F}",
);
# Emoji newer than the Unicode the running perl knows are unassigned code points
# to it, and the multiline mode has always shown those; only check them oneline.
my @newer_emoji = (
    "\x{1FAF1}\x{1F3FB}\x{200D}\x{1FAF2}\x{1F3FC}",           # handshake, skin tones
    "\x{1FAE9}",                                              # face with bags under eyes
);

sub label {
    return join('', map { sprintf('%x ', ord) } split(//, shift));
}

for my $oneline (0, 1) {
    my $mode = $oneline ? 'oneline' : 'multiline';
    for my $cp (@hidden) {
        is($style->humanize("a" . chr($cp) . "b", $oneline), "a" . shown($cp) . "b",
           sprintf("$mode: U+%04X is shown", $cp));
        is($style->humanize_short("a" . chr($cp) . "b"), "a?b",
           sprintf("humanize_short: U+%04X is replaced", $cp));
    }

    # Trojan Source style inputs.
    is($style->humanize("evil\x{202E}gnp.exe", $oneline),
       "evil" . shown(0x202E) . "gnp.exe",
       "$mode: a reversing override cannot hide in text");
    is($style->humanize("\x{2066}a\x{2069}\x{202D}b\x{202C}", $oneline),
       shown(0x2066) . "a" . shown(0x2069) . shown(0x202D) . "b" . shown(0x202C),
       "$mode: isolates and embeddings in a row");
    is($style->humanize("ad\x{200B}min", $oneline),
       "ad" . shown(0x200B) . "min",
       "$mode: a zero-width space inside a word");
    is($style->humanize("\x{FEFF}hello", $oneline),
       shown(0xFEFF) . "hello",
       "$mode: a byte order mark at the start");
    my $tags = join('', map { chr(0xE0000 + ord($_)) } split(//, 'run'));
    is($style->humanize("x${tags}y", $oneline),
       "x" . shown(map { ord } split(//, $tags)) . "y",
       "$mode: ASCII smuggled in tag characters");
    is($style->humanize("\x{1F3F4}${tags}\x{E007F}", $oneline),
       "\x{1F3F4}" . shown((map { ord } split(//, $tags)), 0xE007F),
       "$mode: tag characters are hidden after an emoji too");
    # Only the three real flags are kept: not other tag text, not a flag cut short or
    # followed by more tags, and not the characters of a flag out of order.
    is($style->humanize("\x{1F3F4}\x{E0067}\x{E0062}\x{E0065}\x{E006E}\x{E0067}", $oneline),
       "\x{1F3F4}" . shown(0xE0067, 0xE0062, 0xE0065, 0xE006E, 0xE0067),
       "$mode: a flag without its cancel tag is shown");
    is($style->humanize("\x{1F3F4}\x{E0067}\x{E0062}\x{E0065}\x{E006E}\x{E0067}\x{E007F}\x{E0041}",
                        $oneline),
       "\x{1F3F4}\x{E0067}\x{E0062}\x{E0065}\x{E006E}\x{E0067}\x{E007F}" . shown(0xE0041),
       "$mode: tag characters after a flag are shown");
    is($style->humanize("\x{1F3F4}\x{E0067}\x{E0062}\x{E0075}\x{E0073}\x{E007F}", $oneline),
       "\x{1F3F4}" . shown(0xE0067, 0xE0062, 0xE0075, 0xE0073, 0xE007F),
       "$mode: a subdivision other than England, Scotland and Wales is shown");
    is($style->humanize_short("x${tags}y"), "x???y", "humanize_short: tag characters");
    is($style->humanize_short("\x{1F3F4}\x{E0067}\x{E0062}\x{E0065}\x{E006E}\x{E0067}\x{E007F}"),
       "\x{1F3F4}\x{E0067}\x{E0062}\x{E0065}\x{E006E}\x{E0067}\x{E007F}",
       "humanize_short: the flag of England is kept");

    # Ordinary text is not changed.
    for my $text (@ordinary) {
        is($style->humanize($text, $oneline), $text, "$mode: unchanged: " . label($text));
        is($style->humanize_short($text), $text, "humanize_short: unchanged: " . label($text));
    }

    # The characters of ordinary text are not hidden next to the ones that are.
    is($style->humanize("\x{645}\x{6CC}\x{200C}\x{62E}\x{202E}x\x{200D}", $oneline),
       "\x{645}\x{6CC}\x{200C}\x{62E}" . shown(0x202E) . "x\x{200D}",
       "$mode: a non-joiner is kept next to an override");
    is($style->humanize("\x{200B}\x{1781}\x{200B}\x{179F}\x{200B}a", $oneline),
       shown(0x200B) . "\x{1781}\x{200B}\x{179F}" . shown(0x200B) . "a",
       "$mode: zero-width space is kept only between word-separating script characters");
    is($style->humanize("\x{1781}\x{200B}\x{200B}\x{179F}", $oneline),
       "\x{1781}" . shown(0x200B, 0x200B) . "\x{179F}",
       "$mode: doubled zero-width space in Khmer is shown");
}
for my $text (@newer_emoji) {
    is($style->humanize($text, 1), $text, "oneline: unchanged: " . label($text));
    is($style->humanize_short($text), $text, "humanize_short: unchanged: " . label($text));
}

# Control characters were shown in both modes; line and paragraph separators and
# unassigned code points were only shown in the multiline mode, and still are.
is($style->humanize("a\x{2028}b"), "a" . shown(0x2028) . "b", 'multiline: line separator');
is($style->humanize("a\x{0378}b"), "a" . shown(0x0378) . "b", 'multiline: unassigned code point');
is($style->humanize("a\x{2028}b", 1), "a\x{2028}b", 'oneline: line separator is as before');

# What was already done is still done.
is($style->humanize("a\x{01}b"), 'a@b(^A)b', 'control character');
is($style->humanize("a\rb"), 'a@b(^M)b', 'carriage return');
is($style->humanize("a\x{7F}b", 1), 'a@b(\\x{7f})b', 'delete, oneline');
is($style->humanize("a\nb"), "a\nb", 'a newline is kept when not oneline');
is($style->humanize("a\nb", 1), 'a@b(^J)b', 'a newline is a control character when oneline');
is($style->humanize_short("a\x{01}b"), 'a?b', 'humanize_short on a control character');

# Spaces and newlines are not among the characters that are shown, in either
# mode, also next to ones that are.
for my $oneline (0, 1) {
    my $nl = $oneline ? '@b(^J)' : "\n";
    is($style->humanize("a b  c\td", $oneline), 'a b  c@b(^I)d',
       "spaces are kept, oneline=$oneline");
    is($style->humanize(" a\n b \n", $oneline), " a${nl} b ${nl}",
       "spaces around a newline are kept, oneline=$oneline");
    is($style->humanize("a \x{202E} b\n c", $oneline), 'a ' . shown(0x202E) . " b${nl} c",
       "only the character is shown, not the spaces next to it, oneline=$oneline");
}
is($style->humanize_short("a b\n\x{200B} c"), 'a b?? c',
   'humanize_short keeps spaces, shows a newline and a hidden character as ?');

# A string of bytes is handled as characters, and comes back as characters, so that
# what reaches the C code is valid UTF-8.
for my $oneline (0, 1) {
    my $out = $style->humanize("caf\xE9 \xA0\x85", $oneline);
    ok(utf8::is_utf8($out), "bytes: humanize result is characters, oneline=$oneline");
    is($out, "caf\x{E9} \x{A0}" . shown(0x85), "bytes: handled as Latin-1, oneline=$oneline");
}
ok(utf8::is_utf8($style->humanize_short("caf\xE9")), 'bytes: humanize_short result is characters');
is($style->humanize_short("caf\xE9\x85"), "caf\x{E9}?", 'bytes: humanize_short as Latin-1');

# Several in a row, and with colour.
is($style->humanize("\x{200B}\x{200B}x\x{202E}"), shown(0x200B, 0x200B) . 'x' . shown(0x202E),
   'each one is marked');
$colorztext = 'on';
is($style->humanize("a\x{202E}b"), 'a@b(@color(cyan)\\x{202e})b',
   'highlighted when colorztext is on');
$colorztext = 'off';

# End to end: the header of a message with such an instance shows it.
{
    package TestMessage;
    our @ISA = ('BarnOwl::Message::Zephyr');
    sub is_personal { return shift->{personal}; }
}
my $m = bless {
    type => 'zephyr', direction => 'in', login => 'none',
    class => 'message', instance => "\x{202E}gnp.exe", opcode => '',
    realm => 'ATHENA.MIT.EDU', auth => 'YES',
    sender => "al\x{200B}ice\@ATHENA.MIT.EDU", zsig => "z\x{E0041}", body => "hi\x{2066}",
}, 'TestMessage';
is(BarnOwl::ztext_stylestrip($style->format_message($m)),
   'message / \\x{202e}gnp.exe / al\\x{200b}ice  12:00  (z\\x{e0041})' . "\n" . '    hi\\x{2066}',
   'format_message shows invisible characters in every field');
is(BarnOwl::ztext_stylestrip(BarnOwl::Style::OneLine->format_message($m)),
   sprintf('%s %-13.13s %-11.11s %-12.12s ', '<', 'message', "?gnp.exe", "al?ice") . 'hi?',
   'oneline replaces them with ?');

# ... and ordinary text in a header and a body comes out as it went in.
my $persian = "\x{645}\x{6CC}\x{200C}\x{62E}\x{648}\x{627}\x{647}\x{645}";
my $hebrew = "\x{5E9}\x{5DC}\x{5D5}\x{5DD}\x{200F} (abc)";
my $o = bless {
    type => 'zephyr', direction => 'in', login => 'none',
    class => 'message', instance => $persian, opcode => '',
    realm => 'ATHENA.MIT.EDU', auth => 'YES',
    sender => "alice\@ATHENA.MIT.EDU", zsig => "co\x{AD}operate",
    body => "$persian $hebrew \x{915}\x{94D}\x{200D}\x{937} \x{203C}\x{FE0F} "
          . "\x{1F468}\x{200D}\x{1F469}",
}, 'TestMessage';
is(BarnOwl::ztext_stylestrip($style->format_message($o)),
   "message / $persian / alice  12:00  (co\x{AD}operate)\n    $o->{body}",
   'format_message leaves ordinary text alone');
is(BarnOwl::ztext_stylestrip(BarnOwl::Style::OneLine->format_message($o)),
   sprintf('%s %-13.13s %-11.11s %-12.12s ', '<', 'message', $persian, 'alice') . $o->{body},
   'oneline leaves ordinary text alone');

1;
