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

sub startup {
    BarnOwl::new_variable_string('irc:nick', {default => $ENV{USER}});
    BarnOwl::new_variable_string('irc:user', {default => $ENV{USER}});
    BarnOwl::new_variable_string('irc:name', {default => ""});
    BarnOwl::new_variable_bool('irc:spew', {default => 0});
    register_commands();
    register_handlers();
    BarnOwl::filter('irc type ^IRC$');
}

sub shutdown {
    for my $conn (values %ircnets) {
        $conn->disconnect();
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
    BarnOwl::new_command('irc-connect' => \&cmd_connect);
    BarnOwl::new_command('irc-disconnect' => \&cmd_disconnect);
    BarnOwl::new_command('irc-msg'     => \&cmd_msg);
    BarnOwl::new_command('irc-join' => \&cmd_join);
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
            "port=i"     => \$port,
        );
        $host = shift @ARGV or die("Usage: $cmd HOST\n");
        if(!$alias) {
            $alias = $1 if $host =~ /^(?:irc[.])?(\w+)[.]\w+$/;
            $alias ||= $host;
        }
        $port ||= 6667;
        $ssl ||= 0;
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

    $ircnets{$alias} = $conn;
    return;
}

sub cmd_disconnect {
    my $cmd = shift;
    my $conn = get_connection(\@_);
    $conn->disconnect;
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
    $conn->privmsg($to, $body);
    my $msg = BarnOwl::Message->new(
        type        => 'IRC',
        direction   => 'out',
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
    $conn->join($chan);
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
    my $m = BarnOwl::getcurmsg();
    if($m && $m->type eq 'IRC') {
        return get_connection_by_alias($m->network);
    }
    if(scalar keys %ircnets == 1) {
        return [values(%ircnets)]->[0];
    }
    die("You must specify a network with -a\n");
}

sub get_connection_by_alias {
    my $key = shift;
    die("No such ircnet: $key\n") unless exists $ircnets{$key};
    return $ircnets{$key};
}

1;
