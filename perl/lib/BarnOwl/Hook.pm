use strict;
use warnings;

package BarnOwl::Hook;

=head1 BarnOwl::Hook

=head1 DESCRIPTION

A C<BarnOwl::Hook> represents a list of functions to be triggered on
some event. C<BarnOwl> exports a default set of these (see
C<BarnOwl::Hooks>), but can also be created and used by module code.

=head2 new

Creates a new Hook object

=cut

sub new {
    my $class = shift;
    return bless [], $class;
}

=head2 run [ARGS]

Calls each of the functions registered with this hook with the given
arguments.

=cut

sub run {
    my $self = shift;
    my @args = @_;
    return map {$self->_run($_,@args)} @$self;
}

sub _run {
    my $self = shift;
    my $fn = shift;
    my @args = @_;
    no strict 'refs';
    return $fn->(@args);
}

=head2 add SUBREF

Registers a given subroutine with this hook

=cut

sub add {
    my $self = shift;
    my $func = shift;
    die("Not a coderef!") unless ref($func) eq 'CODE' || !ref($func);
    return if grep {$_ eq $func} @$self;
    push @$self, $func;
}

=head2 clear

Remove all functions registered with this hook.

=cut

sub clear {
    my $self = shift;
    @$self = ();
}


1;
