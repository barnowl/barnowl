package Facebook::Graph::Response;
BEGIN {
  $Facebook::Graph::Response::VERSION = '1.0300';
}

use Any::Moose;
use JSON;
use Ouch;

has response => (
    is      => 'ro',
    required=> 1,
);

has headers => (
    is      => 'ro',
    required=> 1,
);

has uri => (
    is      => 'ro',
    required=> 1,
);

has as_string => (
    is      => 'ro',
    lazy    => 1,
    default => sub {
        my $self = shift;
        if (!defined $self->response) {
            ouch $self->headers->{Status}, $self->headers->{Reason}, $self->uri;
        }
        if ($self->headers->{Status} < 200 || $self->headers->{Status} >= 300) {
            my $type = $self->headers->{Status};
            my $message = $self->response;
            my $error = eval { JSON->new->decode($self->response) };
            unless ($@) {
                $type = $error->{error}{type};
                $message = $error->{error}{message};
            }
            ouch $type, 'Could not execute request ('.$self->uri.'): '.$message, $self->uri;
        }
        return $self->response;
    },
);

has as_json => (
    is      => 'ro',
    lazy    => 1,
    default => sub {
        my $self = shift;
        return $self->as_string;
    },
);

has as_hashref => (
    is      => 'ro',
    lazy    => 1,
    default => sub {
        my $self = shift;
        return JSON->new->decode($self->as_json);
    },
);

no Any::Moose;
__PACKAGE__->meta->make_immutable;

=head1 NAME

Facebook::Graph::Response - Handling of a Facebook::Graph response documents.

=head1 VERSION

version 1.0300

=head1 DESCRIPTION

You'll be given one of these as a result of calling the C<request> method on a C<Facebook::Graph::Query> or others.


=head1 METHODS

Returns the response as a string. Does not throw an exception of any kind.

=head2 as_json ()

Returns the response from Facebook as a JSON string.

=head2 as_hashref ()

Returns the response from Facebook as a hash reference.

=head2 as_string ()

No processing what so ever. Just returns the raw body string that was received from Facebook.

=head2 response ()

Direct access to the L<HTTP::Response> object.

=head1 LEGAL

Facebook::Graph is Copyright 2010 Plain Black Corporation (L<http://www.plainblack.com>) and is licensed under the same terms as Perl itself.

=cut
