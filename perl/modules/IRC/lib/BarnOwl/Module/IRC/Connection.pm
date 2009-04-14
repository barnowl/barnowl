use strict;
use warnings;

package BarnOwl::Module::IRC::Connection;

=head1 NAME

BarnOwl::Module::IRC::Connection

=head1 DESCRIPTION

This module is a wrapper around Net::IRC::Connection for BarnOwl's IRC
support

=cut

use Net::IRC::Connection;

use base qw(Class::Accessor Exporter);
__PACKAGE__->mk_accessors(qw(conn alias channels connected motd names_tmp whois_tmp));
our @EXPORT_OK = qw(&is_private);

use BarnOwl;

BEGIN {
    no strict 'refs';
    my @delegate = qw(nick server);
    for my $meth (@delegate) {
        *{"BarnOwl::Module::IRC::Connection::$meth"} = sub {
            shift->conn->$meth(@_);
        }
    }
};

sub new {
    my $class = shift;
    my $irc = shift;
    my $alias = shift;
    my %args = (@_);
    my $conn = Net::IRC::Connection->new($irc, %args);
    my $self = bless({}, $class);
    $self->conn($conn);
    $self->alias($alias);
    $self->channels([]);
    $self->motd("");
    $self->connected(0);
    $self->names_tmp(0);
    $self->whois_tmp("");

    $self->conn->add_handler(376 => sub { shift; $self->on_connect(@_) });
    $self->conn->add_default_handler(sub { shift; $self->on_event(@_) });
    $self->conn->add_handler(['msg', 'notice', 'public', 'caction'],
            sub { shift; $self->on_msg(@_) });
    $self->conn->add_handler(['welcome', 'yourhost', 'created',
                              'luserclient', 'luserop', 'luserchannels', 'luserme',
                              'error'],
            sub { shift; $self->on_admin_msg(@_) });
    $self->conn->add_handler(['myinfo', 'map', 'n_local', 'n_global',
            'luserconns'],
            sub { });
    $self->conn->add_handler(motdstart => sub { shift; $self->on_motdstart(@_) });
    $self->conn->add_handler(motd      => sub { shift; $self->on_motd(@_) });
    $self->conn->add_handler(endofmotd => sub { shift; $self->on_endofmotd(@_) });
    $self->conn->add_handler(join      => sub { shift; $self->on_join(@_) });
    $self->conn->add_handler(part      => sub { shift; $self->on_part(@_) });
    $self->conn->add_handler(quit      => sub { shift; $self->on_quit(@_) });
    $self->conn->add_handler(disconnect => sub { shift; $self->on_disconnect(@_) });
    $self->conn->add_handler(nicknameinuse => sub { shift; $self->on_nickinuse(@_) });
    $self->conn->add_handler(cping     => sub { shift; $self->on_ping(@_) });
    $self->conn->add_handler(topic     => sub { shift; $self->on_topic(@_) });
    $self->conn->add_handler(topicinfo => sub { shift; $self->on_topicinfo(@_) });
    $self->conn->add_handler(namreply  => sub { shift; $self->on_namreply(@_) });
    $self->conn->add_handler(endofnames=> sub { shift; $self->on_endofnames(@_) });
    $self->conn->add_handler(endofwhois=> sub { shift; $self->on_endofwhois(@_) });
    $self->conn->add_handler(mode      => sub { shift; $self->on_mode(@_) });

    # * nosuchchannel
    # * 

    return $self;
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
    return BarnOwl::Message->new(
        type        => 'IRC',
        server      => $self->server,
        network     => $self->alias,
        sender      => $evt->nick,
        hostname    => $evt->host,
        from        => $evt->from,
        @_
       );
}

sub on_msg {
    my ($self, $evt) = @_;
    my ($recipient) = $evt->to;
    my $body = strip_irc_formatting([$evt->args]->[0]);
    my $nick = $self->nick;
    $body = '* '.$evt->nick.' '.$body if $evt->type eq 'caction';
    my $msg = $self->new_message($evt,
        direction   => 'in',
        recipient   => $recipient,
        body => $body,
        $evt->type eq 'notice' ?
          (notice     => 'true') : (),
        is_private($recipient) ?
          (isprivate  => 'true') : (channel => $recipient),
        replycmd    => 'irc-msg -a ' . $self->alias . ' ' .
            (is_private($recipient) ? $evt->nick : $recipient),
        replysendercmd => 'irc-msg -a ' . $self->alias . ' ' . $evt->nick
       );

    BarnOwl::queue_message($msg);
}

sub on_ping {
    my ($self, $evt) = @_;
    $self->conn->ctcp_reply($evt->nick, join (' ', ($evt->args)));
}

sub on_admin_msg {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC",
            BarnOwl::Style::boldify('IRC ' . $evt->type . ' message from '
                . $self->alias) . "\n"
            . strip_irc_formatting(join ' ', cdr($evt->args)));
}

sub on_motdstart {
    my ($self, $evt) = @_;
    $self->motd(join "\n", cdr($evt->args));
}

sub on_motd {
    my ($self, $evt) = @_;
    $self->motd(join "\n", $self->motd, cdr($evt->args));
}

sub on_endofmotd {
    my ($self, $evt) = @_;
    $self->motd(join "\n", $self->motd, cdr($evt->args));
    if(!$self->connected) {
        BarnOwl::admin_message("IRC", "Connected to " .
                               $self->server . " (" . $self->alias . ")");
        $self->connected(1);
        
    }
    BarnOwl::admin_message("IRC",
            BarnOwl::Style::boldify('MOTD for ' . $self->alias) . "\n"
            . strip_irc_formatting($self->motd));
}

sub on_join {
    my ($self, $evt) = @_;
    my $msg = $self->new_message($evt,
        loginout   => 'login',
        action     => 'join',
        channel    => $evt->to,
        replycmd => 'irc-msg -a ' . $self->alias . ' ' . join(' ', $evt->to),
        replysendercmd => 'irc-msg -a ' . $self->alias . ' ' . $evt->nick
        );
    BarnOwl::queue_message($msg);
}

sub on_part {
    my ($self, $evt) = @_;
    my $msg = $self->new_message($evt,
        loginout   => 'logout',
        action     => 'part',
        channel    => $evt->to,
        replycmd => 'irc-msg -a ' . $self->alias . ' ' . join(' ', $evt->to),
        replysendercmd => 'irc-msg -a ' . $self->alias . ' ' . $evt->nick
        );
    BarnOwl::queue_message($msg);
}

sub on_quit {
    my ($self, $evt) = @_;
    my $msg = $self->new_message($evt,
        loginout   => 'logout',
        action     => 'quit',
        from       => $evt->to,
        reason     => [$evt->args]->[0],
        replycmd => 'irc-msg -a ' . $self->alias . ' ' . $evt->nick,
        replysendercmd => 'irc-msg -a ' . $self->alias . ' ' . $evt->nick
        );
    BarnOwl::queue_message($msg);
}

sub on_disconnect {
    my $self = shift;
    delete $BarnOwl::Module::IRC::ircnets{$self->alias};
    BarnOwl::remove_dispatch($self->{FD});
    BarnOwl::admin_message('IRC',
                           "[" . $self->alias . "] Disconnected from server");
}

sub on_nickinuse {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC",
                           "[" . $self->alias . "] " .
                           [$evt->args]->[1] . ": Nick already in use");
    unless($self->connected) {
        $self->conn->disconnect;
    }
}

sub on_topic {
    my ($self, $evt) = @_;
    my @args = $evt->args;
    if (scalar @args > 1) {
        BarnOwl::admin_message("IRC",
                "Topic for $args[1] on " . $self->alias . " is $args[2]");
    } else {
        BarnOwl::admin_message("IRC",
                "Topic changed to $args[0]");
    }
}

sub on_topicinfo {
    my ($self, $evt) = @_;
    my @args = $evt->args;
    BarnOwl::admin_message("IRC",
        "Topic for $args[1] set by $args[2] at " . localtime($args[3]));
}

# IRC gives us a bunch of namreply messages, followed by an endofnames.
# We need to collect them from the namreply and wait for the endofnames message.
# After this happens, the names_tmp variable is cleared.

sub on_namreply {
    my ($self, $evt) = @_;
    return unless $self->names_tmp;
    $self->names_tmp([@{$self->names_tmp}, split(' ', [$evt->args]->[3])]);
}

sub on_endofnames {
    my ($self, $evt) = @_;
    return unless $self->names_tmp;
    my $names = BarnOwl::Style::boldify("Members of " . [$evt->args]->[1] . ":\n");
    for my $name (@{$self->names_tmp}) {
        $names .= "  $name\n";
    }
    BarnOwl::popless_ztext($names);
    $self->names_tmp(0);
}

sub on_whois {
    my ($self, $evt) = @_;
    $self->whois_tmp(
      $self->whois_tmp . "\n" . $evt->type . ":\n  " .
      join("\n  ", cdr(cdr($evt->args))) . "\n"
    );
}

sub on_endofwhois {
    my ($self, $evt) = @_;
    BarnOwl::popless_ztext(
        BarnOwl::Style::boldify("/whois for " . [$evt->args]->[1] . ":\n") .
        $self->whois_tmp
    );
    $self->whois_tmp([]);
}

sub on_mode {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC",
                           "[" . $self->alias . "] User " . ($evt->nick) . + " set mode " .
                           join(" ", $evt->args) . "on " . $evt->to->[0]
                          );
}

sub on_event {
    my ($self, $evt) = @_;
    return on_whois(@_) if ($evt->type =~ /^whois/);
    BarnOwl::admin_message("IRC",
            "[" . $self->alias . "] Unhandled IRC event of type " . $evt->type . ":\n"
            . strip_irc_formatting(join("\n", $evt->args)))
        if BarnOwl::getvar('irc:spew') eq 'on';
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
