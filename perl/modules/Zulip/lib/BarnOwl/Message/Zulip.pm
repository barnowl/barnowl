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

sub class       { return shift->{"class"}; }
sub instance    { return shift->{"instance"}; }
sub realm       { return shift->{"realm"}; }
sub opcode       { return shift->{"opcode"}; }
sub long_sender        { return shift->{"sender_full_name"}; }
sub zid { return shift->{zid}; }

sub replycmd {
    my $self = shift;
    if ($self->is_private) {
	return $self->replysendercmd;
    } else {
        return BarnOwl::quote("zulip:write", "-c", $self->class,
                              "-i", $self->instance);
    }
}

sub replysendercmd {
    my $self = shift;
    return BarnOwl::quote("zulip:write", $self->sender);
}

1;
