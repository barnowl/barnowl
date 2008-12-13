use warnings;
use strict;

=head1 NAME

BarnOwl::Message::IRC

=head1 DESCRIPTION

A subclass of BarnOwl::Message for IRC messages

=cut

package BarnOwl::Message::IRC;

use base qw( BarnOwl::Message );

sub smartfilter {
    my $self = shift;
    my $inst = shift;

    my $filter;
    my @filter;

    if($self->is_private) {
        my $who;
        if($self->direction eq 'out') {
            $who = $self->recipient;
        } else {
            $who = $self->sender;
        }
        $filter = "irc-user-$who";
        @filter  =
             (qw{( type ^irc$ and filter personal and },
              qw{( ( direction ^in$ and sender}, "^$who\$",
              qw{ ) or ( direction ^out$ and recipient}, "^$who\$",
              qw{) ) ) });
        BarnOwl::command("filter", "$filter", @filter);
        return $filter;
    } else {
        # To a Channel
        my $network = $self->network;
        my $channel = $self->channel;
        my $sender = $self->sender;
        my ($filter, $ftext);
        if ($inst && $self->body =~ /^(\S+):/) {
            $filter = "irc-$network-channel-$channel-$sender-$1";
            @filter =
                 (qw{type ^irc$ and network}, "^$network\$",
                  qw{and channel}, "^$channel\$",
                  qw{and ( sender}, "^$sender\$",
                  qw{or sender}, "^$1\$",qq{)});
        } else {
            $filter = "irc-$network-channel-$channel";
            @filter = (qw{type ^irc$ and network}, "^$network\$",
                       qw{and channel}, "^$channel\$");
        }
        BarnOwl::command("filter", "$filter", @filter);
        return $filter;
    }
}

sub server {shift->{server}}
sub network {shift->{network}}
sub channel {shift->{channel}}
sub action {shift->{action}}
sub reason {shift->{reason}}

# display
sub context {shift->{network};}
sub subcontext {shift->{recipient};}

sub long_sender {shift->{from} || ""};

sub login_type {
    my $self = shift;
    return " (" . uc $self->action . ")";
}

sub login_extra { 
    my $self = shift;
    if ($self->action eq "quit") {
        return $self->reason;
    } else {
        return $self->channel;
    }
}

1;
