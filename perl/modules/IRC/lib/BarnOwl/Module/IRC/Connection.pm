use strict;
use warnings;

package BarnOwl::Module::IRC::Connection;

=head1 NAME

BarnOwl::Module::IRC::Connection

=head1 DESCRIPTION

This module is a Net::IRC::Connection subclass for BarnOwl's IRC
support

=cut

use base qw(Net::IRC::Connection Class::Accessor);
__PACKAGE__->mk_accessors(qw(alias channels));

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

    $self->add_global_handler(endofmotd => sub { goto &on_connect });
    $self->add_global_handler(msg => sub { goto &on_msg });
    $self->add_global_handler(notice => sub { goto &on_msg });
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

sub on_msg {
    my ($self, $evt) = @_;
    my $replycmd = "irc-msg " . $evt->nick;
    my $msg = BarnOwl::Message->new(
        type        => 'IRC',
        direction   => 'in',
        server      => $self->server,
        network     => $self->alias,
        recipient   => $self->nick,
        body        => strip_irc_formatting([$evt->args]->[0]),
        sender      => $evt->nick,
        hostname    => $evt->host,
        from        => $evt->from,
        notice      => $evt->type eq 'notice' ? 'true' : '',
        isprivate   => 'true',
        replycmd    => $replycmd,
        replysendercmd => $replycmd
       );
    BarnOwl::queue_message($msg);
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


1;
