use warnings;
use strict;

=head1 NAME

BarnOwl::Completion

=head1 DESCRIPTION

Hooks for tab-completion support in BarnOwl.

=cut

package BarnOwl::Completion;

use BarnOwl::Completion::Context;
use BarnOwl::Editwin qw(text_before_point text_after_point);

use List::Util qw(max first);

sub do_complete {
    my $cmd = shift;
    my $before = text_before_point();
    my $after  = text_after_point();
    BarnOwl::debug("Completing: $before-|-$after");
    my $ctx = BarnOwl::Completion::Context->new($before, $after);

    my @words = get_completions($ctx);
    return unless @words;
    my $prefix = common_prefix(@words);

    my $word = $ctx->words->[$ctx->word];

    if($prefix && $prefix ne $word) {
        if(scalar @words == 1) {
            $prefix .= ' ';
        }
        
        BarnOwl::Editwin::insert_text(substr($prefix, length($word)));
    }

    if(scalar @words > 1) {
        show_completions(@words);
    }
}

sub get_completions {
    my $ctx = shift;
    if($ctx->word == 0) {
        return complete_command($ctx->words->[0]);
    } else {
        return;
    }
}

sub complete_command {
    my $cmd = shift;
    return grep {$_ =~ m{^\Q$cmd\E}} @BarnOwl::all_commands;
}

sub show_completions {
    my @words = @_;
    my $all = join(" ", map {BarnOwl::quote($_)} @words);
    my $width = BarnOwl::getnumcols();
    if (length($all) > $width-1) {
        $all = substr($all, 0, $width-4) . "...";
    }
    BarnOwl::message($all);
}

sub common_prefix {
    my @words = @_;
    my $len   = max(map {length($_)} @words);
    my $pfx = '';
    for my $i (1..$len) {
        $pfx = substr($words[0], 0, $i);
        if(first {substr($_, 0, $i) ne $pfx} @words) {
            $pfx = substr($pfx, 0, $i-1);
            last;
        }
    }

    return $pfx;
}

1;
