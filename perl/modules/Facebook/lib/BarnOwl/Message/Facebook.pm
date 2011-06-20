use warnings;
use strict;

=head1 NAME

BarnOwl::Message::Facebook

=head1 DESCRIPTION

=cut

package BarnOwl::Message::Facebook;
use base qw(BarnOwl::Message);

sub context { return shift->{"name"}; }
sub subcontext { return shift->{"topic"}; }
sub service { return "http://www.facebook.com"; }
sub long_sender { return shift->{"zsig"}; } # XXX hack, shouldn't be named zsig

sub replycmd {
    my $self = shift;
    # TODO: Support for direct messages
    # XXX: It seems that Facebook may not support this yet
    return BarnOwl::quote('facebook-comment', $self->{post_id});
}

#sub replysendercmd {
#    my $self = shift;
#}

sub smartfilter {
    my $self = shift;
    my $inst = shift;
    my $filter;
    # XXX I hope $filter isn't used for anything besides display purposes
    if($inst) {
        $filter = "facebook: " . $self->{name} . " " . $self->{post_id};
        BarnOwl::command("filter", $filter,
                         qw{type ^facebook$ and name_id}, '^'.$self->{name_id}.'$',
                         qw{and post_id}, '^'.$self->{post_id}.'$');
    } else {
        $filter = "facebook: " . $self->{name};
        BarnOwl::command("filter", $filter,
                         qw{type ^facebook$ and name_id}, '^'.$self->{name_id}.'$');
    }
    return $filter;
}

1;
