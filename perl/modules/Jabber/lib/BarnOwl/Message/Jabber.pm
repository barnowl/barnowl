use warnings;
use strict;

=head1 NAME

BarnOwl::Message::Jabber

=head1 DESCRIPTION

A subclass of BarnOwl::Message for Jabber messages

=cut

package BarnOwl::Message::Jabber;

use base qw( BarnOwl::Message );

sub jtype { shift->{jtype} };
sub from { shift->{from} };
sub to { shift->{to} };
sub room { shift->{room} };
sub nick { shift->{nick} };
sub subject { shift->{subject} };
sub status { shift->{status} }

sub login_type {
    my $self = shift;
    my $type = $self->jtype;
    return " ($type)" if $type;
}

sub login_extra {
    my $self = shift;
    my $show = $self->{show};
    my $status = $self->status;
    my $s = "";
    $s .= $show if $show;
    $s .= ", $status" if $status;
    return $s;
}

sub long_sender {
    my $self = shift;
    if ($self->jtype eq 'groupchat' && $self->nick) {
        my $from_jid = Net::Jabber::JID->new($self->from);
        if ($from_jid->GetJID('base') eq $self->room &&
            $from_jid->GetResource() eq $self->nick) {
            return $self->nick;
        }
    }
    return $self->from;
}

sub context {
    return shift->room;
}

sub subcontext {
    return shift->subject || "";
}

sub personal_context {
    my $self = shift;
    if($self->subject) {
        return "subject: " . $self->subject;
    } elsif ($self->is_incoming) {
        return "to " . $self->to;
    } else {
        return "from " . $self->from;
    }
}

sub smartfilter {
    my $self = shift;
    my $inst = shift;

    my $filter;

    if($self->jtype eq 'chat') {
        my $user;
        if($self->direction eq 'in') {
            $user = $self->from;
        } else {
            $user = $self->to;
        }
        return smartfilter_user($user, $inst);
    } elsif ($self->jtype eq 'groupchat') {
        my $room = $self->room;
        if ($inst) {
            my $subject = $self->subject;
            $filter = "jabber-room-$room-subject-$subject";
            BarnOwl::command(qw[filter], $filter,
                             qw[type ^jabber$ and room], "^\Q$room\E\$",
                             qw[and subject], "^\Q$subject\E\$");
        } else {
            $filter = "jabber-room-$room";
            BarnOwl::command(qw[filter], $filter,
                             qw[type ^jabber$ and room], "^\Q$room\E\$");
        }
        return $filter;
    } elsif ($self->login ne 'none') {
        return smartfilter_user($self->from, $inst);
    }
}

sub smartfilter_user {
    my $user = shift;
    my $inst = shift;

    $user   = Net::Jabber::JID->new($user)->GetJID( $inst ? 'full' : 'base' );
    my $filter = "jabber-user-$user";
    BarnOwl::command(qw[filter], $filter, qw[type ^jabber$],
                     qw[and ( ( direction ^in$ and from], "^\Q$user\E(/.*)?\$",
                     qw[) or ( direction ^out$ and to ], "^\Q$user\E(/.*)?\$",
                     qw[ ) ) ]);
    return $filter;

}

sub replycmd {
    my $self = shift;
    my ($recip, $account, $subject);
    if ($self->is_loginout) {
        $recip   = $self->sender;
        $account = $self->recipient;
    } elsif ($self->jtype eq 'chat') {
        return $self->replysendercmd;
    } elsif ($self->jtype eq 'groupchat') {
        $recip = $self->room;
        if ($self->is_incoming) {
            $account = $self->to;
        } else {
            $account = $self->from;
        }
        $subject = $self->subject;
    } else {
        # Unknown type
        return;
    }
    return jwrite_cmd($recip, $account, $subject);
}

sub replysendercmd {
    my $self = shift;
    if($self->jtype eq 'groupchat'
       || $self->jtype eq 'chat') {
        my ($recip, $account);
        if ($self->is_incoming) {
            $recip   = $self->from;
            $account = $self->to;
        } else {
            $recip   = $self->to;
            $account = $self->from;
        }
        return jwrite_cmd($recip, $account);
    }
    return $self->replycmd;
}

sub jwrite_cmd {
    my ($recip, $account, $subject) = @_;
    if (defined $recip) {
        my @cmd = ('jwrite', '-a', $account);
        if (defined $subject) {
            push @cmd, '-s', $subject;
        }
        push @cmd, '--', $recip;
        return BarnOwl::quote(@cmd);
    } else {
        return undef;
    }
}

=head1 SEE ALSO

L<BarnOwl::Message>

=cut

1;
