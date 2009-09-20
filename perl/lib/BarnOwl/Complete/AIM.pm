use strict;
use warnings;

package BarnOwl::Complete::AIM;

our %users;

sub complete_user { keys %users }

sub canonicalize_aim {
    my $who = shift;
    $who =~ s{ }{}g;
    return lc $who;
}

sub complete_aimwrite {
    my $ctx = shift;
    return unless $ctx->word == 1;
    return complete_user();
}

sub on_message {
    my $m = shift;
    return unless $m->type eq 'AIM';
    $users{canonicalize_aim($m->sender)} = 1;
    $users{canonicalize_aim($m->recipient)} = 1;
}

BarnOwl::Completion::register_completer(aimwrite => \&complete_aimwrite);

$BarnOwl::Hooks::newMessage->add("BarnOwl::Complete::AIM::on_message");


1;
