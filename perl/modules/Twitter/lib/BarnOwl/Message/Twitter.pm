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
    }
    # Roughly, this is how Twitter's @-reply calculation works
    # (based on a few experiments)
    #
    #   1. The person who wrote the tweet you are replying to is added.
    #      This is the only time when your own name can show up in an
    #      at-reply string.
    #   2. Next, Twitter goes through the Tweet front to back, adding
    #      mentioned usernames that have not already been added to the
    #      string.
    #
    # In degenerate cases, the at-reply string might be longer than
    # the max Tweet size; Twitter doesn't care.

    # XXX A horrifying violation of encapsulation
    # NB: names array includes @-signs.
    my $account = BarnOwl::Module::Twitter::find_account_default($self->{account});
    my @inside_names = grep($_ ne ("@" . $account->{cfg}->{user}),
                            $self->{body} =~ /(?:^|\s)(@\w+)/g );
    my @dup_names = ( ( "@" . $self->sender ), @inside_names );

    # XXX Really should use List::MoreUtils qw(uniq).  This code snippet
    # lifted from `perldoc -q duplicate`.
    my %seen = ();
    my @names = grep { ! $seen{ $_ }++ } @dup_names;
    my $prefill = join(' ', @names) . ' '; # NB need trailing space

    if(exists($self->{status_id})) {
        return BarnOwl::quote('twitter-prefill', $prefill, $self->{status_id}, $self->account);
    } else {
        # Give a dummy status ID
        return BarnOwl::quote('twitter-prefill', $prefill, '', $self->account);
    }
}

sub replysendercmd {
    my $self = shift;
    return BarnOwl::quote('twitter-direct', $self->sender, $self->account);
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
