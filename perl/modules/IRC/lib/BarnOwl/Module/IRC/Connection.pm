use strict;
use warnings;

package BarnOwl::Module::IRC::Connection;
use BarnOwl::Timer;

=head1 NAME

BarnOwl::Module::IRC::Connection

=head1 DESCRIPTION

This module is a wrapper around AnyEvent::IRC::Client for BarnOwl's IRC
support

=cut

use AnyEvent::IRC::Client;
use AnyEvent::IRC::Util qw(split_prefix prefix_nick encode_ctcp);

use base qw(Class::Accessor);
use Exporter 'import';
__PACKAGE__->mk_accessors(qw(conn alias motd names_tmp whois_tmp
                             server autoconnect_channels
                             connect_args backoff did_quit));
our @EXPORT_OK = qw(is_private);

use BarnOwl;
use Scalar::Util qw(weaken);

sub new {
    my $class = shift;
    my $alias = shift;
    my $host  = shift;
    my $port  = shift;
    my $args  = shift;
    my $nick = $args->{nick};
    my $conn = AnyEvent::IRC::Client->new();
    my $self = bless({}, $class);
    $self->conn($conn);
    $self->autoconnect_channels([]);
    $self->alias($alias);
    $self->server($host);
    $self->motd("");
    $self->names_tmp(0);
    $self->backoff(0);
    $self->whois_tmp("");
    $self->did_quit(0);

    if(delete $args->{SSL}) {
        $conn->enable_ssl;
    }
    $self->connect_args([$host, $port, $args]);
    $conn->connect($host, $port, $args);
    $conn->{heap}{parent} = $self;
    weaken($conn->{heap}{parent});

    sub on {
        my $meth = "on_" . shift;
        return sub {
            my $conn = shift;
            return unless $conn->{heap}{parent};
            $conn->{heap}{parent}->$meth(@_);
        }
    }

    # $self->conn->add_default_handler(sub { shift; $self->on_event(@_) });
    $self->conn->reg_cb(registered => on("connect"),
                        connfail   => sub { BarnOwl::error("Connection to $host failed!") },
                        disconnect => on("disconnect"),
                        publicmsg  => on("msg"),
                        privatemsg => on("msg"),
                        irc_error  => on("error"));
    for my $m (qw(welcome yourhost created
                  luserclient luserop luserchannels luserme
                  error)) {
        $self->conn->reg_cb("irc_$m" => on("admin_msg"));
    }
    $self->conn->reg_cb(irc_375       => on("motdstart"),
                        irc_372       => on("motd"),
                        irc_376       => on("endofmotd"),
                        irc_join      => on("join"),
                        irc_part      => on("part"),
                        irc_quit      => on("quit"),
                        irc_433       => on("nickinuse"),
                        channel_topic => on("topic"),
                        irc_333       => on("topicinfo"),
                        irc_353       => on("namreply"),
                        irc_366       => on("endofnames"),
                        irc_311       => on("whois"),
                        irc_312       => on("whois"),
                        irc_319       => on("whois"),
                        irc_320       => on("whois"),
                        irc_318       => on("endofwhois"),
                        irc_mode      => on("mode"),
                        irc_401       => on("nosuch"),
                        irc_402       => on("nosuch"),
                        irc_403       => on("nosuch"),
                        nick_change   => on("nick"),
                        ctcp_action   => on("ctcp_action"),
                        'irc_*' => sub { BarnOwl::debug("IRC: " . $_[1]->{command} . " " .
                                                        join(" ", @{$_[1]->{params}})) });

    return $self;
}

sub nick {
    my $self = shift;
    return $self->conn->nick;
}

sub getSocket
{
    my $self = shift;
    return $self->conn->socket;
}

sub me {
    my ($self, $to, $msg) = @_;
    $self->conn->send_msg('privmsg', $to,
                          encode_ctcp(['ACTION', $msg]))
}

################################################################################
############################### IRC callbacks ##################################
################################################################################

sub new_message {
    my $self = shift;
    my $evt = shift;
    my %args = (
        type        => 'IRC',
        server      => $self->server,
        network     => $self->alias,
        @_
       );
    if ($evt) {
        my ($nick, $user, $host) = split_prefix($evt);
        $args{sender}   ||= $nick;
        $args{hostname} ||= $host if defined($host);
        $args{from}     ||= $evt->{prefix};
        $args{params}   ||= join(' ', @{$evt->{params}})
    }
    return BarnOwl::Message->new(%args);
}

sub on_msg {
    my ($self, $recipient, $evt) = @_;
    my $body = strip_irc_formatting($evt->{params}->[1]);
    $self->handle_message($recipient, $evt, $body);
}

sub on_ctcp_action {
    my ($self, $src, $target, $msg) = @_;
    my $body = strip_irc_formatting($msg);
    my $evt = {
        params => [$src],
        type   => 'privmsg',
        prefix => $src
       };
    $self->handle_message($target, $evt, "* $body");
}

sub handle_message {
    my ($self, $recipient, $evt, $body) = @_;
    my $msg = $self->new_message($evt,
        direction   => 'in',
        recipient   => $recipient,
        body        => $body,
        ($evt->{command}||'') eq 'notice' ?
          (notice     => 'true') : (),
        is_private($recipient) ?
          (private  => 'true') : (channel => $recipient),
        replycmd    => BarnOwl::quote('irc-msg', '-a', $self->alias,
           (is_private($recipient) ? prefix_nick($evt) : $recipient)),
        replysendercmd => BarnOwl::quote('irc-msg', '-a', $self->alias, prefix_nick($evt)),
       );

    BarnOwl::queue_message($msg);
}


sub on_admin_msg {
    my ($self, $evt) = @_;
    return if BarnOwl::Module::IRC->skip_msg($evt->{command});
    BarnOwl::admin_message("IRC",
            BarnOwl::Style::boldify('IRC ' . $evt->{command} . ' message from '
                . $self->alias) . "\n"
            . strip_irc_formatting(join ' ', cdr($evt->{params})));
}

sub on_motdstart {
    my ($self, $evt) = @_;
    $self->motd(join "\n", cdr(@{$evt->{params}}));
}

sub on_motd {
    my ($self, $evt) = @_;
    $self->motd(join "\n", $self->motd, cdr(@{$evt->{params}}));
}

sub on_endofmotd {
    my ($self, $evt) = @_;
    $self->motd(join "\n", $self->motd, cdr(@{$evt->{params}}));
    BarnOwl::admin_message("IRC",
            BarnOwl::Style::boldify('MOTD for ' . $self->alias) . "\n"
            . strip_irc_formatting($self->motd));
}

sub on_join {
    my ($self, $evt) = @_;
    my $chan = $evt->{params}[0];
    my $msg = $self->new_message($evt,
        loginout   => 'login',
        action     => 'join',
        channel    => $chan,
        replycmd   => BarnOwl::quote('irc-msg', '-a', $self->alias, $chan),
        replysendercmd => BarnOwl::quote('irc-msg', '-a', $self->alias, prefix_nick($evt)),
        );
    BarnOwl::queue_message($msg);
}

sub on_part {
    my ($self, $evt) = @_;
    my $chan = $evt->{params}[0];
    my $msg = $self->new_message($evt,
        loginout   => 'logout',
        action     => 'part',
        channel    => $chan,
        replycmd   => BarnOwl::quote('irc-msg', '-a', $self->alias, $chan),
        replysendercmd => BarnOwl::quote('irc-msg', '-a', $self->alias, prefix_nick($evt)),
        );
    BarnOwl::queue_message($msg);
}

sub on_quit {
    my ($self, $evt) = @_;
    my $msg = $self->new_message($evt,
        loginout   => 'logout',
        action     => 'quit',
        from       => $evt->{prefix},
        reason     => $evt->{params}->[0],
        replycmd   => BarnOwl::quote('irc-msg', '-a', $self->alias, prefix_nick($evt)),
        replysendercmd => BarnOwl::quote('irc-msg', '-a', $self->alias, prefix_nick($evt)),
        );
    BarnOwl::queue_message($msg);
}

sub disconnect {
    my $self = shift;
    $self->conn->disconnect;
}

sub on_disconnect {
    my ($self, $why) = @_;
    BarnOwl::admin_message('IRC',
                           "[" . $self->alias . "] Disconnected from server: $why");
    $self->motd("");
    if (!$self->did_quit) {
        $self->schedule_reconnect;
    } else {
        delete $BarnOwl::Module::IRC::ircnets{$self->alias};
    }
}

sub on_error {
    my ($self, $evt) = @_;
    BarnOwl::admin_message('IRC',
                           "[" . $self->alias . "] " .
                           "Error: " . join(" ", @{$evt->{params}}));
}

sub on_nickinuse {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC",
                           "[" . $self->alias . "] " .
                           $evt->{params}->[1] . ": Nick already in use");
}

sub on_nick {
    my ($self, $old_nick, $new_nick, $is_me) = @_;
    if ($is_me) {
        BarnOwl::admin_message("IRC",
                               "[" . $self->alias . "] " .
                               "You are now known as $new_nick");
    } else {
        my $msg = $self->new_message('',
            loginout   => 'login',
            action     => 'nick change',
            from       => $new_nick,
            sender     => $new_nick,
            replycmd   => BarnOwl::quote('irc-msg', '-a', $self->alias,
                                         $new_nick),
            replysendercmd => BarnOwl::quote('irc-msg', '-a', $self->alias,
                                             $new_nick),
            old_nick   => $old_nick);
        BarnOwl::queue_message($msg);
    }
}

sub on_topic {
    my ($self, $channel, $topic, $who) = @_;
    if ($channel) {
        BarnOwl::admin_message("IRC",
                "Topic for $channel on " . $self->alias . " is $topic");
    } else {
        BarnOwl::admin_message("IRC",
                "Topic changed to $channel");
    }
}

sub on_topicinfo {
    my ($self, $evt) = @_;
    my @args = @{$evt->{params}};
    BarnOwl::admin_message("IRC",
        "Topic for $args[1] set by $args[2] at " . localtime($args[3]));
}

# IRC gives us a bunch of namreply messages, followed by an endofnames.
# We need to collect them from the namreply and wait for the endofnames message.
# After this happens, the names_tmp variable is cleared.

sub on_namreply {
    my ($self, $evt) = @_;
    return unless $self->names_tmp;
    $self->names_tmp([@{$self->names_tmp},
                      map {prefix_nick($_)} split(' ', $evt->{params}[3])]);
}

sub cmp_user {
    my ($lhs, $rhs) = @_;
    my ($sigil_l) = ($lhs =~ m{^([+@]?)});
    my ($sigil_r) = ($rhs =~ m{^([+@]?)});
    my %rank = ('@' => 1, '+' => 2, '' => 3);
    return ($rank{$sigil_l} <=> $rank{$sigil_r}) ||
            $lhs cmp $rhs;
}

sub on_endofnames {
    my ($self, $evt) = @_;
    return unless $self->names_tmp;
    my $names = BarnOwl::Style::boldify("Members of " . $evt->{params}->[1] . ":\n");
    for my $name (sort {cmp_user($a, $b)} @{$self->names_tmp}) {
        $names .= "  $name\n";
    }
    BarnOwl::popless_ztext($names);
    $self->names_tmp(0);
}

sub on_whois {
    my ($self, $evt) = @_;
    my %names = (
        311 => 'user',
        312 => 'server',
        319 => 'channels',
        330 => 'whowas',
       );
    $self->whois_tmp(
        $self->whois_tmp . "\n" . $names{$evt->{command}} . ":\n  " .
        join("\n  ", cdr(cdr(@{$evt->{params}}))) . "\n"
       );
}

sub on_endofwhois {
    my ($self, $evt) = @_;
    BarnOwl::popless_ztext(
        BarnOwl::Style::boldify("/whois for " . $evt->{params}->[1] . ":\n") .
        $self->whois_tmp
    );
    $self->whois_tmp('');
}

sub on_mode {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC",
                           "[" . $self->alias . "] User " . (prefix_nick($evt)) . + " set mode " .
                           join(" ", cdr(@{$evt->{params}})) . " on " . $evt->{params}->[0]
                          );
}

sub on_nosuch {
    my ($self, $evt) = @_;
    my %things = (401 => 'nick', 402 => 'server', 403 => 'channel');
    BarnOwl::admin_message("IRC",
                           "[" . $self->alias . "] " .
                           "No such @{[$things{$evt->{command}}]}: @{[$evt->{params}->[1]]}")
}

sub on_event {
    my ($self, $evt) = @_;
    return on_whois(@_) if ($evt->type =~ /^whois/);
    BarnOwl::admin_message("IRC",
            "[" . $self->alias . "] Unhandled IRC event of type " . $evt->type . ":\n"
            . strip_irc_formatting(join("\n", $evt->args)))
        if BarnOwl::getvar('irc:spew') eq 'on';
}

sub schedule_reconnect {
    my $self = shift;
    my $interval = $self->backoff;
    if ($interval) {
        $interval *= 2;
        $interval = 60*5 if $interval > 60*5;
    } else {
        $interval = 5;
    }
    $self->backoff($interval);

    my $weak = $self;
    weaken($weak);
    if (defined $self->{reconnect_timer}) {
        $self->{reconnect_timer}->stop;
    }
    $self->{reconnect_timer} =
        BarnOwl::Timer->new( {
            name  => 'IRC (' . $self->alias . ') reconnect_timer',
            after => $interval,
            cb    => sub {
                $weak->reconnect( $interval ) if $weak;
            },
        } );
}

sub cancel_reconnect {
    my $self = shift;

    if (defined $self->{reconnect_timer}) {
        $self->{reconnect_timer}->stop;
    }
    delete $self->{reconnect_timer};
}

sub on_connect {
    my $self = shift;
    $self->connected("Connected to " . $self->alias . " as " . $self->nick)
}

sub connected {
    my $self = shift;
    my $msg = shift;
    BarnOwl::admin_message("IRC", $msg);
    $self->cancel_reconnect;
    if ($self->autoconnect_channels) {
        for my $c (@{$self->autoconnect_channels}) {
            $self->conn->send_msg(join => $c);
        }
        $self->autoconnect_channels([]);
    }
    $self->conn->enable_ping(60, sub {
                                 $self->on_disconnect("Connection timed out.");
                                 $self->schedule_reconnect;
                             });
    $self->backoff(0);
}

sub reconnect {
    my $self = shift;
    my $backoff = $self->backoff;

    $self->autoconnect_channels([keys(%{$self->{channel_list}})]);
    $self->conn->connect(@{$self->connect_args});
}

################################################################################
########################### Utilities/Helpers ##################################
################################################################################

sub strip_irc_formatting {
    my $body = shift;
    # Strip mIRC colors. If someone wants to write code to convert
    # these to zephyr colors, be my guest.
    $body =~ s/\cC\d+(?:,\d+)?//g;
    $body =~ s/\cO//g;

    my @pieces = split /\cB/, $body;
    my $out = '';
    while(@pieces) {
        $out .= shift @pieces;
        $out .= BarnOwl::Style::boldify(shift @pieces) if @pieces;
    }
    return $out;
}

# Determines if the given message recipient is a username, as opposed to
# a channel that starts with # or &.
sub is_private {
    return shift !~ /^[\#\&]/;
}

sub cdr {
    shift;
    return @_;
}

1;
