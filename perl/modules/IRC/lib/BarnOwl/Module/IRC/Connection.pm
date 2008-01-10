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
__PACKAGE__->mk_accessors(qw(alias channels));
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
    bless($self, $class);

    $self->add_global_handler(376 => sub { goto &on_connect });
    $self->add_global_handler(['msg', 'notice', 'public', 'caction'],
            sub { goto &on_msg });
    $self->add_global_handler(cping => sub { goto &on_ping });
    $self->add_global_handler(join  => sub { goto &on_join });
    $self->add_global_handler(part  => sub { goto &on_part });
    $self->add_default_handler(sub { goto &on_event; });

    return $self;
}

################################################################################
############################### IRC callbacks ##################################
################################################################################

sub on_connect {
    my ($self, $evt) = @_;
    BarnOwl::admin_message("IRC", "Connected to " . $self->server . " (" . $self->alias . ")");
}

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
    my @pieces = split /\x02/, $body;
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

1;
