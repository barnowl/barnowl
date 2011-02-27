use strict;
use warnings;

package BarnOwl::Module::IRC::Connection;
use BarnOwl::Timer;

=head1 NAME

BarnOwl::Module::IRC::Connection

=head1 DESCRIPTION

This module is a wrapper around Net::IRC::Connection for BarnOwl's IRC
support

=cut

use AnyEvent::IRC::Client;
use AnyEvent::IRC::Util qw(split_prefix prefix_nick);

use base qw(Class::Accessor);
use Exporter 'import';
__PACKAGE__->mk_accessors(qw(conn alias motd names_tmp whois_tmp server autoconnect_channels));
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
    if(delete $args->{SSL}) {
        $conn->enable_ssl;
    }
    $conn->connect($host, $port, $args);
    my $self = bless({}, $class);
    $self->conn($conn);
    $self->autoconnect_channels([]);
    $self->alias($alias);
    $self->server($host);
    $self->motd("");
    $self->names_tmp(0);
    $self->whois_tmp("");

    # $self->conn->add_default_handler(sub { shift; $self->on_event(@_) });
    $self->conn->reg_cb(registered => sub { shift; $self->connected("Connected to $alias as $nick") },
                        connfail   => sub { shift; BarnOwl::error("Connection to $host failed!") },
                        disconnect => sub { shift; $self->on_disconnect(@_) },
                        publicmsg  => sub { shift; $self->on_msg(@_) },
                        privatemsg => sub { shift; $self->on_msg(@_) });
    for my $m (qw(welcome yourhost created
                  luserclient luserop luserchannels luserme
                  error)) {
        $self->conn->reg_cb("irc_$m" => sub { shift;  $self->on_admin_msg(@_) });
    }
    $self->conn->reg_cb(irc_375       => sub { shift; $self->on_motdstart(@_) },
                        irc_372       => sub { shift; $self->on_motd(@_) },
                        irc_376       => sub { shift; $self->on_endofmotd(@_) },
                        irc_join      => sub { shift; $self->on_join(@_) },
                        irc_part      => sub { shift; $self->on_part(@_) },
                        irc_quit      => sub { shift; $self->on_quit(@_) },
                        irc_433       => sub { shift; $self->on_nickinuse(@_) },
                        channel_topic => sub { shift; $self->on_topic(@_) },
                        irc_333       => sub { shift; $self->on_topicinfo(@_) },
                        irc_353       => sub { shift; $self->on_namreply(@_) },
                        irc_366       => sub { shift; $self->on_endofnames(@_) },
                        irc_311       => sub { shift; $self->on_whois(@_) },
                        irc_312       => sub { shift; $self->on_whois(@_) },
                        irc_319       => sub { shift; $self->on_whois(@_) },
                        irc_320       => sub { shift; $self->on_whois(@_) },
                        irc_318       => sub { shift; $self->on_endofwhois(@_) },
                        irc_mode      => sub { shift; $self->on_mode(@_) },
                        irc_401       => sub { shift; $self->on_nosuch(@_) },
                        irc_402       => sub { shift; $self->on_nosuch(@_) },
                        irc_403       => sub { shift; $self->on_nosuch(@_) },
                        nick_change  => sub { shift; $self->on_nick(@_); },
                        'irc_*' => sub { BarnOwl::debug("IRC: " . $_[1]->{command}) });

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

################################################################################
############################### IRC callbacks ##################################
################################################################################

sub new_message {
    my $self = shift;
    my $evt = shift;
    my ($nick, $user, $host) = split_prefix($evt);
    return BarnOwl::Message->new(
        type        => 'IRC',
        server      => $self->server,
        network     => $self->alias,
        sender      => $nick,
        defined($host) ? (hostname    => $host) : (),
        from        => $evt->{prefix},
        @_
       );
}

sub on_msg {
    my ($self, $recipient, $evt) = @_;
    my $body = strip_irc_formatting($evt->{params}->[1]);

    my $msg = $self->new_message($evt,
        direction   => 'in',
        recipient   => $recipient,
        body        => $body,
        $evt->{command} eq 'notice' ?
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
        reason     => $evt->{params}->[1],
        replycmd   => BarnOwl::quote('irc-msg', '-a', $self->alias, prefix_nick($evt)),
        replysendercmd => BarnOwl::quote('irc-msg', '-a', $self->alias, prefix_nick($evt)),
        );
    BarnOwl::queue_message($msg);
}

sub disconnect {
    my $self = shift;
    delete $BarnOwl::Module::IRC::ircnets{$self->alias};
    for my $k (keys %BarnOwl::Module::IRC::channels) {
        my @conns = grep {$_ ne $self} @{$BarnOwl::Module::IRC::channels{$k}};
        if(@conns) {
            $BarnOwl::Module::IRC::channels{$k} = \@conns;
        } else {
            delete $BarnOwl::Module::IRC::channels{$k};
        }
    }
    $self->motd("");
}

sub on_disconnect {
    my ($self, $why) = @_;
    $self->disconnect;
    BarnOwl::admin_message('IRC',
                           "[" . $self->alias . "] Disconnected from server");
    if ($why && $why =~ m{error in connection}) {
        $self->schedule_reconnect;
    }
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
        BarnOwl::admin_message("IRC",
                               "[" . $self->alias . "] " .
                               "$old_nick is now known as $new_nick");
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
    $self->names_tmp([@{$self->names_tmp}, split(' ', $evt->{params}[3])]);
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
                           join(" ", cdr(@{$evt->{params}})) . "on " . $evt->{params}->[0]
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
    my $interval = shift || 5;
    delete $BarnOwl::Module::IRC::ircnets{$self->alias};
    $BarnOwl::Module::IRC::reconnect{$self->alias} = $self;
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
    delete $BarnOwl::Module::IRC::reconnect{$self->alias};
    if (defined $self->{reconnect_timer}) {
        $self->{reconnect_timer}->stop;
    }
    delete $self->{reconnect_timer};
}

sub connected {
    my $self = shift;
    my $msg = shift;
    BarnOwl::admin_message("IRC", $msg);
    $self->cancel_reconnect;
    $BarnOwl::Module::IRC::ircnets{$self->alias} = $self;
    if ($self->autoconnect_channels) {
        for my $c (@{$self->autoconnect_channels}) {
            $self->conn->send_msg(join => $c);
        }
        $self->autoconnect_channels([]);
    }
    $self->conn->enable_ping(60, sub {
                                 $self->disconnect("Connection timed out.");
                                 $self->schedule_reconnect;
                             });
}

sub reconnect {
    my $self = shift;
    my $backoff = shift;

    $self->autoconnect_channels([keys(%{$self->channel_list})]);
    $self->conn->connect;
    if ($self->conn->connected) {
        $self->connected("Reconnected to ".$self->alias);
        return;
    }

    $backoff *= 2;
    $backoff = 60*5 if $backoff > 60*5;
    $self->schedule_reconnect( $backoff );
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
