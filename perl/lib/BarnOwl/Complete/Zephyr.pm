use strict;
use warnings;

package BarnOwl::Complete::Zephyr;

use BarnOwl::Completion::Util qw(complete_flags);

my %classes = ();
my %realms = map {$_ => 1} qw(ATHENA.MIT.EDU ANDREW.CMU.EDU ZONE.MIT.EDU);
my %users = ();

sub complete_class    { keys %classes }
sub complete_instance { }
sub complete_realm    { keys %realms }
sub complete_opcode   { }
sub complete_user     { keys %users; }

sub complete_zwrite {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw(-n -C -m)],
        {
            "-c" => \&complete_class,
            "-i" => \&complete_instance,
            "-r" => \&complete_realm,
            "-O" => \&complete_opcode,
        },
        \&complete_user
       );
}

sub complete_sub {
    my $ctx = shift;
    return complete_flags($ctx,
        [],
        {
            "-r" => \&complete_realm,
        });
}

sub on_message {
    my $m = shift;
    return unless $m->type eq 'zephyr';
    $classes{$m->class} = 1;
    $realms{$m->realm} = 1;
    $users{BarnOwl::Message::Zephyr::strip_realm($m->sender)} = 1;
}

BarnOwl::Completion::register_completer(zwrite    => \&complete_zwrite);
BarnOwl::Completion::register_completer(zcrypt    => \&complete_zwrite);

# TODO: Handle aliases transparently in the core completion code?
BarnOwl::Completion::register_completer(sub       => \&complete_sub);
BarnOwl::Completion::register_completer(subscribe => \&complete_sub);

$BarnOwl::Hooks::newMessage->add("BarnOwl::Complete::Zephyr::on_message");

1;
