use strict;
use warnings;

# Completers for client state

package BarnOwl::Complete::Client;

use BarnOwl::Completion::Util qw(complete_flags complete_file);
use BarnOwl::Complete::Filter qw(complete_filter_name complete_filter_expr);

my @all_colors = qw(default
                    black
                    blue
                    cyan
                    green
                    magenta
                    red
                    white
                    yellow);

my %show = (
    information => undef,
    colors      => undef,
    commands    => undef,
    command     => \&complete_command,
    errors      => undef,
    filters     => undef,
    filter      => \&complete_filter_name,
    license     => undef,
    keymaps     => undef,
    keymap      => \&complete_keymap,
    quickstart  => undef,
    startup     => undef,
    status      => undef,
    styles      => undef,
    subscriptions       => undef,
    subs        => undef,
    terminal    => undef,
    variables   => undef,
    variable    => \&complete_variable,
    version     => undef,
    view        => undef,
    zpunts      => undef,
   );

sub complete_command { return sort @BarnOwl::all_commands; }
sub complete_color { return @all_colors; }
sub complete_variable    { return @{BarnOwl::all_variables()}; }
sub complete_style       { return @{BarnOwl::all_styles()}; }
sub complete_keymap      { return @{BarnOwl::all_keymaps()}; }

sub complete_help {
    my $ctx = shift;
    if($ctx->word == 1) {
        return complete_command();
    }
}

sub complete_show {
    my $ctx = shift;
    if($ctx->word == 1) {
        return keys %show;
    } elsif ($ctx->word == 2) {
        my $cmd = $show{$ctx->words->[1]};
        if($cmd) {
            return $cmd->($ctx);
        }
    }
}

sub complete_filter {
    my $ctx = shift;
    # Syntax: filter FILTERNAME FLAGS EXPR

    # FILTERNAME
    return complete_filter_name() if $ctx->word == 1;

    # FLAGS
    $ctx = $ctx->shift_words(1); # complete_flags starts at the second word
    return complete_flags($ctx,
        [qw()],
        {
           "-c" => \&complete_color,
           "-b" => \&complete_color,
        },
        # EXPR
        sub {
            my $ctx = shift;
            my $arg = shift;

            # We pass stop_at_nonflag, so we can rewind to the start
            my $idx = $ctx->word - $arg;
            $ctx = $ctx->shift_words($idx);
            return complete_filter_expr($ctx);
        },
        stop_at_nonflag => 1
        );
}

sub complete_filter_no_flags
{
    my $ctx = shift;
    # Syntax: filter FILTERNAME EXPR

    # FILTERNAME
    return complete_filter_name() if $ctx->word == 1;

    $ctx = $ctx->shift_words(2);
    return complete_filter_expr($ctx);
}

sub complete_filter_append {
    my $ctx = shift;
    # Syntax: filterappend FILTERNAME EXPR

    # FILTERNAME
    return complete_filter_name() if $ctx->word == 1;
    return qw(and or) if $ctx->word == 2;
    $ctx = $ctx->shift_words(3);
    return complete_filter_expr($ctx);
}

sub complete_view {
    my $ctx = shift;
    if ($ctx->word == 1) {
        return ("--home", "-d", "-r", "-s", complete_filter_name());
    }
    if ($ctx->words->[1] eq "--home") {
        return;
    }
    if ($ctx->words->[1] eq "-d") {
        $ctx = $ctx->shift_words(2);
        return complete_filter_expr($ctx);
    }
    if ($ctx->words->[1] eq "-s") {
        return complete_style();
    }
    return;
}

sub complete_getvar {
    my $ctx = shift;
    return unless ($ctx->word == 1);
    return complete_variable();
}

sub complete_set {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw(-q)],
        {
        },
         \&complete_set_args
        );
}
sub complete_set_args {
    my $ctx = shift;
    my $arg = shift;
    return if $arg;
    return complete_variable();
}

sub complete_startup {
    my $ctx = shift;
    my $new_ctx = $ctx->shift_words(1);
    return BarnOwl::Completion::get_completions($new_ctx);
}

sub complete_bindkey {
    my $ctx = shift;
    # bindkey KEYMAP KEYSEQ command COMMAND
    #   0      1       2      3        4
    if ($ctx->word == 1) {
        return complete_keymap();
    } elsif ($ctx->word == 2) {
        return;
    } elsif ($ctx->word == 3) {
        return ('command');
    } else {
        my $new_ctx = $ctx->shift_words(4);
        return BarnOwl::Completion::get_completions($new_ctx);
    }
}

sub complete_print {
    my $ctx = shift;
    return unless $ctx->word == 1;
    return complete_variable();
}

sub complete_one_file_arg {
    my $ctx = shift;
    return unless $ctx->word == 1;
    return complete_file($ctx->words->[1]);
}

BarnOwl::Completion::register_completer(help    => \&complete_help);
BarnOwl::Completion::register_completer(filter  => \&complete_filter);
BarnOwl::Completion::register_completer(filteror        => \&complete_filter_no_flags);
BarnOwl::Completion::register_completer(filterand       => \&complete_filter_no_flags);
BarnOwl::Completion::register_completer(filterappend    => \&complete_filter_append);
BarnOwl::Completion::register_completer(view    => \&complete_view);
BarnOwl::Completion::register_completer(show    => \&complete_show);
BarnOwl::Completion::register_completer(getvar  => \&complete_getvar);
BarnOwl::Completion::register_completer(set     => \&complete_set);
BarnOwl::Completion::register_completer(unset   => \&complete_set);
BarnOwl::Completion::register_completer(startup => \&complete_startup);
BarnOwl::Completion::register_completer(bindkey => \&complete_bindkey);
BarnOwl::Completion::register_completer(print   => \&complete_print);

BarnOwl::Completion::register_completer(source      => \&complete_one_file_arg);
BarnOwl::Completion::register_completer('load-subs' => \&complete_one_file_arg);
BarnOwl::Completion::register_completer(loadsubs    => \&complete_one_file_arg);
BarnOwl::Completion::register_completer(loadloginsubs    => \&complete_one_file_arg);
BarnOwl::Completion::register_completer(dump        => \&complete_one_file_arg);

1;
