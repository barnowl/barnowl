use strict;
use warnings;
package BarnOwl::AnyEvent;
use BarnOwl::Timer;

sub io {
    my ($class, %arg) = @_;
    my $fd = fileno($arg{fh});
    my $mode = $arg{poll};
    my $cb = $arg{cb};
    my $self = {cb => $cb,
                mode => $mode,
                fd => $fd };
    bless($self, class);
    $self->{dispatch} = BarnOwl::add_io_dispatch($fd, $mode, $cb);
    return $self;
}

sub BarnOwl::AnyEvent::io::DESTROY
{
    my ($self) = @_;
    BarnOwl::remove_io_dispatch($self->{fd});
}

sub timer {
    my ($class, %arg) = @_;
    return BarnOwl::Timer->new(%args);
}

# sub idle {
#     my ($class, %arg) = @_;
#     my $cb = $arg{cb};
# }

# sub child {
#     my ($class, %arg) = @_;
#     my $cb = $arg{cb};
# }

1;
