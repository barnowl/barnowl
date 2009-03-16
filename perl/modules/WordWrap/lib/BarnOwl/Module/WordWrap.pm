use warnings;
use strict;

=head1 NAME

BarnOwl::Module::WordWrap

=head1 DESCRIPTION

An extension of the default owl style that wordwraps displayed
messages using L<Text::Autoformat>

=cut

package BarnOwl::Module::WordWrap;
use base qw(BarnOwl::Style::Default);

eval "use Text::Autoformat";

if($@) {
    BarnOwl::error("Text::Autoformat not found; Not loading wordwrap style");
} else {
    BarnOwl::create_style("wordwrap", "BarnOwl::Module::WordWrap");
}

sub indent_body {
    my $self = shift;
    my $m = shift;
    my $body = $m->body;

    my $left = 5;

    $body = autoformat($body, {left => $left,
                               right => BarnOwl::getnumcols() - $left - 1,
                               all => 1, renumber => 0, list => 0});
    $body =~ s/\n$//;
    return $body;
}

sub description {
    "A word-wrapping variant of the default style."
}

1;
