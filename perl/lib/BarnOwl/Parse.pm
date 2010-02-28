use warnings;
use strict;

package BarnOwl::Parse;

use base qw(Exporter);
our @EXPORT_OK = qw(tokenize tokenize_with_point);

# TODO: have the main function return whether or not it was a valid parse, with
# possible error messages or something.  (Still give a parse of some sort on
# invalid parses, just let us know it's invalid if we care.) This is to
# implement command-line-ish things in Perl.

=for doc

Ideally, this should use the same codepath we use to /actually/
tokenize commands, but for now, make sure this is kept in sync with
owl_parseline in util.c

Unlike owl_parseline, we always return a result, even in the presence
of parse errors, since we may be called on incomplete command-lines.

The owl_parseline rules are:

* Tokenize on ' ' and '\t'
* ' and " are quote characters
* \ has no effect

=cut

my $boring = qr{[^'" \t]};
my $quote  = qr{['"]};
my $space  = qr{[ \t]};

sub tokenize_with_point {
    my $line = shift;
    my $point = shift;

    my @words = ();
    my $cword = 0;
    my $cword_start;
    my $cword_end;
    my $word_point;

    my $word = '';
    my $wstart = 0;
    my $skipped = 0;
    my $have_word = 0;

    pos($line) = 0;
    while(pos($line) < length($line)) {
        if($line =~ m{\G ($boring+) }gcx) {
            $word .= $1;
            $have_word = 1;
        } elsif ($line =~ m{\G ($quote)}gcx) {
            my $chr = $1;
            $skipped++ if pos($line) > $point;
            if($line =~ m{\G ([^$chr]*) $chr}gcx) {
                $word .= $1;
                $skipped++ if pos($line) > $point;
            } else {
                $word .= substr($line, pos($line));
                pos($line) = length($line);
            }
            $have_word = 1;
        }

        if ($line =~ m{\G ($space+|$)}gcx) {
            my $wend = pos($line) - length($1);
            if ($have_word) {
                push @words, $word;
                $cword++ unless $wend >= $point;
                if(($wend >= $point) && !defined($word_point)) {
                    $word_point = length($word) - ($wend - $point) + $skipped;
                    $cword_start = $wstart;
                    $cword_end   = $wend;
                }
            }
            # Always reset, so we get $wstart right
            $word = '';
            $wstart = pos($line);
            $skipped = 0;
            $have_word = 0;
        }
    }

    if(length($word)) { die("Internal error, leftover=$word"); }

    unless(defined($word_point)) {
        $word_point = 0;
        $cword_start = $cword_end = $point;
    }

    return (\@words, $cword, $word_point, $cword_start, $cword_end);
}

sub tokenize {
    my $line = shift;

    my ($words, $word, $word_point,
        $word_start, $word_end) = tokenize_with_point($line, 0);
    return $words;
}
