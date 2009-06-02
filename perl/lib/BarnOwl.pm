use strict;
use warnings;

package BarnOwl;

BEGIN {
# bootstrap in C bindings and glue
    *owl:: = \*BarnOwl::;
    bootstrap BarnOwl 1.2;
};

use lib(get_data_dir() . "/lib");
use lib(get_config_dir() . "/lib");

use BarnOwl::Hook;
use BarnOwl::Hooks;
use BarnOwl::Message;
use BarnOwl::Style;
use BarnOwl::Timer;

=head1 NAME

BarnOwl

=head1 DESCRIPTION

The BarnOwl module contains the core of BarnOwl's perl
bindings. Source in this module is also run at startup to bootstrap
barnowl by defining things like the default style.

=for NOTE
These following functions are defined in perlglue.xs. Keep the
documentation here in sync with the user-visible commands defined
there!

=head2 command STRING

Executes a BarnOwl command in the same manner as if the user had
executed it at the BarnOwl command prompt. If the command returns a
value, return it as a string, otherwise return undef.

=head2 getcurmsg

Returns the current message as a C<BarnOwl::Message> subclass, or
undef if there is no message selected

=head2 getnumcols

Returns the width of the display window BarnOwl is currently using

=head2 getidletime

Returns the length of time since the user has pressed a key, in
seconds.

=head2 zephyr_getrealm

Returns the zephyr realm barnowl is running in

=head2 zephyr_getsender

Returns the fully-qualified name of the zephyr sender barnowl is
running as, e.g. C<nelhage@ATHENA.MIT.EDU>

=head2 zephyr_zwrite COMMAND MESSAGE

Sends a zephyr programmatically. C<COMMAND> should be a C<zwrite>
command line, and C<MESSAGE> is the zephyr body to send.

=head2 ztext_stylestrip STRING

Strips zephyr formatting from a string and returns the result

=head2 zephyr_getsubs

Returns the list of subscription triples <class,instance,recipient>,
separated by newlines.

=head2 queue_message MESSAGE

Enqueue a message in the BarnOwl message list, logging it and
processing it appropriately. C<MESSAGE> should be an instance of
BarnOwl::Message or a subclass.

=head2 admin_message HEADER BODY

Display a BarnOwl B<Admin> message, with the given header and body.

=head2 start_question PROMPT CALLBACK

Displays C<PROMPT> on the screen and lets the user enter a line of
text, and calls C<CALLBACK>, which must be a perl subroutine
reference, with the text the user entered

=head2 start_password PROMPT CALLBACK

Like C<start_question>, but echoes the user's input as C<*>s when they
input.

=head2 start_edit_win PROMPT CALLBACK

Like C<start_question>, but displays C<PROMPT> on a line of its own
and opens the editwin. If the user cancels the edit win, C<CALLBACK>
is not invoked.

=head2 get_data_dir

Returns the BarnOwl system data directory, where system libraries and
modules are stored

=head2 get_config_dir

Returns the BarnOwl user configuration directory, where user modules
and configuration are stored (by default, C<$HOME/.owl>)

=head2 popless_text TEXT

Show a popup window containing the given C<TEXT>

=head2 popless_ztext TEXT

Show a popup window containing the provided zephyr-formatted C<TEXT>

=head2 error STRING

Reports an error and log it in `show errors'. Note that in any
callback or hook called in perl code from BarnOwl, a C<die> will be
caught and passed to C<error>.

=head2 getnumcolors

Returns the number of colors this BarnOwl is capable of displaying

=head2 add_dispatch FD CALLBACK

Adds a file descriptor to C<BarnOwl>'s internal C<select()>
loop. C<CALLBACK> will be invoked whenever data is available to be
read from C<FD>.

=head2 remove_dispatch FD

Remove a file descriptor previously registered via C<add_dispatch>

=head2 create_style NAME OBJECT

Creates a new barnowl style with the given NAME defined by the given
object. The object must have a C<description> method which returns a
string description of the style, and a and C<format_message> method
which accepts a C<BarnOwl::Message> object and returns a string that
is the result of formatting the message for display.

=cut

# perlconfig.c will set this to the value of the -c command-line
# switch, if present.
our $configfile;

if(!$configfile && -f $ENV{HOME} . "/.barnowlconf") {
    $configfile = $ENV{HOME} . "/.barnowlconf";
}
$configfile ||= $ENV{HOME}."/.owlconf";

# populate global variable space for legacy owlconf files
sub _receive_msg_legacy_wrap {
    my ($m) = @_;
    $m->legacy_populate_global();
    return &BarnOwl::Hooks::_receive_msg($m);
}

=head2 AUTOLOAD

BarnOwl.pm has a C<AUTOLOAD> method that translates unused names in
the BarnOwl:: namespace to a call to BarnOwl::command() with that
command. Underscores are also translated to C<->s, so you can do
e.g. C<BarnOwl::start_command()> and it will be translated into
C<start-command>.

So, if you're looking for functionality that you can't find in the
perl interface, check C<:show commands> or C<commands.c> in the
BarnOwl source tree -- there's a good chance it exists as a BarnOwl
command.

=head3 BUGS

There are horrible quoting issues here. The AUTOLOAD simple joins your
commands with spaces and passes them unmodified to C<::command>

=cut

# make BarnOwl::<command>("foo") be aliases to BarnOwl::command("<command> foo");
sub AUTOLOAD {
    our $AUTOLOAD;
    my $called = $AUTOLOAD;
    $called =~ s/.*:://;
    $called =~ s/_/-/g;
    return &BarnOwl::command("$called ".join(" ",@_));
}

=head2 new_command NAME FUNC [{ARGS}]

Add a new owl command. When owl executes the command NAME, FUNC will
be called with the arguments passed to the command, with NAME as the
first argument.

ARGS should be a hashref containing any or all of C<summary>,
C<usage>, or C<description> keys:

=over 4

=item summary

A one-line summary of the purpose of the command

=item usage

A one-line usage synopsis, showing available options and syntax

=item description

A longer description of the syntax and semantics of the command,
explaining usage and options

=back

=cut

sub new_command {
    my $name = shift;
    my $func = shift;
    my $args = shift || {};
    my %args = (
        summary     => "",
        usage       => "",
        description => "",
        %{$args}
    );

    BarnOwl::Internal::new_command($name, $func, $args{summary}, $args{usage}, $args{description});
}

=head2 new_variable_int NAME [{ARGS}]

=head2 new_variable_bool NAME [{ARGS}]

=head2 new_variable_string NAME [{ARGS}]

Add a new owl variable, either an int, a bool, or a string, with the
specified name.

ARGS can optionally contain the following keys:

=over 4

=item default

The default and initial value for the variable

=item summary

A one-line summary of the variable's purpose

=item description

A longer description of the function of the variable

=back

=cut

sub new_variable_int {
    unshift @_, \&BarnOwl::Internal::new_variable_int, 0;
    goto \&_new_variable;
}

sub new_variable_bool {
    unshift @_, \&BarnOwl::Internal::new_variable_bool, 0;
    goto \&_new_variable;
}

sub new_variable_string {
    unshift @_, \&BarnOwl::Internal::new_variable_string, "";
    goto \&_new_variable;
}

sub _new_variable {
    my $func = shift;
    my $default_default = shift;
    my $name = shift;
    my $args = shift || {};
    my %args = (
        summary     => "",
        description => "",
        default     => $default_default,
        %{$args});
    $func->($name, $args{default}, $args{summary}, $args{description});
}

=head2 quote STRING

Return a version of STRING fully quoted to survive processing by
BarnOwl's command parser.

=cut

sub quote {
    my $str = shift;
    return "''" if $str eq '';
    if ($str !~ /['" ]/) {
        return "$str";
    }
    if ($str !~ /'/) {
        return "'$str'";
    }
    $str =~ s/"/"'"'"/g;
    return '"' . $str . '"';
}


1;
