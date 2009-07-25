use warnings;
use strict;

=head1 NAME

BarnOwl::Completion::Context

=head1 DESCRIPTION

BarnOwl::Completion::Context is the context that is passed to a
completion function by BarnOwl. It contains information on the text
being completed.

=head1 METHODS

=head2 line

The entire command-line currently being completed.

=head2 point

The index of the cursor in C<line>, in the range C<[0,len($line)]>.

=head2 words

The current command-line, tokenized according to BarnOwl's
tokenization rules, as an array reference.

=head2 word

The current word the point is sitting in, as an index into C<words>.

=head2 word_point

The index of the point within C<$words->[$word]>.

=cut

package BarnOwl::Completion::Context;

use base qw(Class::Accessor::Fast);

__PACKAGE__->mk_ro_accessors(qw(line point words word word_point));

sub new {
    my $class = shift;
    my $before_point = shift;
    my $after_point = shift;

    my $line  = $before_point . $after_point;
    my $point = length ($before_point);
    my ($words, $word, $word_point) = tokenize($line, $point);

    my $self = {
        line  => $line,
        point => $point,
        words => $words,
        word  => $word,
        word_point => $word_point
       };
    return bless($self, $class);
}

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

sub tokenize {
    my $line = shift;
    my $point = shift;

    my @words = ();
    my $cword = 0;
    my $word_point;

    my $word = '';
    my $skipped = 0;

    pos($line) = 0;
    while(pos($line) < length($line)) {
        if($line =~ m{\G ($boring+) }gcx) {
            $word .= $1;
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
        }

        if ($line =~ m{\G ($space+|$)}gcx) {
            my $wend = pos($line) - length($1);
            push @words, $word;
            $cword++ unless $wend >= $point;
            if ($wend >= $point && !defined($word_point)) {
                $word_point = length($word) - ($wend - $point) + $skipped;
            }
            $word = '';
            $skipped = 0;
        }
    }

    if(length($word)) { die("Internal error, leftover=$word"); }

    $word_point = 0 unless defined($word_point);

    return (\@words, $cword, $word_point);
}

1;
