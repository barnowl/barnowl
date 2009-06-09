use strict;
use warnings;

package BarnOwl::Module::IRC;

=head1 NAME

BarnOwl::Module::IRC

=head1 DESCRIPTION

This module implements IRC support for barnowl.

=cut

use BarnOwl;
use BarnOwl::Hooks;
use BarnOwl::Message::IRC;
use BarnOwl::Module::IRC::Connection qw(is_private);

use Net::IRC;
use Getopt::Long;

our $VERSION = 0.02;

our $irc;

# Hash alias -> BarnOwl::Module::IRC::Connection object
our %ircnets;
our %channels;

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
        'admin messages. Intended for debugging and development use only '
       });
    
    register_commands();
    register_handlers();
    BarnOwl::filter('irc type ^IRC$');
}

sub shutdown {
    for my $conn (values %ircnets) {
        $conn->conn->disconnect();
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

#sub mainloop_hook {
#    return unless defined $irc;
#    eval {
#        $irc->do_one_loop();
#    };
#    return;
#}

sub OwlProcess {
    return unless defined $irc;
    eval {
        $irc->do_one_loop();
    };
    return;
}


sub register_handlers {
    if(!$irc) {
        $irc = Net::IRC->new;
        $irc->timeout(0);
    }
}

use constant OPTIONAL_CHANNEL => 1;
use constant REQUIRE_CHANNEL => 2;

sub register_commands {
    BarnOwl::new_command(
        'irc-connect' => \&cmd_connect,
        {
            summary => 'Connect to an IRC server',
            usage =>
'irc-connect [-a ALIAS ] [-s] [-p PASSWORD] [-n NICK] SERVER [port]',
            description =>

              <<END_DESCR
Connect to an IRC server. Supported options are

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
        'irc-disconnect' => mk_irc_command( \&cmd_disconnect ),
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
        'irc-msg' => mk_irc_command( \&cmd_msg ),
        {
            summary => 'Send an IRC message',
            usage   => 'irc-msg [-a ALIAS] DESTINATION MESSAGE',

            description => <<END_DESCR
Send an IRC message
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-mode' => mk_irc_command( \&cmd_mode, OPTIONAL_CHANNEL ),
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
            usage   => 'irc-join [-a ALIAS] #channel [KEY]',

            description => <<END_DESCR

Join an IRC channel.
END_DESCR
        }
    );

    BarnOwl::new_command(
        'irc-part' => mk_irc_command( \&cmd_part, REQUIRE_CHANNEL ),
        {
            summary => 'Leave an IRC channel',
            usage   => 'irc-part [-a ALIAS] #channel',

            description => <<END_DESCR

Part from an IRC channel.
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
        'irc-names' => mk_irc_command( \&cmd_names, REQUIRE_CHANNEL ),
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
        'irc-topic' => mk_irc_command( \&cmd_topic, REQUIRE_CHANNEL ),
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
}


$BarnOwl::Hooks::startup->add('BarnOwl::Module::IRC::startup');
$BarnOwl::Hooks::shutdown->add('BarnOwl::Module::IRC::shutdown');
$BarnOwl::Hooks::getQuickstart->add('BarnOwl::Module::IRC::quickstart');

################################################################################
######################## Owl command handlers ##################################
################################################################################

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

    my $conn = BarnOwl::Module::IRC::Connection->new($irc, $alias,
        Nick      => $nick,
        Server    => $host,
        Port      => $port,
        Username  => $username,
        Ircname   => $ircname,
        Port      => $port,
        Password  => $password,
        SSL       => $ssl
       );

    if ($conn->conn->connected) {
        BarnOwl::admin_message("IRC", "Connected to $alias as $nick");
        $ircnets{$alias} = $conn;
        my $fd = $conn->getSocket()->fileno();
        BarnOwl::add_dispatch($fd, \&OwlProcess);
        $conn->{FD} = $fd;
    } else {
        die("IRC::Connection->connect failed: $!");
    }

    return;
}

sub cmd_disconnect {
    my $cmd = shift;
    my $conn = shift;
    $conn->conn->disconnect;
    delete $ircnets{$conn->alias};
}

sub cmd_msg {
    my $cmd  = shift;
    my $conn = shift;
    my $to = shift or die("Usage: $cmd [NICK|CHANNEL]\n");
    # handle multiple recipients?
    if(@_) {
        process_msg($conn, $to, join(" ", @_));
    } else {
        BarnOwl::start_edit_win("/msg -a " . $conn->alias . " $to", sub {process_msg($conn, $to, @_)});
    }
}

sub process_msg {
    my $conn = shift;
    my $to = shift;
    my $body = shift;
    # Strip whitespace. In the future -- send one message/line?
    $body =~ tr/\n\r/  /;
    if ($body =~ /^\/me (.*)/) {
        $conn->conn->me($to, $1);
        $body = '* '.$conn->nick.' '.$1;
    } else {
        $conn->conn->privmsg($to, $body);
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
        replycmd    => "irc-msg -a " . $conn->alias . " $to",
        replysendercmd => "irc-msg -a " . $conn->alias . " $to"
       );
    BarnOwl::queue_message($msg);
}

sub cmd_mode {
    my $cmd = shift;
    my $conn = shift;
    my $target = shift;
    $target ||= shift;
    $conn->conn->mode($target, @_);
}

sub cmd_join {
    my $cmd = shift;
    my $conn = shift;
    my $chan = shift or die("Usage: $cmd channel\n");
    $channels{$chan} ||= [];
    push @{$channels{$chan}}, $conn;
    $conn->conn->join($chan, @_);
}

sub cmd_part {
    my $cmd = shift;
    my $conn = shift;
    my $chan = shift;
    $channels{$chan} = [grep {$_ ne $conn} @{$channels{$chan} || []}];
    $conn->conn->part($chan);
}

sub cmd_nick {
    my $cmd = shift;
    my $conn = shift;
    my $nick = shift or die("Usage: $cmd <new nick>\n");
    $conn->conn->nick($nick);
}

sub cmd_names {
    my $cmd = shift;
    my $conn = shift;
    my $chan = shift;
    $conn->names_tmp([]);
    $conn->conn->names($chan);
}

sub cmd_whois {
    my $cmd = shift;
    my $conn = shift;
    my $who = shift || die("Usage: $cmd <user>\n");
    $conn->conn->whois($who);
}

sub cmd_motd {
    my $cmd = shift;
    my $conn = shift;
    $conn->conn->motd;
}

sub cmd_list {
    my $cmd = shift;
    my $message = BarnOwl::Style::boldify('Current IRC networks:') . "\n";
    while (my ($alias, $conn) = each %ircnets) {
        $message .= '  ' . $alias . ' => ' . $conn->nick . '@' . $conn->server . "\n";
    }
    BarnOwl::popless_ztext($message);
}

sub cmd_who {
    my $cmd = shift;
    my $conn = shift;
    my $who = shift || die("Usage: $cmd <user>\n");
    BarnOwl::error("WHO $cmd $conn $who");
    $conn->conn->who($who);
}

sub cmd_stats {
    my $cmd = shift;
    my $conn = shift;
    my $type = shift || die("Usage: $cmd <chiklmouy> [server] \n");
    $conn->conn->stats($type, @_);
}

sub cmd_topic {
    my $cmd = shift;
    my $conn = shift;
    my $chan = shift;
    $conn->conn->topic($chan, @_ ? join(" ", @_) : undef);
}

sub cmd_quote {
    my $cmd = shift;
    my $conn = shift;
    $conn->conn->sl(join(" ", @_));
}

################################################################################
########################### Utilities/Helpers ##################################
################################################################################

sub mk_irc_command {
    my $sub = shift;
    my $use_channel = shift || 0;
    return sub {
        my $cmd = shift;
        my $conn;
        my $alias;
        my $channel;
        my $getopt = Getopt::Long::Parser->new;
        my $m = BarnOwl::getcurmsg();

        local @ARGV = @_;
        $getopt->configure(qw(pass_through permute no_getopt_compat prefix_pattern=-|--));
        $getopt->getoptions("alias=s" => \$alias);

        if(defined($alias)) {
            $conn = get_connection_by_alias($alias);
        }
        if(!$conn && $use_channel) {
            $channel = $ARGV[0];
            if(defined($channel) && $channel =~ /^#/) {
                if($channels{$channel} && @{$channels{$channel}} == 1) {
                    shift @ARGV;
                    $conn = $channels{$channel}[0];
                }  
            } else {
                if($m && $m->type eq 'IRC' && !$m->is_private) {
                    $channel = $m->channel;
                } else {
                    undef $channel;
                }
            }
        }
        if(!$channel && $use_channel == REQUIRE_CHANNEL) {
            die("Usage: $cmd <channel>\n");
        }
        if(!$conn) {
            if($m && $m->type eq 'IRC') {
                $conn = get_connection_by_alias($m->network);
            }
        }
        if(!$conn && scalar keys %ircnets == 1) {
            $conn = [values(%ircnets)]->[0];
        }
        if(!$conn) {
            die("You must specify an IRC network using -a.\n");
        }
        if($use_channel) {
            $sub->($cmd, $conn, $channel, @ARGV);
        } else {
            $sub->($cmd, $conn, @ARGV);
        }
    };
}

sub get_connection_by_alias {
    my $key = shift;
    die("No such ircnet: $key\n") unless exists $ircnets{$key};
    return $ircnets{$key};
}

1;
