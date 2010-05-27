use warnings;
use strict;

=head1 NAME

BarnOwl::Message::Twitter

=head1 DESCRIPTION

=cut

package BarnOwl::Message::Twitter;
use base qw(BarnOwl::Message);

sub context {'twitter'}
sub subcontext {undef}
sub service { return (shift->{"service"} || "http://twitter.com"); }
sub account { return shift->{"account"}; }
sub retweeted_by { shift->{retweeted_by}; }
sub long_sender {
    my $self = shift;
    $self->service =~ m#^\s*(.*?://.*?)/.*$#;
    my $service = $1 || $self->service;
    my $long = $service . '/' . $self->sender;
    if ($self->retweeted_by) {
        $long = "(retweeted by " . $self->retweeted_by . ") $long";
    }
    return $long;
}

sub replycmd {
    my $self = shift;
    if($self->is_private) {
        return $self->replysendercmd;
    } elsif(exists($self->{status_id})) {
        return 'twitter-atreply ' . $self->sender . " " . $self->{status_id} . " " . $self->account;
    } else {
        return 'twitter-atreply ' . $self->sender . " " . $self->account;
    }
}

sub replysendercmd {
    my $self = shift;
    return 'twitter-direct ' . $self->sender . " " . $self->account;
}

sub smartfilter {
    my $self = shift;
    my $inst = shift;
    my $filter;

    if($inst) {
        $filter = "twitter-" . $self->sender;
        BarnOwl::command("filter", $filter,
                         qw{type ^twitter$ and sender}, '^'.$self->sender.'$');
    } else {
        $filter = "twitter";
    }
    return $filter;
}

1;
