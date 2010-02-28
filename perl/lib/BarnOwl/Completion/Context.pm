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
use Carp qw(croak);
use BarnOwl::Parse qw(tokenize_with_point);

__PACKAGE__->mk_ro_accessors(qw(line point words word word_point
                                word_start word_end));

sub new {
    my $class = shift;
    my $before_point = shift;
    my $after_point = shift;

    my $line  = $before_point . $after_point;
    my $point = length ($before_point);
    my ($words, $word, $word_point,
        $word_start, $word_end) = tokenize_with_point($line, $point);
    push @$words, '' if scalar @$words <= $word;

    my $self = {
        line  => $line,
        point => $point,
        words => $words,
        word  => $word,
        word_point => $word_point,
        word_start => $word_start,
        word_end   => $word_end
       };
    return bless($self, $class);
}

=head2 shift_words N

Returns a new C<Context> object, with the leading C<N> words
stripped. All fields are updated as appopriate. If C<N> > C<<
$self->word >>, C<croak>s with an error message.

=cut

sub shift_words {
    my $self = shift;
    my $n    = shift;

    if($n > $self->word) {
        croak "Context::shift: Unable to shift $n words";
    }

    my $before = substr($self->line, 0, $self->point);
    my $after  = substr($self->line, $self->point);

    return BarnOwl::Completion::Context->new(BarnOwl::skiptokens($before, $n),
                                             $after);
}

1;
