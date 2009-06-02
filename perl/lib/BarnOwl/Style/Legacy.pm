use strict;
use warnings;

package BarnOwl::Style::Legacy;

sub new {
    my $class = shift;
    my $func  = shift;
    my $desc  = shift;
    my $useglobals = shift;
    $useglobals = 0 unless defined($useglobals);
    return bless {function    => $func,
                  description => $desc,
                  useglobals  => $useglobals}, $class;
}

sub description {
    my $self = shift;
    return $self->{description} ||
    ("User-defined perl style that calls " . $self->{function});
};

sub format_message {
    my $self = shift;
    if($self->{useglobals}) {
        $_[0]->legacy_populate_global();
    }
    {
      package main;
      no strict 'refs';
      goto \&{$self->{function}};
    }
}


1;
