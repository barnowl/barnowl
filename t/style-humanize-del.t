#!/usr/bin/env perl
use strict;
use warnings;
use utf8;

use Test::More qw(no_plan);

use BarnOwl;

=head1 DESCRIPTION

humanize shows DEL (127) as ^? like the other control characters, and
leaves U+00FF (y with diaeresis), which is a printable letter, alone.  It
used to compare with 255 instead of 127, so DEL was shown as \x{7f} and the
letter as ^?.

=cut

{
    no warnings 'redefine';
    my $real_getvar = \&BarnOwl::getvar;
    *BarnOwl::getvar = sub {
        return 'off' if $_[0] eq 'colorztext';
        return $real_getvar->(@_);
    };
}

my $style = 'BarnOwl::Style::Default';

# Without the utf8 flag Perl treats 0x80-0xFF as not printable, so a string
# holding U+00FF only is a letter once it is flagged.  Message text reaches
# humanize decoded, so flag the strings here in the same way.
sub h {
    my ($s, $oneline) = @_;
    utf8::upgrade($s);
    return $style->humanize($s, $oneline);
}

# DEL, with and without the one_line flag, alone and inside a name.
for my $oneline (0, 1) {
    my $mode = $oneline ? 'one line' : 'multi line';
    is(h("\x{7f}", $oneline), '@b(^?)', "DEL, $mode");
    is(h("a\x{7f}b", $oneline), 'a@b(^?)b', "DEL inside text, $mode");
    is(h("\x{7f}\x{7f}", $oneline), '@b(^?)@b(^?)', "two DELs, $mode");
    is(h("\x{7f}\x{1b}[2J", $oneline), '@b(^?)@b(^[)[2J', "DEL then escape, $mode");

    # U+00FF is printable and is not changed, as a character and in a word.
    is(h("\x{ff}", $oneline), "\x{ff}", "U+00FF, $mode");
    is(h("na\x{ef}ve \x{ff}", $oneline), "na\x{ef}ve \x{ff}", "U+00FF in text, $mode");
    is(h("\x{ff}\x{7f}", $oneline), "\x{ff}\@b(^?)", "U+00FF then DEL, $mode");

    # The C1 controls are not DEL: they keep their \x{..} form.
    is(h("\x{80}", $oneline), '@b(\x{80})', "U+0080, $mode");
    is(h("\x{9f}", $oneline), '@b(\x{9f})', "U+009F, $mode");

    # The C0 controls are unchanged.
    is(h("\x{01}", $oneline), '@b(^A)', "SOH, $mode");
    is(h("\x{1f}", $oneline), '@b(^_)', "US, $mode");
}

# With colorztext on, the replacement is cyan.
{
    no warnings 'redefine';
    my $real_getvar = \&BarnOwl::getvar;
    local *BarnOwl::getvar = sub {
        return 'on' if $_[0] eq 'colorztext';
        return $real_getvar->(@_);
    };
    is(h("\x{7f}", 1), '@b(@color(cyan)^?)', 'DEL, colorztext on');
    is(h("\x{ff}", 1), "\x{ff}", 'U+00FF, colorztext on');
}
