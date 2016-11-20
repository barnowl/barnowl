use strict;
use warnings;

package BarnOwl::Message::Zulip;
use base qw(BarnOwl::Message);

use BarnOwl::Module::Zulip qw(default_realm);


sub strip_realm {
    my $sender = shift;
    my $realm = BarnOwl::Module::Zulip::default_realm();
    $sender =~ s/\@\Q$realm\E$//;
    return $sender;
}

sub context {
    return shift->class;
}

sub subcontext {
    return shift->instance;
}

sub pretty_sender {
    my ($m) = @_;
    return strip_realm($m->sender);
}

sub pretty_recipient {
    my ($m) = @_;
    return strip_realm($m->recipient);
}

sub private_recipient_list {
    my ($m) = @_;
    return split(/ /, $m->recipient);
}

sub class       { return shift->{"class"}; }
sub instance    { return shift->{"instance"}; }
sub realm       { return shift->{"realm"}; }
sub opcode       { return shift->{"opcode"}; }
sub long_sender        { return shift->{"sender_full_name"}; }
sub zid { return shift->{zid}; }

sub replycmd {
    my $self = shift;
    if ($self->is_private) {
        return $self->replyprivate(1);
    } else {
        return BarnOwl::quote("zulip:write", "-c", $self->class,
                              "-i", $self->instance);
    }
}

sub replysendercmd {
    my $self = shift;
    if ($self->is_private) {
        return $self->replyprivate(0);
    } else {
        return BarnOwl::quote("zulip:write", $self->sender);
    }
}

sub replyprivate {
    # Cases:
    # - me -> me: me
    # - me -> other: other
    # - (all) me -> other1, other2: other1, other2
    # - (not) me -> other1, other2: error
    # - other -> me: other
    # - (all) other1 -> me, other2: other1, other2
    # - (not) other1 -> me, other2: other1
    my $self = shift;
    my $all = shift;
    my @recipients;
    if ($all) {
       @recipients = $self->private_recipient_list;
    } else {
        if ($self->sender eq BarnOwl::Module::Zulip::user()) {
            @recipients = $self->private_recipient_list;
            if (scalar(@recipients) > 1) {
                BarnOwl::error("Cannot reply privately to message to more than one recipient");
                return;
            }
        } else {
            @recipients = $self->sender;
        }
    }
    my @filtered_recipients = grep { $_ ne BarnOwl::Module::Zulip::user() } @recipients;
    if (scalar(@filtered_recipients) == 0) {
        # Self must have been only recipient, so re-add it (one copy)
        @filtered_recipients = @recipients[0];
    }
    return BarnOwl::quote("zulip:write", @filtered_recipients);
}

1;
