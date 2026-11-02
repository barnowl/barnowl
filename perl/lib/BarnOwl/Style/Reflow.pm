use strict;
use warnings;

package BarnOwl::Style::Reflow;

=head1 NAME

BarnOwl::Style::Reflow

=head1 DESCRIPTION

Wraps the lines of a message body that are too wide for the screen, for
L<BarnOwl::Style::Default>. A line that fits is returned exactly as it was
sent; only a line wider than the screen is broken, so a sender that has
already wrapped its text is not disturbed.

The rules:

=over 4

=item *

ztext markup takes no columns and is never split; C<@@> is one column.

=item *

A URL is never split, since a URL broken across lines can no longer be
clicked or copied. If it does not fit on a line of its own it overflows, as
it would without wrapping.

=item *

Any other word wider than a whole line is cut at the edge.

=item *

A list item (C<- >, C<* >, C<1. >) hangs under its text, and quote marks
(C<< > >>, C<  | >) continue on every wrapped line, except where a C<< > >>
would close a C<< < >> span of the sender's markup (then it is left out).

=item *

A line with a tab is left alone: BarnOwl expands tabs later, at columns
that are not known here.

=item *

Characters that occupy two columns (CJK) count as two, combining marks as
none.

=back

BarnOwl formats every message again when the terminal is resized, so the
wrapped text reflows.

=head2 VARIABLE

C<wrap_long_lines> (default off) turns this on for every message the default
style, or a style that inherits its C<indent_body>, draws. Messages that the
C<wordwrap> filter matches are wrapped whatever it says, as before. A message
of more than 64 KB is left as sent.

=cut

$BarnOwl::Hooks::startup->add("BarnOwl::Style::Reflow::_register_variables");

our $WRAP_LONG_LINES = 0;

sub _register_variables {
    BarnOwl::new_variable_full('wrap_long_lines', {
        summary        => 'wrap message lines wider than the screen',
        description    => "When on, the default style (or a style that inherits its indent_body)\n"
                        . "wraps a message body line that would be cut off at the right edge; a line\n"
                        . "that fits is left as sent.  A URL is never split:\n"
                        . "if it does not fit on a line of its own, its end is cut off as before.\n"
                        . "Lines with tabs are left alone, as is a message of more than 64 KB.\n"
                        . "Messages that the wordwrap filter matches are wrapped whatever this says.",
        get_tostring   => sub { $WRAP_LONG_LINES ? 'on' : 'off' },
        set_fromstring => sub {
            die "Valid settings are on/off\n" unless $_[0] eq 'on' || $_[0] eq 'off';
            $WRAP_LONG_LINES = $_[0] eq 'on';
            # Cached formats were made with the old setting.
            BarnOwl::redisplay();
        },
        validsettings  => 'on,off',
        takes_on_off   => 1,
    });
}

# Columns a body line may use. The default style draws a body 7 columns in
# and wraps at getnumcols()-9.
sub wrap_width {
    my $cols = BarnOwl::getnumcols();
    my $w = $cols - 9;
    return $w < 20 ? 20 : $w;
}

# Terminal columns of one character, as the default style draws it.
sub char_cols {
    my $c = shift;
    return 1 if $c =~ /[\x20-\x7E]/;
    return 2 if $c =~ /[\x00-\x1F]/;    # humanize draws ^X
    # humanize draws any other character that is not printable as \x{NN}
    return length(sprintf('\\x{%x}', ord $c)) if $c !~ /\p{XPosixPrint}/;
    return 0 if $c =~ /[\p{Mn}\p{Me}\p{Cf}]/;
    return 2 if $c =~ /[\p{Ea=W}\p{Ea=F}]/;
    return 1;
}

my %CLOSER = ('(' => ')', '[' => ']', '{' => '}', '<' => '>');

# fmtext.c keeps 32 open spans; past that every @ is a literal character.
my $MAX_DEPTH = 32;

# Wrapping takes time and memory in proportion to the text, about 500 bytes of
# memory per character; a body longer than this is left as sent (and the
# default style then does what it did before).
our $MAX_TEXT = 64 * 1024;

# ztext_atoms($line, \@open, $colorztext) -> ([TEXT, COLUMNS, CLOSER], ...)
#
# One atom per drawn character, per "@@", and per piece of markup (0
# columns), so that a line is never broken inside markup. CLOSER is the
# closer that would end the innermost open span when the atom starts ('' if
# none), so a ">" put in front of the atom would end the span. This follows
# owl_fmtext_append_ztext (fmtext.c): @b @bold @i @italic and a bare @ open a
# span; @color(NAME) is markup only under colorztext, and an @color( that is
# not closed by its own closer is dropped, 7 characters and no columns; a
# closer is markup only when it closes the innermost open span; with 32 spans
# open, an @ is a literal. @open carries the open spans on to the next line,
# as BarnOwl's parser does.
#
# An unclosed @color< is special: fmtext.c looks for its ">" in the whole
# text, so a ">" put before the next special character on the line would be
# taken for it, and the text up to there would vanish. Those atoms, and the
# one after them, get CLOSER ">" too, whatever is open.
sub ztext_atoms {
    my ($s, $open, $color) = @_;
    my @atoms;
    my $near = 0;    # the next atom is in or just after an unclosed @color<
    my $len = length $s;
    pos($s) = 0;
    while (pos($s) < $len) {
        my $p = pos($s);
        my $top = @$open ? $open->[-1] : '';
        my $mark = $near ? '>' : $top;
        my $was_near = $near;
        $near = 0;
        if ($s =~ /\G([^\@{\[<()>\]}]+)/gc) {
            push @atoms, map { [$_, char_cols($_), $mark] } split //, $1;
            $near = $was_near;
        } elsif (@$open < $MAX_DEPTH && $s =~ /\G\@\@/gc) {
            push @atoms, ['@@', 1, $mark];
        } elsif (@$open < $MAX_DEPTH && $color && $s =~ /\G\@(?i:color)([(\[{<])/gc) {
            my $opener = $1;
            my $after = pos($s);
            my $closed = $s =~ /\G[^\@{\[<()>\]}]*([)\]}>])/gc && $1 eq $CLOSER{$opener};
            pos($s) = $after unless $closed;
            $near = 1 if !$closed && $opener eq '<';
            push @atoms, [substr($s, $p, pos($s) - $p), 0, $mark];
        } elsif (@$open < $MAX_DEPTH && $s =~ /\G\@(?i:bold|b|italic|i)?([(\[{<])/gc) {
            push @$open, $CLOSER{$1};
            push @atoms, [substr($s, $p, pos($s) - $p), 0, $mark];
        } else {
            my $c = substr($s, $p, 1);
            pos($s) = $p + 1;
            if ($top ne '' && $c eq $top) {
                pop @$open;
                push @atoms, [$c, 0, $mark];
            } else {
                push @atoms, [$c, char_cols($c), $mark];
            }
        }
    }
    return @atoms;
}

sub _sum_cols {
    my $n = 0;
    $n += $_->[1] for @_;
    return $n;
}

# Where a wrapped line's later lines start.
# -> (number of leading characters that are the first line's prefix,
#     the prefix for later lines)
# $closing is true when the line starts inside a "<" span: a ">" there closes
# the span instead of quoting.
sub _prefixes {
    my ($line, $closing) = @_;
    my ($quote) = $closing ? '' : $line =~ /\A((?: *[|>] )*)/;
    my ($lead) = substr($line, length $quote) =~ /\A( *(?:(?:[-*+\x{2022}]|[0-9]{1,3}[.)]) +)?)/;
    return (length($quote) + length($lead), $quote . (' ' x length $lead));
}

my $URL = qr{\A[^\w]*(?:[a-z][a-z0-9+.\-]*://|www\.)}i;

# Wraps one line's atoms to $width columns. A word wider than the room left
# moves to a new line; one wider than a whole line is cut between atoms,
# except a URL, which is left whole and overflows. A later line carries the
# first line's quote marks and list indent, but its ">" marks become spaces
# where a ">" would close a "<" span.
sub _wrap_atoms {
    my ($atoms, $width, $line, $closing) = @_;
    my ($nfirst, $cont) = _prefixes($line, $closing);
    my @a = @$atoms;
    my @first = splice(@a, 0, $nfirst);
    push @first, shift @a while @a && $a[0][0] eq ' ';
    if (_sum_cols(@first) > $width / 2) {
        # An absurd indent or run of quote marks: do not carry it along.
        @a = @$atoms;
        @first = ();
        $cont = '';
        shift @a while @a && $a[0][0] eq ' ';
    }
    # Where a ">" would close a span, quote marks are left out; keep the rest.
    my $quiet = $cont;
    $quiet =~ tr/>/ /;
    my @words;    # [ [word atoms], [space atoms after it] ]
    for my $x (@a) {
        if ($x->[0] eq ' ') {
            push @{ $words[-1][1] }, $x;
        } else {
            push @words, [[], []] if !@words || @{ $words[-1][1] };
            push @{ $words[-1][0] }, $x;
        }
    }
    my $prefix = join '', map { $_->[0] } @first;
    my $pcols = _sum_cols(@first);
    my (@lines, @cur);
    my $curcols = 0;
    my $room = sub { my $r = $width - $pcols; $r < 10 ? 10 : $r };
    my $flush = sub {
        pop @cur while @cur && $cur[-1][0] eq ' ';
        # A later line's prefix depends on how it starts, so it is chosen here.
        my $pre = $prefix;
        $pre = (@cur && $cur[0][2] eq '>') ? $quiet : $cont if !defined $pre;
        push @lines, $pre . join('', map { $_->[0] } @cur);
        ($prefix, $pcols, $curcols) = (undef, length $cont, 0);
        @cur = ();
    };
    for my $w (@words) {
        my ($wa, $gap) = @$w;
        my $wc = _sum_cols(@$wa);
        if (@cur && $curcols + $wc <= $room->()) {
            push @cur, @$wa, @$gap;
            $curcols += $wc + @$gap;
            next;
        }
        $flush->() if @cur;
        my @rest = @$wa;
        if ($wc > $room->()
            && join('', map { $_->[1] ? $_->[0] : '' } @rest) =~ $URL) {
            @cur = @rest;
            $flush->();
            next;
        }
        # $left is the columns of @rest, kept as atoms leave it so that a
        # long word is not summed again for every line cut from it.
        my $left = $wc;
        while ($left > $room->()) {
            my $room_now = $room->();
            my $used = 0;
            while (@rest && $used + $rest[0][1] <= $room_now) {
                $used += $rest[0][1];
                push @cur, shift @rest;
            }
            # Always make progress, even if one character is wider than the room.
            if (!@cur && @rest) {
                $used += $rest[0][1];
                push @cur, shift @rest;
            }
            $left -= $used;
            $flush->();
        }
        @cur = (@rest, @$gap);
        $curcols = $left + @$gap;
    }
    $flush->() if @cur || !@lines;
    return @lines;
}

=head2 reflow TEXT WIDTH

Returns the ztext TEXT with every line wider than WIDTH columns wrapped.
Lines that fit, and lines with tabs, are returned unchanged.

=cut

sub reflow {
    my ($text, $width) = @_;
    return $text if length($text) > $MAX_TEXT;
    my $color = (BarnOwl::getvar('colorztext') eq 'on');
    my @open;
    my @out;
    for my $line (split /\n/, $text, -1) {
        if (!@open && $line =~ /\A[\x20-\x3F\x41-\x7E]*\z/) {    # plain ASCII, no markup
            push @out, length($line) <= $width ? $line
                : _wrap_atoms([map { [$_, 1, ''] } split //, $line], $width, $line);
            next;
        }
        my $closing = @open && $open[-1] eq '>';
        my @atoms = ztext_atoms($line, \@open, $color);
        if ($line =~ /\t/ || _sum_cols(@atoms) <= $width) {
            push @out, $line;
            next;
        }
        push @out, _wrap_atoms(\@atoms, $width, $line, $closing);
    }
    return join("\n", @out);
}

1;
