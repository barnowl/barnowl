use strict;
use warnings;

# Completers for client state

package BarnOwl::Complete::Client;

use BarnOwl::Completion::Util qw(complete_flags);

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
    filters     => undef,
    filter      => \&complete_filter_name,
    license     => undef,
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
sub complete_filter_name { return @{BarnOwl::all_filters()}; }
sub complete_variable {}        # stub

sub complete_filter_args {
    my $ctx = shift;
    my $arg = shift;
    return if $arg;
    return complete_filter_name();
}

sub complete_help {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw()],
        {
        },
        \&complete_command
       );
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
    return complete_flags($ctx,
        [qw()],
        {
           "-c" => \&complete_color,
           "-b" => \&complete_color,
        },
         \&complete_filter_args
        );
}

BarnOwl::Completion::register_completer(help    => \&complete_help);
BarnOwl::Completion::register_completer(filter  => \&complete_filter);
BarnOwl::Completion::register_completer(show    => \&complete_show);

1;
