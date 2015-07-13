use strict;
use warnings;

package BarnOwl::Module::IRC;

=head1 NAME

BarnOwl::Module::IRC

=head1 DESCRIPTION

This module implements IRC support for BarnOwl.

=cut

use BarnOwl;
use BarnOwl::Hooks;
use BarnOwl::Message::IRC;
use BarnOwl::Module::IRC::Connection qw(is_private);
use BarnOwl::Module::IRC::Completion;

use AnyEvent::IRC;
use Encode;
use File::Spec;
use Getopt::Long;
use Text::Wrap;

our $VERSION = 0.02;

our $IRC_SUBS_FILENAME = "ircchannels";

our $irc;

# Hash alias -> BarnOwl::Module::IRC::Connection object
our %ircnets;

sub startup {
    BarnOwl::new_variable_string('irc:nick', {
        default     => $ENV{USER},
        summary     => 'The default IRC nickname',
        description => 'By default, irc-connect will use this nick '  .
        'when connecting to a new server. See :help irc-connect for ' .
        'more information.'
       });

    BarnOwl::new_variable_string('irc:user', {
        default => $ENV{USER},
        summary => 'The IRC "username" field'
       });
        BarnOwl::new_variable_string('irc:name', {
        default => "",
        summary     => 'A short name field for IRC',
        description => 'A short (maybe 60 or so chars) piece of text, ' .
        'originally intended to display your real name, which people '  .
        'often use for pithy quotes and URLs.'
       });

    BarnOwl::new_variable_bool('irc:spew', {
        default     => 0,
        summary     => 'Show unhandled IRC events',
        description => 'If set, display all unrecognized IRC events as ' .
        'admin messages. Intended for debugging and development use only.'
       });

    BarnOwl::new_variable_string('irc:skip', {
        default     => 'welcome yourhost created ' .
        'luserclient luserme luserop luserchannels',
        summary     => 'Skip messages of these types',
        description => 'If set, each (space-separated) message type ' .
        'provided will be hidden and ignored if received.'
       });

    BarnOwl::new_variable_int('irc:max-message-length', {
        default     => 450,
        summary     => 'Split messages to at most this many characters.' .
                       "If non-positive, don't split messages",
        description => 'If set to a positive number, any paragraph in an ' .
                       'IRC message will be split after this many characters.'
       });

    register_commands();
    BarnOwl::filter(qw{irc type ^IRC$ or ( type ^admin$ and adminheader ^IRC$ )});
}

sub shutdown {
    for my $conn (values %ircnets) {
        $conn->conn->disconnect('Quitting');
    }
}

sub quickstart {
    return <<'END_QUICKSTART';
@b[IRC:]
Use ':irc-connect @b[server]' to connect to an IRC server, and
':irc-join @b[#channel]' to join a channel. ':irc-msg @b[#channel]
@b[message]' sends a message to a channel.
END_QUICKSTART
}

sub buddylist {
    my $list = "";

    for my $net (sort keys %ircnets) {
        my $conn = $ircnets{$net};
        my ($nick, $server) = ($conn->nick, $conn->server);
        $list .= BarnOwl::Style::boldify("IRC channels for $net ($nick\@$server)");
        $list .= "\n";

        for my $chan (keys %{$conn->conn->{channel_list}}) {
            $list .= "  $chan\n";
        }
    }

    return $list;
}

sub skip_msg {
    my $class = shift;
    my $type = lc shift;
    my $skip = lc BarnOwl::getvar('irc:skip');
    return grep {$_ eq $type} split ' ', $skip;
}

=head2 mk_irc_command SUB FLAGS

Return a subroutine that can be bound as a an IRC command. The
subroutine will be called with arguments (COMMAND-NAME,
IRC-CONNECTION, [CHANNEL], ARGV...).

C<IRC-CONNECTION> and C<CHANNEL> will be inferred from arguments to
the command and the current message if appropriate.

The bitwise C<or> of zero or more C<FLAGS> can be passed in as a
second argument to alter the behavior of the returned commands:

=over 4

=item C<CHANNEL_ARG>

This command accepts the name of a channel. Pass in the C<CHANNEL>
argument listed above, and die if no channel argument can be found.

=item C<CHANNEL_OR_USER>

Pass the channel argument, but accept it if it's a username (e.g.
has no hash).  Only relevant with C<CHANNEL_ARG>.

=item C<CHANNEL_OPTIONAL>

Pass the channel argument, but don't die if not present. Only relevant
with C<CHANNEL_ARG>.

=item C<ALLOW_DISCONNECTED>

C<IRC-CONNECTION> may be a disconnected connection object that is
currently pending a reconnect.

=back

=cut

use constant CHANNEL_ARG        => 1;
use constant CHANNEL_OPTIONAL   => 2;
use constant CHANNEL_OR_USER    => 4;

use constant ALLOW_DISCONNECTED => 8;

sub register_commands {
    BarnOwl::new_command(
        'irc-connect' => \&cmd_connect,
        {
            summary => 'Connect to an IRC server',
            usage =>
'irc-connect [-a ALIAS] [-s] [-p PASSWORD] [-n NICK] SERVER [port]',
            description => <<END_DESCR
Connect to an IRC server. Supported options are:

 -a <alias>          Define an alias for this server
 -s                  Use SSL
 -p <password>       Specify the password to use
 -n <nick>           Use a non-default nick

The -a option specifies an alias to use for this connection. This
alias can be passed to the '-a' argument of any other IRC command to
control which connection it operates on.

For servers with hostnames of the form "irc.FOO.{com,org,...}", the
alias will default to "FOO"; For other servers the full hostname is
used.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-disconnect' => mk_irc_command( \&cmd_disconnect, ALLOW_DISCONNECTED ),
        {
            summary => 'Disconnect from an IRC server',
            usage   => 'irc-disconnect [-a ALIAS]',

            description => <<END_DESCR
Disconnect from an IRC server. You can specify a specific server with
"-a SERVER-ALIAS" if necessary.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-msg' => mk_irc_command( \&cmd_msg, CHANNEL_OR_USER|CHANNEL_ARG|CHANNEL_OPTIONAL ),
        {
            summary => 'Send an IRC message',
            usage   => 'irc-msg [-a ALIAS] DESTINATION MESSAGE',

            description => <<END_DESCR
Send an IRC message.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-mode' => mk_irc_command( \&cmd_mode, CHANNEL_OPTIONAL|CHANNEL_ARG ),
        {
            summary => 'Change an IRC channel or user mode',
            usage   => 'irc-mode [-a ALIAS] TARGET [+-]MODE OPTIONS',

            description => <<END_DESCR
Change the mode of an IRC user or channel.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-join' => mk_irc_command( \&cmd_join ),
        {
            summary => 'Join an IRC channel',
            usage   => 'irc-join [-a ALIAS] [-t] #channel [KEY]',

            description => <<END_DESCR
Join an IRC channel.  If the -t option is present the subscription will only be
temporary, i.e., it will not be written to the subscription file and will
therefore not be present the next time BarnOwl is started, and will disappear
if the connection is lost.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-part' => mk_irc_command( \&cmd_part, CHANNEL_ARG ),
        {
            summary => 'Leave an IRC channel',
            usage   => 'irc-part [-a ALIAS] [-t] #channel',

            description => <<END_DESCR
Part from an IRC channel.  If the -t option is present the unsubscription will
only be temporary, i.e., it will not be updated in the subscription file and
will therefore not be in effect the next time BarnOwl is started, or if the
connection is lost.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-nick' => mk_irc_command( \&cmd_nick ),
        {
            summary => 'Change your IRC nick on an existing connection.',
            usage   => 'irc-nick [-a ALIAS] NEW-NICK',

            description => <<END_DESCR
Set your IRC nickname on an existing connect. To change it prior to
connecting, adjust the `irc:nick' variable.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-names' => mk_irc_command( \&cmd_names, CHANNEL_ARG ),
        {
            summary => 'View the list of users in a channel',
            usage   => 'irc-names [-a ALIAS] #channel',

            description => <<END_DESCR
`irc-names' displays the list of users in a given channel in a pop-up
window.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-whois' => mk_irc_command( \&cmd_whois ),
        {
            summary => 'Displays information about a given IRC user',
            usage   => 'irc-whois [-a ALIAS] NICK',

            description => <<END_DESCR
Pops up information about a given IRC user.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-motd' => mk_irc_command( \&cmd_motd ),
        {
            summary => 'Displays an IRC server\'s MOTD (Message of the Day)',
            usage   => 'irc-motd [-a ALIAS]',

            description => <<END_DESCR
Displays an IRC server's message of the day.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-list' => \&cmd_list,
        {
            summary => 'Show all the active IRC connections.',
            usage   => 'irc-list',

            description => <<END_DESCR
Show all the currently active IRC connections with their aliases and
server names.
END_DESCR
        }
    );

    BarnOwl::new_command( 'irc-who'   => mk_irc_command( \&cmd_who ) );
    BarnOwl::new_command( 'irc-stats' => mk_irc_command( \&cmd_stats ) );

    BarnOwl::new_command(
        'irc-topic' => mk_irc_command( \&cmd_topic, CHANNEL_ARG ),
        {
            summary => 'View or change the topic of an IRC channel',
            usage   => 'irc-topic [-a ALIAS] #channel [TOPIC]',

            description => <<END_DESCR
Without extra arguments, fetches and displays a given channel's topic.

With extra arguments, changes the target channel's topic string. This
may require +o on some channels.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-quote' => mk_irc_command( \&cmd_quote ),
        {
            summary => 'Send a raw command to the IRC servers.',
            usage   => 'irc-quote [-a ALIAS] TEXT',

            description => <<END_DESCR
Send a raw command line to an IRC server.

This can be used to perform some operation not yet supported by
BarnOwl, or to define new IRC commands.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-loadchannels' => \&cmd_loadchannels,
        {
            summary => 'Reload persistent channels',
            usage   => 'irc-loadchannels [-a ALIAS] [<file>]',

            description => <<END_DESCR
Load persistent channels from a file.  The file defaults to
\$HOME/.owl/$IRC_SUBS_FILENAME.  If the ALIAS is present, only channels
on the given alias are loaded.  The ALIAS is case-sensitive.

Each line of the file should describe a single channel, in the format
'\$alias \$channel' (without quotes).
END_DESCR
        }
    );
}


$BarnOwl::Hooks::startup->add('BarnOwl::Module::IRC::startup');
$BarnOwl::Hooks::shutdown->add('BarnOwl::Module::IRC::shutdown');
$BarnOwl::Hooks::getQuickstart->add('BarnOwl::Module::IRC::quickstart');
$BarnOwl::Hooks::getBuddyList->add("BarnOwl::Module::IRC::buddylist");

################################################################################
######################## Owl command handlers ##################################
################################################################################

sub make_autoconnect_filename {
    # can't use ||, or else we'll treat '0' as invalid.  We could check for eq "" ...
    # TODO(jgross): When we move to requiring perl 5.10, combine the
    # following two lines using //
    my $filename = shift;
    $filename = File::Spec->catfile(BarnOwl::get_config_dir(), $IRC_SUBS_FILENAME) unless defined $filename;
    if (!File::Spec->file_name_is_absolute($filename)) {
        $filename = File::Spec->catfile($ENV{HOME}, $filename);
    }
    return $filename;
}

sub _get_autoconnect_lines {
    my $filename = shift;

    # TODO(jgross): Write a C-side function to do this, asynchronously;
    #               AIUI, perl doesn't do asynchronous I/O in any useful way
    if (open (my $subsfile, "<:encoding(UTF-8)", $filename)) {
        my @lines = <$subsfile>;
        close($subsfile);

        # strip trailing newlines
        local $/ = "";
        chomp(@lines);

        return @lines;
    }

    return ();
}

sub get_autoconnect_channels {
    my $filename = make_autoconnect_filename(shift);
    my %channel_hash = ();

    # Load the subs from the file
    my @lines = _get_autoconnect_lines($filename);

    foreach my $line (@lines) {
        my @parsed_args = split(' ', $line);
        if (scalar @parsed_args == 2) {
            push @{$channel_hash{$parsed_args[0]}}, $parsed_args[1];
        } else {
            warn "Trouble parsing irc configuration file '$filename' line '$line'; the format is '\$alias \$channel', with no spaces in either\n";
        }
    }

    return %channel_hash;
}

sub add_autoconnect_channel {
    my $conn = shift;
    my $channel = shift;
    my $alias = $conn->alias;
    my $filename = make_autoconnect_filename(shift);

    # we already checked for spaces in $channel in cmd_join, but we still need
    # to check $alias
    die "Alias name '$alias' contains a space; parsing will fail.  Use the -t flag.\n" unless index($alias, " ") == -1;

    my $line = "$alias $channel";

    my @lines = _get_autoconnect_lines($filename);

    # We don't want to be noisy about duplicated joins.  For example, some
    # people might have :irc-join in startup files, even though that doesn't
    # work correctly anymore because connect is asynchronous and so join on
    # startup races with connect.  Regardless, just fail silently if the line
    # already exists.
    return if grep { $_ eq $line } @lines;

    open (my $subsfile, ">>:encoding(UTF-8)", make_autoconnect_filename($filename))
        or die "Cannot open $filename for writing: $!\n";
    local $, = "";
    local $/ = "";
    print $subsfile "$line\n";
    close($subsfile);
}

sub remove_autoconnect_channel {
    my $conn = shift;
    my $channel = shift;
    my $alias = $conn->alias;
    my $filename = make_autoconnect_filename(shift);

    BarnOwl::Internal::file_deleteline($filename, "$alias $channel", 1);
}

sub cmd_loadchannels {
    my $cmd = shift;
    my $alias;
    my $getopt = Getopt::Long::Parser->new;

    local @ARGV = @_;
    $getopt->configure(qw(pass_through permute no_getopt_compat prefix_pattern=-|--));
    $getopt->getoptions("alias=s" => \$alias);

    my %channel_hash = get_autoconnect_channels(@ARGV);

    my $aliases = (defined $alias) ? [$alias] : [keys %channel_hash];

    foreach my $cur_alias (@$aliases) {
        # get_connection_by_alias might die, and we don't want to
        eval {
            my $conn = get_connection_by_alias($cur_alias, 1);
            my %existing_channels = map { $_ => 1 } @{$conn->autoconnect_channels}, @{$channel_hash{$cur_alias}};
            $conn->autoconnect_channels([keys %existing_channels]);
        };
        foreach my $channel (@{$channel_hash{$cur_alias}}) {
            if ($cur_alias eq "") {
                BarnOwl::command("irc-join", "-t", $channel);
            } else {
                BarnOwl::command("irc-join", "-t", "-a", $cur_alias, $channel);
            }
        }
    }
}

sub cmd_connect {
    my $cmd = shift;

    my $nick = BarnOwl::getvar('irc:nick');
    my $username = BarnOwl::getvar('irc:user');
    my $ircname = BarnOwl::getvar('irc:name');
    my $host;
    my $port;
    my $alias;
    my $ssl;
    my $password = undef;

    {
        local @ARGV = @_;
        GetOptions(
            "alias=s"    => \$alias,
            "ssl"        => \$ssl,
            "password=s" => \$password,
            "nick=s"     => \$nick,
        );
        $host = shift @ARGV or die("Usage: $cmd HOST\n");
        if(!$alias) {
            if($host =~ /^(?:irc[.])?([\w-]+)[.]\w+$/) {
                $alias = $1;
            } else {
                $alias = $host;
            }
        }
        $ssl ||= 0;
        $port = shift @ARGV || ($ssl ? 6697 : 6667);
    }

    if(exists $ircnets{$alias}) {
        die("Already connected to a server with alias '$alias'. Either disconnect or specify an alias with -a.\n");
    }

    my %channel_hash = get_autoconnect_channels;

    my $conn = BarnOwl::Module::IRC::Connection->new($alias, $host, $port, {
        nick                 => $nick,
        user                 => $username,
        real                 => $ircname,
        password             => $password,
        SSL                  => $ssl,
        timeout              => sub {0},
        autoconnect_channels => $channel_hash{$alias}
       });
    $ircnets{$alias} = $conn;
    return;
}

sub cmd_disconnect {
    my $cmd = shift;
    my $conn = shift;
    if ($conn->conn->{socket}) {
        $conn->did_quit(1);
        $conn->conn->disconnect("Goodbye!");
    } elsif ($conn->{reconnect_timer}) {
        BarnOwl::admin_message('IRC',
                               "[" . $conn->alias . "] Reconnect cancelled");
        $conn->cancel_reconnect;
        delete $ircnets{$conn->alias};
    } elsif (exists $ircnets{$conn->alias}) { # inconsistent state; no socket, but not yet deleted
        BarnOwl::admin_message('IRC',
                               "[" . $conn->alias . "] Attempt to disconnect from a socketless connection; deleting it");
        delete $ircnets{$conn->alias};
    }
}

sub cmd_msg {
    my $cmd  = shift;
    my $conn = shift;
    my $to = shift or die("Usage: $cmd [NICK|CHANNEL]\n");
    # handle multiple recipients?
    if(@_) {
        process_msg($conn, $to, join(" ", @_));
    } else {
        BarnOwl::start_edit_win(BarnOwl::quote('/msg', '-a', $conn->alias, $to), sub {process_msg($conn, $to, @_)});
    }
    return;
}

sub process_msg {
    my $conn = shift;
    my $to = shift;
    my $fullbody = shift;
    my @msgs;
    # Require the user to send in paragraphs (double-newline between) to
    # actually send multiple PRIVMSGs, in order to play nice with autofill.
    $fullbody =~ s/\r//g;
    @msgs = split "\n\n", $fullbody;
    map { tr/\n/ / } @msgs;
    # split each body at irc:max-message-length characters, if that number
    # is positive.  Only split at space boundaries.  Start counting a-fresh
    # at the beginning of each paragraph
    my $max_len = BarnOwl::getvar('irc:max-message-length');
    if ($max_len > 0) {
        local($Text::Wrap::columns) = $max_len;
        @msgs = split "\n", wrap("", "", join "\n", @msgs);
    }
    for my $body (@msgs) {
	if ($body =~ /^\/me (.*)/) {
	    $conn->me($to, Encode::encode('utf-8', $1));
	    $body = '* '.$conn->nick.' '.$1;
	} else {
	    $conn->conn->send_msg('privmsg', $to, Encode::encode('utf-8', $body));
	}
	my $msg = BarnOwl::Message->new(
	    type        => 'IRC',
	    direction   => is_private($to) ? 'out' : 'in',
	    server      => $conn->server,
	    network     => $conn->alias,
	    recipient   => $to,
	    body        => $body,
	    sender      => $conn->nick,
	    is_private($to) ?
	      (isprivate  => 'true') : (channel => $to),
	    replycmd    => BarnOwl::quote('irc-msg',  '-a', $conn->alias, $to),
	    replysendercmd => BarnOwl::quote('irc-msg', '-a', $conn->alias, $to),
	);
	BarnOwl::queue_message($msg);
    }
    return;
}

sub cmd_mode {
    my $cmd = shift;
    my $conn = shift;
    my $target = shift;
    $target ||= shift;
    $conn->conn->send_msg(mode => $target, @_);
    return;
}

sub cmd_join {
    my $cmd = shift;
    my $is_temporary;

    my $getopt = Getopt::Long::Parser->new;

    local @ARGV = @_;
    $getopt->configure(qw(pass_through permute no_getopt_compat prefix_pattern=-|--));
    $getopt->getoptions("temporary" => \$is_temporary);

    my $conn = shift @ARGV;
    my $chan = shift @ARGV or die("Usage: $cmd channel\n");

    die "Channel name '$chan' contains a space.  As per RFC 2812, IRC channel names may not contain spaces.\n" unless index($chan, " ") == -1;

    $conn->conn->send_msg(join => $chan, @ARGV);

    # regardless of whether or not this is temporary, we want to persist it
    # across reconnects.

    # check if the channel is already in the list
    if (!grep { $_ eq $chan } @{$conn->autoconnect_channels}) {
        push @{$conn->autoconnect_channels}, $chan;
    }

    if (!$is_temporary) {
        # add the line to the subs file
        add_autoconnect_channel($conn, $chan);
    }

    return;
}

sub cmd_part {
    my $cmd = shift;
    my $is_temporary;

    my $getopt = Getopt::Long::Parser->new;

    local @ARGV = @_;
    $getopt->configure(qw(pass_through permute no_getopt_compat prefix_pattern=-|--));
    $getopt->getoptions("temporary" => \$is_temporary);

    my $conn = shift @ARGV;
    my $chan = shift @ARGV or die("Usage: $cmd channel\n");

    $conn->conn->send_msg(part => $chan);

    # regardless of whether or not this is temporary, we want to persist it
    # across reconnects
    my %existing_channels = map { $_ => 1 } @{$conn->autoconnect_channels};
    delete $existing_channels{$chan};
    $conn->autoconnect_channels([keys %existing_channels]);

    if (!$is_temporary) {
        # remove the line from the subs file
        remove_autoconnect_channel($conn, $chan);
    }

    return;
}

sub cmd_nick {
    my $cmd = shift;
    my $conn = shift;
    my $nick = shift or die("Usage: $cmd <new nick>\n");
    $conn->conn->send_msg(nick => $nick);
    return;
}

sub cmd_names {
    my $cmd = shift;
    my $conn = shift;
    my $chan = shift;
    $conn->names_tmp([]);
    $conn->conn->send_msg(names => $chan);
    return;
}

sub cmd_whois {
    my $cmd = shift;
    my $conn = shift;
    my $who = shift || die("Usage: $cmd <user>\n");
    $conn->conn->send_msg(whois => $who);
    return;
}

sub cmd_motd {
    my $cmd = shift;
    my $conn = shift;
    $conn->conn->send_msg('motd');
    return;
}

sub cmd_list {
    my $cmd = shift;
    my $message = BarnOwl::Style::boldify('Current IRC networks:') . "\n";
    while (my ($alias, $conn) = each %ircnets) {
        $message .= '  ' . $alias . ' => ' . $conn->nick . '@' . $conn->server . "\n";
    }
    BarnOwl::popless_ztext($message);
    return;
}

sub cmd_who {
    my $cmd = shift;
    my $conn = shift;
    my $who = shift || die("Usage: $cmd <user>\n");
    $conn->conn->send_msg(who => $who);
    return;
}

sub cmd_stats {
    my $cmd = shift;
    my $conn = shift;
    my $type = shift || die("Usage: $cmd <chiklmouy> [server] \n");
    $conn->conn->send_msg(stats => $type, @_);
    return;
}

sub cmd_topic {
    my $cmd = shift;
    my $conn = shift;
    my $chan = shift;
    $conn->conn->send_msg(topic => $chan, @_ ? join(" ", @_) : undef);
    return;
}

sub cmd_quote {
    my $cmd = shift;
    my $conn = shift;
    $conn->conn->send_msg(@_);
    return;
}

################################################################################
########################### Utilities/Helpers ##################################
################################################################################

sub find_channel {
    my $channel = shift;
    my @found;
    for my $conn (values %ircnets) {
        if($conn->conn->{channel_list}{lc $channel}) {
            push @found, $conn;
        }
    }
    return $found[0] if(scalar @found == 1);
}

sub mk_irc_command {
    my $sub = shift;
    my $flags = shift || 0;
    return sub {
        my $cmd = shift;
        my $conn;
        my $alias;
        my $channel;
        my $is_temporary;
        my $getopt = Getopt::Long::Parser->new;
        my $m = BarnOwl::getcurmsg();

        local @ARGV = @_;
        $getopt->configure(qw(pass_through permute no_getopt_compat prefix_pattern=-|--));
        $getopt->getoptions("alias=s" => \$alias,
                            "temporary" => \$is_temporary);

        if(defined($alias)) {
            $conn = get_connection_by_alias($alias,
                                            $flags & ALLOW_DISCONNECTED);
        }
        if($flags & CHANNEL_ARG) {
            $channel = $ARGV[0];
            if(defined($channel) && $channel =~ /^#/) {
                if(my $c = find_channel($channel)) {
                    shift @ARGV;
                    $conn ||= $c;
                }
            } elsif (defined($channel) && ($flags & CHANNEL_OR_USER)) {
                shift @ARGV;
            } elsif ($m && $m->type eq 'IRC' && !$m->is_private) {
                $channel = $m->channel;
            } else {
                undef $channel;
            }
        }

        if(!defined($channel) &&
           ($flags & CHANNEL_ARG) &&
           !($flags & CHANNEL_OPTIONAL)) {
            die("Usage: $cmd <channel>\n");
        }
        if(!$conn) {
            if($m && $m->type eq 'IRC') {
                $conn = get_connection_by_alias($m->network,
                                               $flags & ALLOW_DISCONNECTED);
            }
        }
        if(!$conn && scalar keys %ircnets == 1) {
            $conn = [values(%ircnets)]->[0];
        }
        if(!$conn) {
            die("You must specify an IRC network using -a.\n");
        }
        push @ARGV, "-t" if $is_temporary;
        if($flags & CHANNEL_ARG) {
            $sub->($cmd, $conn, $channel, @ARGV);
        } else {
            $sub->($cmd, $conn, @ARGV);
        }
    };
}

sub get_connection_by_alias {
    my $key = shift;
    my $allow_disconnected = shift;

    my $conn = $ircnets{$key};
    die("No such ircnet: $key\n") unless $conn;
    if ($conn->conn->{registered} || $allow_disconnected) {
        return $conn;
    }
    die("[@{[$conn->alias]}] Not currently connected.");
}

1;
