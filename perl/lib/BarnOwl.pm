use strict;
use warnings;

package BarnOwl;

use base qw(Exporter);
our @EXPORT_OK = qw(command getcurmsg getnumcols getnumlines getidletime
                    register_idle_watcher unregister_idle_watcher
                    zephyr_getsender zephyr_getrealm zephyr_zwrite
                    zephyr_stylestrip zephyr_smartstrip_user zephyr_getsubs
                    queue_message admin_message
                    start_edit
                    start_question start_password start_edit_win
                    get_data_dir get_config_dir popless_text popless_ztext
                    error debug
                    create_style getnumcolors wordwrap
                    message_matches_filter
                    add_dispatch remove_dispatch
                    add_io_dispatch remove_io_dispatch
                    new_command
                    new_variable_int new_variable_bool new_variable_string
                    new_variable_enum
                    quote redisplay);
our %EXPORT_TAGS = (all => \@EXPORT_OK);

BEGIN {
# bootstrap in C bindings and glue
    *owl:: = \*BarnOwl::;
    bootstrap BarnOwl 1.2;
};

use lib(get_data_dir() . "/lib");
use lib(get_config_dir() . "/lib");

use Glib;
use AnyEvent;

use BarnOwl::Hook;
use BarnOwl::Hooks;
use BarnOwl::Logging;
use BarnOwl::Message;
use BarnOwl::Style;
use BarnOwl::Zephyr;
use BarnOwl::Timer;
use BarnOwl::Editwin;
use BarnOwl::Completion;
use BarnOwl::Help;
use BarnOwl::DeferredLogging;

use List::Util qw(max);
use Tie::RefHash;

=head1 NAME

BarnOwl

=head1 DESCRIPTION

The BarnOwl module contains the core of BarnOwl's perl
bindings. Source in this module is also run at startup to bootstrap
BarnOwl by defining things like the default style.

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

Returns the zephyr realm BarnOwl is running in

=head2 zephyr_getsender

Returns the fully-qualified name of the zephyr sender BarnOwl is
running as, e.g. C<nelhage@ATHENA.MIT.EDU>

=head2 zephyr_zwrite COMMAND MESSAGE

Sends a zephyr programmatically. C<COMMAND> should be a C<zwrite>
command line, and C<MESSAGE> is the zephyr body to send.

=cut

sub zephyr_zwrite {
    my ($command, $message) = @_;
    my $ret = BarnOwl::Internal::zephyr_zwrite($command, $message);
    die "Error sending zephyr" unless $ret == 0;
}

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

=head2 start_edit %ARGS

Displays a prompt on the screen and lets the user enter text,
and calls a callback when the editwin is closed.

C<%ARGS> must contain the following keys:

=over 4

=item prompt

The line to display on the screen

=item type

One of:

=over 4

=item edit_win

Displays the prompt on a line of its own and opens the edit_win.

=item question

Displays prompt on the screen and lets the user enter a line of
text.

=item password

Like question, but echoes the user's input as C<*>s when they
input.

=back

=item callback

A Perl subroutine that is called when the user closes the edit_win.
C<CALLBACK> gets called with two parameters: the text the user entered,
and a C<SUCCESS> boolean parameter which is false if the user canceled
the edit_win and true otherwise.

=back

=head2 start_question PROMPT CALLBACK

=head2 start_password PROMPT CALLBACK

=head2 start_edit_win PROMPT CALLBACK

Roughly equivalent to C<start_edit> called with the appropriate parameters.
C<CALLBACK> is only called on success, for compatibility.

These are deprecated wrappers around L<BarnOwl::start_edit>, and should not
be uesd in new code.

=cut

sub start_edit {
    my %args = (@_);
    BarnOwl::Internal::start_edit($args{type}, $args{prompt}, $args{callback});
}

sub start_question {
    my ($prompt, $callback) = @_;
    BarnOwl::start_edit(type => 'question', prompt => $prompt, callback => sub {
            my ($text, $success) = @_;
            $callback->($text) if $success;
        });
}

sub start_password {
    my ($prompt, $callback) = @_;
    BarnOwl::start_edit(type => 'password', prompt => $prompt, callback => sub {
            my ($text, $success) = @_;
            $callback->($text) if $success;
        });
}

sub start_edit_win {
    my ($prompt, $callback) = @_;
    BarnOwl::start_edit(type => 'edit_win', prompt => $prompt, callback => sub {
            my ($text, $success) = @_;
            $callback->($text) if $success;
        });
}

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

=head2 debug STRING

Logs a debugging message to BarnOwl's debug log

=head2 getnumcolors

Returns the number of colors this BarnOwl is capable of displaying

=head2 message_matches_filter MESSAGE FILTER_NAME [QUIET = 0]

Returns 1 if C<FILTER_NAME> is the name of a valid filter, and
C<MESSAGE> matches that filter.  Returns 0 otherwise.  If
C<QUIET> is false, this method displays an error message if
if C<FILTER_NAME> does not name a valid filter.

=head2 add_dispatch FD CALLBACK

Adds a file descriptor to C<BarnOwl>'s internal C<select()>
loop. C<CALLBACK> will be invoked whenever data is available to be
read from C<FD>.

C<add_dispatch> has been deprecated in favor of C<AnyEvent>, and is
now a wrapper for C<add_io_dispatch> called with C<mode> set to
C<'r'>.

=cut

sub add_dispatch {
    my $fd = shift;
    my $cb = shift;
    add_io_dispatch($fd, 'r', $cb);
}

=head2 remove_dispatch FD

Remove a file descriptor previously registered via C<add_dispatch>

C<remove_dispatch> has been deprecated in favor of C<AnyEvent>.

=cut

*remove_dispatch = \&remove_io_dispatch;

=head2 add_io_dispatch FD MODE CB

Adds a file descriptor to C<BarnOwl>'s internal C<select()>
loop. <MODE> can be 'r', 'w', or 'rw'. C<CALLBACK> will be invoked
whenever C<FD> becomes ready, as specified by <MODE>.

Only one callback can be registered per FD. If a new callback is
registered, the old one is removed.

C<add_io_dispatch> has been deprecated in favor of C<AnyEvent>.

=cut

our %_io_dispatches;

sub add_io_dispatch {
    my $fd = shift;
    my $modeStr = shift;
    my $cb = shift;
    my @modes;

    push @modes, 'r' if $modeStr =~ /r/i; # Read
    push @modes, 'w' if $modeStr =~ /w/i; # Write
    if (@modes) {
	BarnOwl::remove_io_dispatch($fd);
	for my $mode (@modes) {
	    push @{$_io_dispatches{$fd}}, AnyEvent->io(fh => $fd,
						       poll => $mode,
						       cb => $cb);
	}
    } else {
        die("Invalid I/O Dispatch mode: $modeStr");
    }
}

=head2 remove_io_dispatch FD

Remove a file descriptor previously registered via C<add_io_dispatch>

C<remove_io_dispatch> has been deprecated in favor of C<AnyEvent>.

=cut

sub remove_io_dispatch {
    my $fd = shift;
    undef $_ foreach @{$_io_dispatches{$fd}};
    delete $_io_dispatches{$fd};
}

=head2 create_style NAME OBJECT

Creates a new BarnOwl style with the given NAME defined by the given
object. The object must have a C<description> method which returns a
string description of the style, and a and C<format_message> method
which accepts a C<BarnOwl::Message> object and returns a string that
is the result of formatting the message for display.

=head2 redisplay

Redraw all of the messages on screen. This is useful if you've just
changed how a style renders messages.

=cut

# perlconfig.c will set this to the value of the -c command-line
# switch, if present.
our $configfile;

our @all_commands;

if(!$configfile) {
    if (-f get_config_dir() . "/init.pl") {
        $configfile = get_config_dir() . "/init.pl";
    } elsif (-f $ENV{HOME} . "/.barnowlconf") {
        $configfile = $ENV{HOME} . "/.barnowlconf";
    } else {
        $configfile = $ENV{HOME}."/.owlconf";
    }
}

# populate global variable space for legacy owlconf files
sub _receive_msg_legacy_wrap {
    my ($m) = @_;
    $m->legacy_populate_global();
    return &BarnOwl::Hooks::_receive_msg($m);
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

=head2 new_variable_enum NAME [{ARGS}]

Add a new owl variable, either an int, a bool, a string, or an enum with the
specified name.

For new_variable_enum, ARGS is required to contain a validsettings key pointing
to an array reference. For all four, it can optionally contain the following
keys:

=over 4

=item default

The default and initial value for the variable

=item summary

A one-line summary of the variable's purpose

=item description

A longer description of the function of the variable

=back

In addition, new_variable_string optionally accepts a string validsettings
parameter, in case people want to set it to "<path>".

=cut

sub new_variable_int {
    my ($name, $args) = @_;
    my $storage = defined($args->{default}) ? $args->{default} : 0;
    BarnOwl::new_variable_full($name, {
            %{$args},
            get_tostring => sub { "$storage" },
            set_fromstring => sub {
                die "Expected integer" unless $_[0] =~ /^-?[0-9]+$/;
                $storage = 0 + $_[0];
            },
            validsettings => "<int>",
            takes_on_off => 0,
        });
}

sub new_variable_bool {
    my ($name, $args) = @_;
    my $storage = defined($args->{default}) ? $args->{default} : 0;
    BarnOwl::new_variable_full($name, {
            %{$args},
            get_tostring => sub { $storage ? "on" : "off" },
            set_fromstring => sub {
                die "Valid settings are on/off" unless $_[0] eq "on" || $_[0] eq "off";
                $storage = $_[0] eq "on";
            },
            validsettings => "on,off",
            takes_on_off => 1,
        });
}

sub new_variable_string {
    my ($name, $args) = @_;
    my $storage = defined($args->{default}) ? $args->{default} : "";
    BarnOwl::new_variable_full($name, {
            # Allow people to override this one if they /reaaally/ want to for
            # some reason. Though we still reserve the right to interpret this
            # value in interesting ways for tab-completion purposes.
            validsettings => "<string>",
            %{$args},
            get_tostring => sub { $storage },
            set_fromstring => sub { $storage = $_[0]; },
            takes_on_off => 0,
        });
}

sub new_variable_enum {
    my ($name, $args) = @_;

    # Gather the valid settings.
    die "validsettings is required" unless defined($args->{validsettings});
    my %valid;
    map { $valid{$_} = 1 } @{$args->{validsettings}};

    my $storage = (defined($args->{default}) ?
                   $args->{default} :
                   $args->{validsettings}->[0]);
    BarnOwl::new_variable_full($name, {
            %{$args},
            get_tostring => sub { $storage },
            set_fromstring => sub {
                die "Invalid input" unless $valid{$_[0]};
                $storage = $_[0];
            },
            validsettings => join(",", @{$args->{validsettings}})
        });
}

=head2 new_variable_full NAME {ARGS}

Create a variable, in full generality. The keyword arguments have types below:

 get_tostring : ()  -> string
 set_fromstring : string -> int
 -- optional --
 summary : string
 description : string
 validsettings : string
 takes_on_off : int

The get/set functions are required. Note that the caller manages storage for the
variable. get_tostring/set_fromstring both convert AND store the value.
set_fromstring dies on failure.

If the variable takes parameters 'on' and 'off' (i.e. is boolean-looking), set
takes_on_off to 1. This makes :set VAR and :unset VAR work. set_fromstring will
be called with those arguments.

=cut

sub new_variable_full {
    my $name = shift;
    my $args = shift || {};
    my %args = (
        summary => "",
        description => "",
        takes_on_off => 0,
        validsettings => "<string>",
        %{$args});

    die "get_tostring required" unless $args{get_tostring};
    die "set_fromstring required" unless $args{set_fromstring};

    # Strip off the bogus dummy argument. Aargh perl-Glib.
    my $get_tostring_fn = sub { $args{get_tostring}->() };
    my $set_fromstring_fn = sub {
      my ($dummy, $val) = @_;
      # Translate from user-supplied die-on-failure callback to expected
      # non-zero on error. Less of a nuisance than interacting with ERRSV.
      eval { $args{set_fromstring}->($val) };
      # TODO: Consider changing B::I::new_variable to expect string|NULL with
      # string as the error message. That can then be translated to a GError in
      # owl_variable_set_fromstring. For now the string is ignored.
      return ($@ ? -1 : 0);
    };

    BarnOwl::Internal::new_variable($name, $args{summary}, $args{description}, $args{validsettings},
                                    $args{takes_on_off}, $get_tostring_fn, $set_fromstring_fn, undef);
}

=head2 quote LIST

Quotes each of the strings in LIST and returns a string that will be
correctly decoded to LIST by the BarnOwl command parser.  For example:

    quote('zwrite', 'andersk', '-m', 'Hello, world!')
    # returns "zwrite andersk -m 'Hello, world!'"

=cut

sub quote {
    my @quoted;
    for my $str (@_) {
        if ($str eq '') {
            push @quoted, "''";
        } elsif ($str !~ /['" \n\t]/) {
            push @quoted, "$str";
        } elsif ($str !~ /'/) {
            push @quoted, "'$str'";
        } else {
            (my $qstr = $str) =~ s/"/"'"'"/g;
            push @quoted, '"' . $qstr . '"';
        }
    }
    return join(' ', @quoted);
}

=head2 Modify filters by appending text

=cut

sub register_builtin_commands {
    # Filter modification
    BarnOwl::new_command("filterappend",
                         sub { filter_append_helper('appending', '', @_); },
                       {
                           summary => "append '<text>' to filter",
                           usage => "filterappend <filter> <text>",
                       });

    BarnOwl::new_command("filterand",
                         sub { filter_append_helper('and-ing', 'and', @_); },
                       {
                           summary => "append 'and <text>' to filter",
                           usage => "filterand <filter> <text>",
                       });

    BarnOwl::new_command("filteror",
                         sub { filter_append_helper('or-ing', 'or', @_); },
                       {
                           summary => "append 'or <text>' to filter",
                           usage => "filteror <filter> <text>",
                       });

    # Date formatting
    BarnOwl::new_command("showdate",
                         sub { BarnOwl::time_format('showdate', '%Y-%m-%d %H:%M'); },
                       {
                           summary => "Show date in timestamps for supporting styles.",
                           usage => "showdate",
                       });

    BarnOwl::new_command("hidedate",
                         sub { BarnOwl::time_format('hidedate', '%H:%M'); },
                       {
                           summary => "Don't show date in timestamps for supporting styles.",
                           usage => "hidedate",
                       });

    BarnOwl::new_command("timeformat",
                         \&BarnOwl::time_format,
                       {
                           summary => "Set the format for timestamps and re-display messages",
                           usage => "timeformat <format>",
                       });

    # Receive window scrolling
    BarnOwl::new_command("recv:shiftleft",
                        \&BarnOwl::recv_shift_left,
                        {
                            summary => "scrolls receive window to the left",
                            usage => "recv:shiftleft [<amount>]",
                            description => <<END_DESCR
By default, scroll left by 10 columns. Passing no arguments or 0 activates this default behavior.
Otherwise, scroll by the number of columns specified as the argument.
END_DESCR
                        });

    BarnOwl::new_command("recv:shiftright",
                        \&BarnOwl::recv_shift_right,
                        {
                            summary => "scrolls receive window to the right",
                            usage => "recv:shiftright [<amount>]",
                            description => <<END_DESCR
By default, scroll right by 10 columns. Passing no arguments or 0 activates this default behavior.
Otherwise, scroll by the number of columns specified as the argument.
END_DESCR
                        });

}

$BarnOwl::Hooks::startup->add("BarnOwl::register_builtin_commands");

=head3 filter_append_helper ACTION SEP FUNC FILTER APPEND_TEXT

Helper to append to filters.

=cut

sub filter_append_helper
{
    my $action = shift;
    my $sep = shift;
    my $func = shift;
    my $filter = shift;
    my @append = @_;
    my $oldfilter = BarnOwl::getfilter($filter);
    chomp $oldfilter;
    my $newfilter = "$oldfilter $sep " . quote(@append);
    my $msgtext = "To filter " . quote($filter) . " $action\n" . quote(@append) . "\nto get\n$newfilter";
    if (BarnOwl::getvar('showfilterchange') eq 'on') {
        BarnOwl::admin_message("Filter", $msgtext);
    }
    set_filter($filter, $newfilter);
    return;
}
BarnOwl::new_variable_bool("showfilterchange",
                           { default => 1,
                             summary => 'Show modifications to filters by filterappend and friends.'});

sub set_filter
{
    my $filtername = shift;
    my $filtertext = shift;
    my $cmd = 'filter ' . BarnOwl::quote($filtername) . ' ' . $filtertext;
    BarnOwl::command($cmd);
}

=head3 time_format FORMAT

Set the format for displaying times (variable timeformat) and redisplay
messages.

=cut

my $timeformat = '%H:%M';

sub time_format
{
    my $function = shift;
    my $format = shift;
    if(!$format)
    {
        return $timeformat;
    }
    if(shift)
    {
        return "Wrong number of arguments for command";
    }
    $timeformat = $format;
    redisplay();
}

=head3 Receive window scrolling

Permit scrolling the receive window left or right by arbitrary
amounts (with a default of 10 characters).

=cut

sub recv_shift_left
{
    my $func = shift;
    my $delta = shift;
    $delta = 10 unless defined($delta) && int($delta) > 0;
    my $shift = BarnOwl::recv_getshift();
    if($shift > 0) {
        BarnOwl::recv_setshift(max(0, $shift-$delta));
    } else {
        return "Already full left";
    }
}

sub recv_shift_right
{
    my $func = shift;
    my $delta = shift;
    $delta = 10 unless defined($delta) && int($delta) > 0;
    my $shift = BarnOwl::recv_getshift();
    BarnOwl::recv_setshift($shift+$delta);
}

=head3 default_zephyr_signature

Compute the default zephyr signature.

=cut

sub default_zephyr_signature
{
  my $zsig = getvar('zsig');
  if (!defined($zsig) || $zsig eq '') {
      my $zsigproc = getvar('zsigproc');
      if (defined($zsigproc) && $zsigproc ne '') {
	  $zsig = `$zsigproc`;
      } elsif (!defined($zsig = get_zephyr_variable('zwrite-signature'))) {
	  $zsig = ((getpwuid($<))[6]);
	  $zsig =~ s/,.*//;
      }
  }
  chomp($zsig);
  return $zsig;
}

=head3 random_zephyr_signature

Retrieve a random line from ~/.zsigs (except those beginning with '#') 
and use it as the zephyr signature.

=cut

sub random_zephyr_signature
{
    my $zsigfile = "$ENV{'HOME'}/.zsigs";
    open my $file, '<', $zsigfile or die "Error opening file $zsigfile: $!";
    my @lines = grep !(/^#/ || /^\s*$/), <$file>;
    close $file;
    return '' if !@lines;
    my $zsig = "$lines[int(rand(scalar @lines))]";
    chomp $zsig;
    return $zsig;
}

=head3 register_idle_watcher %ARGS

Call a callback whenever the amount of time the user becomes idle or comes
back from being idle.

You must include the following parameters:

=over 4

=item name

The name given to the idle watcher

=item after

How long the user must be idle, in seconds, before the callback is called.
If the value is too small, you may have spurious or inaccurate calls.
(The current lower limit is about 1 second.)

=item callback

The Perl subroutine that gets called when the user has been idle for C<AFTER>
seconds, or comes back from being idle.  The subroutine is passed one parameter,
which is true if the user is currently idle, and false otherwise.

=back

This method returns a unique identifier which may be passed to
L<BarnOwl::unregister_idle_watcher>.

=cut

=head3 unregister_idle_watcher UNIQUE_ID [...]

Removed and returns the idle watcher specified by C<UNIQUE_ID>.
You may specify multiple unique ids.

=cut

my %idle_watchers;
tie %idle_watchers, 'Tie::RefHash';

$BarnOwl::Hooks::wakeup->add(sub {
        foreach my $idle_watcher (values %idle_watchers) {
            _wakeup_idle_watcher($idle_watcher);
        }
    });

sub _wakeup_idle_watcher {
    my ($idle_watcher, $offset) = @_;
    $offset = 0 unless defined $offset;
    # go unidle
    $idle_watcher->{idle_timer}->stop if $idle_watcher->{idle_timer};
    undef $idle_watcher->{idle_timer};
    $idle_watcher->{callback}->(0) if $idle_watcher->{is_idle};
    $idle_watcher->{is_idle} = 0;

    # queue going idle
    $idle_watcher->{idle_timer} = BarnOwl::Timer->new({
        name  => $idle_watcher->{name},
        after => $idle_watcher->{after} - $offset,
        cb    => sub {
            $idle_watcher->{is_idle} = 1;
            $idle_watcher->{callback}->(1);
        }
    });
}

sub register_idle_watcher {
    my %args = (@_);
    $idle_watchers{\%args} = \%args;
    _wakeup_idle_watcher(\%args, BarnOwl::getidletime); # make sure to queue up the idle/unidle events from this idle watcher
    return \%args;
}

sub unregister_idle_watcher {
    my ($id) = @_;
    $idle_watchers{$id}->{idle_timer}->stop if $idle_watchers{$id}->{idle_timer};
    return delete $idle_watchers{$id};
}

# Stub for owl::startup / BarnOwl::startup, so it isn't bound to the
# startup command. This may be redefined in a user's configfile.
sub startup
{
}

1;
