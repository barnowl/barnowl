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

sub complete_command { return sort @BarnOwl::all_commands; }
sub complete_color { return @all_colors; }

sub complete_help {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw()],
        {
        },
        \&complete_command
       );
}

sub complete_filter {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw()],
        {
           "-c" => \&complete_color,
           "-b" => \&complete_color,
        },
        undef # TODO: make this get list of filters for the first argument
              # then maybe some magic like completing keywords instance and other shiny
        );
}

BarnOwl::Completion::register_completer(help    => \&complete_help);
BarnOwl::Completion::register_completer(filter  => \&complete_filter);

1;
