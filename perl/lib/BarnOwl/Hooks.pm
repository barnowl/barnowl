use strict;
use warnings;

package BarnOwl::Hooks;

use Carp;
use List::Util qw(first);

=head1 BarnOwl::Hooks

=head1 DESCRIPTION

C<BarnOwl::Hooks> exports a set of C<BarnOwl::Hook> objects made
available by BarnOwl internally.

=head2 USAGE

Modules wishing to respond to events in BarnOwl should register
functions with these hooks.

=head2 EXPORTS

None by default. Either import the hooks you need explicitly, or refer
to them with fully-qualified names. Available hooks are:

=over 4

=item $startup

Called on BarnOwl startup, and whenever modules are
reloaded. Functions registered with the C<$startup> hook get a true
argument if this is a reload, and false if this is a true startup

=item $shutdown

Called before BarnOwl shutdown

=item $receiveMessage

Called with a C<BarnOwl::Message> object every time BarnOwl receives a
new incoming message.

=item $newMessage

Called with a C<BarnOwl::Message> object every time BarnOwl appends
I<any> new message to the message list.

=item $mainLoop

Called on every pass through the C<BarnOwl> main loop. This is
guaranteed to be called at least once/sec and may be called more
frequently.

=item $getBuddyList

Called to display buddy lists for all protocol handlers. The result
from every function registered with this hook will be appended and
displayed in a popup window, with zephyr formatting parsed.

=item $getQuickstart

Called by :show quickstart to display 2-5 lines of help on how to
start using the protocol. The result from every function registered
with this hook will be appended and displayed in an admin message,
with zephyr formatting parsed. The format should be
"@b(Protocol:)\nSome text.\nMore text.\n"

=back

=cut

use Exporter;

our @EXPORT_OK = qw($startup $shutdown
                    $receiveMessage $newMessage
                    $mainLoop $getBuddyList
                    $getQuickstart);

our %EXPORT_TAGS = (all => [@EXPORT_OK]);

use BarnOwl::MainLoopCompatHook;

our $startup = BarnOwl::Hook->new;
our $shutdown = BarnOwl::Hook->new;
our $receiveMessage = BarnOwl::Hook->new;
our $newMessage = BarnOwl::Hook->new;
our $mainLoop = BarnOwl::MainLoopCompatHook->new;
our $getBuddyList = BarnOwl::Hook->new;
our $getQuickstart = BarnOwl::Hook->new;

# Internal startup/shutdown routines called by the C code

sub _load_perl_commands {
    # Load builtin perl commands
    BarnOwl::new_command(style => \&BarnOwl::Style::style_command,
                       {
                           summary => "creates a new style",
                           usage   => "style <name> perl <function_name>",
                           description =>
                           "A style named <name> will be created that will\n" .
                           "format messages using the perl function <function_name>.\n\n" .
                           "SEE ALSO: show styles, view -s, filter -s\n\n" .
                           "DEPRECATED in favor of BarnOwl::create_style(NAME, OBJECT)",
                          });
    BarnOwl::new_command('edit:complete' => \&BarnOwl::Completion::do_complete,
                       {
                           summary     => "Complete the word at point",
                           usage       => "complete",
                           description =>
                           "This is the function responsible for tab-completion."
                       });
    BarnOwl::new_command('edit:help' => \&BarnOwl::Help::show_help,
                       {
                           summary     => "Display help for the current command",
                           usage       => "help",
                           description =>
                           "Opens the help information on the current command.\n" .
                           "Returns to the previous editing context afterwards.\n\n" .
                           "SEE ALSO: help"
                         });
    BarnOwl::bindkey(editline => TAB => command => 'edit:complete');
    BarnOwl::bindkey(editline => 'M-h' => command => 'edit:help');
}

sub _load_owlconf {
    # load the config  file
    if ( -r $BarnOwl::configfile ) {
        undef $@;
        package main;
        do $BarnOwl::configfile;
        if($@) {
            BarnOwl::error("In startup: $@\n");
            return;
        }
        package BarnOwl;
        if(*BarnOwl::format_msg{CODE}) {
            # if the config defines a legacy formatting function, add 'perl' as a style
            BarnOwl::create_style("perl", BarnOwl::Style::Legacy->new(
                "BarnOwl::format_msg",
                "User-defined perl style that calls BarnOwl::format_msg"
                . " with legacy global variable support",
                1));
             BarnOwl::set("-q default_style perl");
        }
    }
}

# These are the internal hooks called by the BarnOwl C code, which
# take care of dispatching to the appropriate perl hooks, and deal
# with compatibility by calling the old, fixed-name hooks.

sub _startup {
    _load_perl_commands();
    _load_owlconf();

    if(eval {require BarnOwl::ModuleLoader}) {
        eval {
            BarnOwl::ModuleLoader->load_all;
        };
        BarnOwl::error("$@") if $@;

    } else {
        BarnOwl::error("Can't load BarnOwl::ModuleLoader, loadable module support disabled:\n$@");
    }

    $mainLoop->check_owlconf();
    $startup->run(0);
    BarnOwl::startup() if *BarnOwl::startup{CODE};
}

sub _shutdown {
    $shutdown->run;

    BarnOwl::shutdown() if *BarnOwl::shutdown{CODE};
}

sub _receive_msg {
    my $m = shift;

    $receiveMessage->run($m);

    BarnOwl::receive_msg($m) if *BarnOwl::receive_msg{CODE};
}

sub _new_msg {
    my $m = shift;

    $newMessage->run($m);

    BarnOwl::new_msg($m) if *BarnOwl::new_msg{CODE};
}

sub _get_blist {
    my @results = grep defined, $getBuddyList->run;
    s/^\s+|\s+$//sg for (@results);
    return join("\n", grep {length($_)} @results);
}

sub _get_quickstart {
    return join("\n", $getQuickstart->run);
}

sub _new_command {
    my $command = shift;
    (my $symbol = $command) =~ s/-/_/g;
    my $package = "BarnOwl";


    if(!contains(\@BarnOwl::all_commands, $command)) {
        push @BarnOwl::all_commands, $command;
    }

    if($symbol =~ m{^edit:(.+)$}) {
        $symbol = $1;
        $package = "BarnOwl::Editwin";
    } else {
        $symbol =~ s/:/_/;
    }
    {
        no strict 'refs';
        if(defined(*{"${package}::${symbol}"}{CODE})) {
            return;
        }
        *{"${package}::${symbol}"} = sub {
            if(@_ == 1 && $_[0] =~ m{\s}) {
                carp "DEPRECATED: ${package}::${symbol}: Tokenizing argument on ' '.\n"
                . "In future versions, the argument list will be passed to\n"
                . "'$command' directly. Tokenize yourself, or use BarnOwl::command()\n";
                BarnOwl::command("$command $_[0]");
            } else {
                BarnOwl::command($command, @_);
            }
          };
        if(defined(*{"${package}::EXPORT_OK"}{ARRAY})
          && !contains(*{"${package}::EXPORT_OK"}{ARRAY}, $symbol)) {
            push @{*{"${package}::EXPORT_OK"}{ARRAY}}, $symbol;
        }
    }
}

sub contains {
    my $list = shift;
    my $what = shift;
    return defined(first {$_ eq $what} @$list);
}

1;
