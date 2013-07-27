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
              qw{( ( direction ^in$ and sender}, "^\Q$who\E\$",
              qw{ ) or ( direction ^out$ and recipient}, "^\Q$who\E\$",
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
                 (qw{type ^irc$ and network}, "^\Q$network\E\$",
                  qw{and channel}, "^\Q$channel\E\$",
                  qw{and ( sender}, "^\Q$sender\E\$",
                  qw{or sender}, "^\Q$1\E\$",qq{)});
        } else {
            $filter = "irc-$network-channel-$channel";
            @filter = (qw{type ^irc$ and network}, "^\Q$network\E\$",
                       qw{and channel}, "^\Q$channel\E\$");
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
sub old_nick {shift->{old_nick}};

# display
sub context {shift->{network};}
sub subcontext {shift->{recipient};}
sub personal_context {"on " . shift->{network};}
sub short_personal_context { shift->{network};}

sub long_sender {shift->{from} || ""};

sub login_type {
    my $self = shift;
    return " (" . uc $self->action . ")";
}

sub login_extra {
    my $self = shift;
    if ($self->action eq "quit") {
        return $self->reason;
    } elsif ($self->action eq 'nick change') {
        return "was: " . $self->old_nick;
    } else {
        return $self->channel;
    }
}

# logging
sub log_filenames {
    my ($m) = @_;
    die "IRC should not be handling non-IRC messages" if lc($m->type) ne "irc";
    BarnOwl::error("IRC message without a network") if !defined($m->network) || $m->network eq '';
    my $filename = lc($m->network);
    if ($m->is_personal) {
        if ($m->is_incoming) {
            $filename .= ":" . $m->sender;
        } elsif ($m->is_outgoing) {
            $filename .= ":" . $m->recipient;
        }
    } else {
        $filename .= ":" . $m->channel;
    }
    return ($filename);
}

sub log {
    my ($m) = @_;
    my $sender = $m->sender;
    my $timestr = $m->time;
    my $body = $m->body;
    if ($m->is_loginout) {
        return BarnOwl::Message::log($m);
    } else {
        return "[$timestr] <$sender> $body\n";
    }
}

1;
