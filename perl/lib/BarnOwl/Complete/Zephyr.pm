use strict;
use warnings;

package BarnOwl::Complete::Zephyr;

use BarnOwl::Completion::Util qw(complete_flags);

our %classes = ();
our %realms = map {$_ => 1} qw(ATHENA.MIT.EDU ANDREW.CMU.EDU ZONE.MIT.EDU);
our %users = ();

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

sub complete_viewclass {
    my $ctx = shift;
    return unless $ctx->word == 1;
    return complete_class();
}

sub complete_viewuser {
    my $ctx = shift;
    return unless $ctx->word == 1;
    return complete_user();
}

sub complete_unsub {
    my $ctx = shift;
    return unless $ctx->word == 1;
    return complete_class();
}

sub on_message {
    my $m = shift;
    return unless $m->type eq 'zephyr';
    $classes{lc $m->class} = 1;
    $realms{lc $m->realm} = 1;
    $users{lc BarnOwl::Message::Zephyr::strip_realm($m->sender)} = 1;
}

BarnOwl::Completion::register_completer(zwrite    => \&complete_zwrite);
BarnOwl::Completion::register_completer(zcrypt    => \&complete_zwrite);

# Aliases should really be taken care of by the core completion code.
BarnOwl::Completion::register_completer(viewclass => \&complete_viewclass);
BarnOwl::Completion::register_completer(vc        => \&complete_viewclass);
BarnOwl::Completion::register_completer(viewuser  => \&complete_viewuser);
BarnOwl::Completion::register_completer(vu        => \&complete_viewuser);

BarnOwl::Completion::register_completer(unsub     => \&complete_unsub);

$BarnOwl::Hooks::newMessage->add("BarnOwl::Complete::Zephyr::on_message");

1;
