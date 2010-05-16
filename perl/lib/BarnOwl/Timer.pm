use strict;
use warnings;

package BarnOwl::Timer;

sub new {
    my $class = shift;
    my $args = shift;

    my $cb = $args->{cb};
    die("Invalid callback pased to BarnOwl::Timer\n") unless ref($cb) eq 'CODE';

    my $self = {cb => $cb};

    bless($self, $class);

    $self->{timer} = BarnOwl::Internal::add_timer($args->{after} || 0,
                                                  $args->{interval} || 0,
                                                  $self);
    return $self;
}

sub stop {
    my $self = shift;
    if(defined($self->{timer})) {
        BarnOwl::Internal::remove_timer($self->{timer});
        undef $self->{timer};
    }
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
