use strict;
use warnings;

package BarnOwl::Timer;

use AnyEvent;

sub new {
    my $class = shift;
    my $args = shift;

    my $cb = $args->{cb};
    die("Invalid callback pased to BarnOwl::Timer\n") unless ref($cb) eq 'CODE';

    my $self = {cb => $cb};

    bless($self, $class);

    $self->{timer} = AnyEvent->timer(%$args);
    return $self;
}

sub stop {
    my $self = shift;
    undef $self->{timer};
}

sub do_callback {
    my $self = shift;
    $self->{cb}->($self);
}

sub DESTROY {
    my $self = shift;
    $self->stop;
}


1;
