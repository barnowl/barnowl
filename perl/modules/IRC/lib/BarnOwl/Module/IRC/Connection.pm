use strict;
use warnings;

package BarnOwl::Module::IRC::Connection;

=head1 NAME

BarnOwl::Module::IRC::Connection

=head1 DESCRIPTION

This module is a Net::IRC::Connection subclass for BarnOwl's IRC
support

=cut

use base qw(Net::IRC::Connection Class::Accessor Exporter);
__PACKAGE__->mk_accessors(qw(alias channels owl_connected owl_motd));
our @EXPORT_OK = qw(&is_private);

use BarnOwl;

sub new {
    my $class = shift;
    my $irc = shift;
    my $alias = shift;
    my %args = (@_);
    my $self = $class->SUPER::new($irc, %args);
    $self->alias($alias);
    $self->channels([]);
    $self->owl_motd("");
    $self->owl_connected(0);
    bless($self, $class);

    $self->add_default_handler(sub { goto &on_event; });
    $self->add_handler(['msg', 'notice', 'public', 'caction'],
            sub { goto &on_msg });
    $self->add_handler(['welcome', 'yourhost', 'created',
            'luserclient', 'luserop', 'luserchannels', 'luserme'],
            sub { goto &on_admin_msg });
    $self->add_handler(['myinfo', 'map', 'n_local', 'n_global',
            'luserconns'],
            sub { });
    $self->add_handler(motdstart => sub { goto &on_motdstart });
    $self->add_handler(motd      => sub { goto &on_motd });
    $self->add_handler(endofmotd => sub { goto &on_endofmotd });
    $self->add_handler(join      => sub { goto &on_join });
    $self->add_handler(part      => sub { goto &on_part });
    $self->add_handler(disconnect => sub { goto &on_disconnect });
    $self->add_handler(nicknameinuse => sub { goto &on_nickinuse });
    $self->add_handler(cping     => sub { goto &on_ping });

    return $self;
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
    BarnOwl::beep() if $body =~ /\b\Q$nick\E\b/;
    $body = BarnOwl::Style::boldify($evt->nick.' '.$body) if $evt->type eq 'caction';
    my $msg = $self->new_message($evt,
        direction   => 'in',
        recipient   => $recipient,
        body => $body,
        $evt->type eq 'notice' ?
          (notice     => 'true') : (),
        is_private($recipient) ?
          (isprivate  => 'true') : (channel => $recipient),
        replycmd    => 'irc-msg ' .
            (is_private($recipient) ? $evt->nick : $recipient),
        replysendercmd => 'irc-msg ' . $evt->nick
       );

    BarnOwl::queue_message($msg);
}

sub on_ping {
    my ($self, $evt) = @_;
    $self->ctcp_reply($evt->nick, join (' ', ($evt->args)));
}

sub on_admin_msg {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC",
            BarnOwl::Style::boldify('IRC ' . $evt->type . ' message from '
                . $evt->alias) . "\n"
            . strip_irc_formatting(join '\n', cdr $evt->args));
}

sub on_motdstart {
    my ($self, $evt) = @_;
    $self->owl_motd(join "\n", cdr $evt->args);
}

sub on_motd {
    my ($self, $evt) = @_;
    $self->owl_motd(join "\n", $self->owl_motd, cdr $evt->args);
}

sub on_endofmotd {
    my ($self, $evt) = @_;
    $self->owl_motd(join "\n", $self->owl_motd, cdr $evt->args);
    if(!$self->owl_connected) {
        BarnOwl::admin_message("IRC", "Connected to " .
                               $self->server . " (" . $self->alias . ")");
        $self->owl_connected(1);
        
    }
    BarnOwl::admin_message("IRC",
            BarnOwl::Style::boldify('MOTD for ' . $self->alias) . "\n"
            . strip_irc_formatting($self->owl_motd));
}

sub on_join {
    my ($self, $evt) = @_;
    my $msg = $self->new_message($evt,
        loginout   => 'login',
        channel    => $evt->to,
        );
    BarnOwl::queue_message($msg);
}

sub on_part {
    my ($self, $evt) = @_;
    my $msg = $self->new_message($evt,
        loginout   => 'logout',
        channel    => $evt->to,
        );
    BarnOwl::queue_message($msg);
}

sub on_disconnect {
    my $self = shift;
    delete $BarnOwl::Module::IRC::ircnets{$self->alias};

    BarnOwl::admin_message('IRC',
                           "[" . $self->alias . "] Disconnected from server");
}

sub on_nickinuse {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC",
                           "[" . $self->alias . "] " .
                           [$evt->args]->[1] . ": Nick already in use");
    unless($self->owl_connected) {
        $self->disconnect;
    }
}

sub on_event {
    my ($self, $evt) = @_;
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
     my $out;
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
