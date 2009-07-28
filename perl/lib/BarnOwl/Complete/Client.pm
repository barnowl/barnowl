use strict;
use warnings;

# Completers for client state

package BarnOwl::Complete::Client;

use BarnOwl::Completion::Util qw(complete_flags);

sub complete_command { return sort @BarnOwl::all_commands; }

sub complete_help {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw()],
        {
        },
        \&complete_command
       );
}

BarnOwl::Completion::register_completer(help      => \&complete_help);

1;
