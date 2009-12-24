use warnings;
use strict;

=head1 NAME

BarnOwl::Completion

=head1 DESCRIPTION

Hooks for tab-completion support in BarnOwl.

=cut

package BarnOwl::Completion;

use BarnOwl::Completion::Context;
use BarnOwl::Editwin qw(save_excursion text_before_point text_after_point
                        point_move replace_region);

use List::Util qw(min first);

our %completers = ();

sub do_complete {
    my $cmd = shift;
    my $before = text_before_point();
    my $after  = text_after_point();
    BarnOwl::debug("Completing: $before-|-$after");
    my $ctx = BarnOwl::Completion::Context->new($before, $after);

    my @words = get_completions($ctx);
    return unless @words;
    my $prefix = common_prefix(map {completion_value($_)} @words);

    if($prefix) {
        insert_completion($ctx, $prefix,
                          scalar @words == 1 && completion_done($words[0]));
    }

    if(scalar @words > 1) {
        show_completions(@words);
    } else {
        BarnOwl::message('');
    }
}

=head1 COMPLETIONS

A COMPLETION is either a simple string, or a reference to an array
containing two or more values.

In the former case, the string use used for both the text to display,
as well as the result of the completion, and is assumed to be a full
completion.

An arrayref completion consists of

    [$display_text, $replacement_value[, $completion_done] ].

$display_text will be printed in the case of ambiguous completions,
$replacement_value will be used to substitute the value in. If there
is only a single completion for a given word, a space will be appended
after the completion iff $completion_done is true (or missing).

=cut

sub completion_text {
    my $c = shift;
    return $c unless ref($c) eq 'ARRAY';
    return $c->[0];
}

sub completion_value {
    my $c = shift;
    return $c unless ref($c) eq 'ARRAY';
    return $c->[1];
}

sub completion_done {
    my $c = shift;
    return 1 if ref($c) ne 'ARRAY' or @$c < 3;
    return $c->[2];
}

sub insert_completion {
    my $ctx = shift;
    my $completion = BarnOwl::quote(completion_value(shift));
    my $done = shift;

    if($done) {
        $completion .= " ";
    }

    save_excursion {
        point_move($ctx->word_start - $ctx->point);
        BarnOwl::Editwin::set_mark();
        point_move($ctx->word_end - $ctx->word_start);
        replace_region($completion);
    };
    if(!length($ctx->words->[$ctx->word])) {
        point_move(length($completion));
    }
}

sub show_completions {
    my @words = @_;
    my $all = BarnOwl::quote(map {completion_text($_)} @words);
    my $width = BarnOwl::getnumcols();
    if (length($all) > $width-1) {
        $all = substr($all, 0, $width-4) . "...";
    }
    BarnOwl::message($all);
}

sub common_prefix {
    my @words = @_;
    my $len   = min(map {length($_)} @words);
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


sub get_completions {
    my $ctx = shift;
    if($ctx->word == 0) {
        return complete_command($ctx->words->[0]);
    } else {
        my $cmd = $ctx->words->[0];
        my $word = $ctx->words->[$ctx->word];
        if(exists($completers{$cmd})) {
            return grep {completion_value($_) =~ m{^\Q$word\E}} $completers{$cmd}->($ctx);
        }
        return;
    }
}

sub complete_command {
    my $cmd = shift;
    $cmd = "" unless defined($cmd);
    return grep {$_ =~ m{^\Q$cmd\E}} @BarnOwl::all_commands;
}

sub register_completer {
    my $cmd = shift;
    my $completer = shift;
    $completers{$cmd} = $completer;
}

sub load_completers {
    opendir(my $dh, BarnOwl::get_data_dir() . "/" . "lib/BarnOwl/Complete/") or return;
    while(my $name = readdir($dh)) {
        next if $name =~ m{^\.};
        next unless $name =~ m{[.]pm$};
        $name =~ s{[.]pm$}{};
        eval "use BarnOwl::Complete::$name";
        if($@) {
            BarnOwl::error("Loading completion module $name:\n$@\n");
        }
    }
}

$BarnOwl::Hooks::startup->add("BarnOwl::Completion::load_completers");

1;
