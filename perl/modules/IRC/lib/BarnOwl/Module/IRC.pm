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

sub mainloop_hook {
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

sub register_commands {
    BarnOwl::new_command('irc-connect' => \&cmd_connect,
                       {
                           summary      => 'Connect to an IRC server',
                           usage        => 'irc-connect [-a ALIAS ] [-s] [-p PASSWORD] [-n NICK] SERVER [port]',
                           description  =>

                           "Connect to an IRC server. Supported options are\n\n" .
                           "-a <alias>          Define an alias for this server\n" .
                           "-s                  Use SSL\n" .
                           "-p <password>       Specify the password to use\n" .
                           "-n <nick>           Use a non-default nick"
                       });
    BarnOwl::new_command('irc-disconnect' => \&cmd_disconnect);
    BarnOwl::new_command('irc-msg'        => \&cmd_msg);
    BarnOwl::new_command('irc-join'       => \&cmd_join);
    BarnOwl::new_command('irc-part'       => \&cmd_part);
    BarnOwl::new_command('irc-nick'       => \&cmd_nick);
    BarnOwl::new_command('irc-names'      => \&cmd_names);
    BarnOwl::new_command('irc-whois'      => \&cmd_whois);
    BarnOwl::new_command('irc-motd'       => \&cmd_motd);
}

$BarnOwl::Hooks::startup->add(\&startup);
$BarnOwl::Hooks::shutdown->add(\&shutdown);
$BarnOwl::Hooks::mainLoop->add(\&mainloop_hook);

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
            if($host =~ /^(?:irc[.])?(\w+)[.]\w+$/) {
                $alias = $1;
            } else {
                $alias = $host;
            }
        }
        $port = shift @ARGV || 6667;
        $ssl ||= 0;
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

    if ($conn->connected) {
        BarnOwl::admin_message("IRC", "Connected to $alias as $nick");
        $ircnets{$alias} = $conn;
    } else {
        die("IRC::Connection->connect failed: $!");
    }

    return;
}

sub cmd_disconnect {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    $conn->conn->disconnect;
    delete $ircnets{$conn->alias};
}

sub cmd_msg {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    my $to = shift or die("Usage: $cmd NICK\n");
    # handle multiple recipients?
    if(@_) {
        process_msg($conn, $to, join(" ", @_));
    } else {
        BarnOwl::start_edit_win("/msg $to -a " . $conn->alias, sub {process_msg($conn, $to, @_)});
    }
}

sub process_msg {
    my $conn = shift;
    my $to = shift;
    my $body = shift;
    # Strip whitespace. In the future -- send one message/line?
    $body =~ tr/\n\r/  /;
    $conn->conn->privmsg($to, $body);
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
        replycmd    => "irc-msg $to",
        replysendercmd => "irc-msg $to"
       );
    BarnOwl::queue_message($msg);
}

sub cmd_join {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    my $chan = shift or die("Usage: $cmd channel\n");
    $channels{$chan} ||= [];
    push @{$channels{$chan}}, $conn;
    $conn->conn->join($chan);
}

sub cmd_part {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    my $chan = get_channel(\@_) || die("Usage: $cmd <channel>\n");
    $channels{$chan} = [grep {$_ ne $conn} @{$channels{$chan} || []}];
    $conn->conn->part($chan);
}

sub cmd_nick {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    my $nick = shift or die("Usage: $cmd <new nick>\n");
    $conn->conn->nick($nick);
}

sub cmd_names {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    my $chan = get_channel(\@_) || die("Usage: $cmd <channel>\n");
    $conn->conn->names($chan);
}

sub cmd_whois {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    my $who = shift || die("Usage: $cmd <user>\n");
    $conn->conn->whois($who);
}

sub cmd_motd {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    $conn->conn->motd;
}

################################################################################
########################### Utilities/Helpers ##################################
################################################################################

sub get_connection {
    my $args = shift;
    if(scalar @$args >= 2 && $args->[0] eq '-a') {
        shift @$args;
        return get_connection_by_alias(shift @$args);
    }
    my $channel = $args->[-1];
    if (defined($channel) && $channel =~ /^#/
        and $channels{$channel} and @{$channels{$channel}} == 1) {
        return $channels{$channel}[0];
    }
    my $m = BarnOwl::getcurmsg();
    if($m && $m->type eq 'IRC') {
        return get_connection_by_alias($m->network);
    }
    if(scalar keys %ircnets == 1) {
        return [values(%ircnets)]->[0];
    }
    die("You must specify a network with -a\n");
}

sub get_channel {
    my $args = shift;
    if(scalar @$args) {
        return shift @$args;
    }
    my $m = BarnOwl::getcurmsg();
    if($m && $m->type eq 'IRC') {
        return $m->channel if !$m->is_private;
    }
    return undef;
}

sub get_connection_by_alias {
    my $key = shift;
    die("No such ircnet: $key\n") unless exists $ircnets{$key};
    return $ircnets{$key};
}

1;
