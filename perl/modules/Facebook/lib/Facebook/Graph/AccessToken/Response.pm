package Facebook::Graph::AccessToken::Response;
BEGIN {
  $Facebook::Graph::AccessToken::Response::VERSION = '1.0300';
}

use Any::Moose;
use URI;
use URI::QueryParam;
use Ouch;

has response => (
    is      => 'ro',
    required=> 1,
);

has token => (
    is      => 'ro',
    lazy    => 1,
    default => sub {
        # LOL error handling
        my $self = shift;
        return URI->new('?'.$self->response)->query_param('access_token');
        #else {
        #    ouch $response->code, 'Could not fetch access token: '.$response->message, $response->request->uri->as_string;
        #}
    }
);

has expires => (
    is      => 'ro',
    lazy    => 1,
    default => sub {
        # LOL error handling
        my $self = shift;
        return URI->new('?'.$self->response)->query_param('expires');
        #else {
        #    ouch $response->code, 'Could not fetch access token: '.$response->message, $response->request->uri->as_string;
        #}
    }
);

no Any::Moose;
__PACKAGE__->meta->make_immutable;

=head1 NAME

Facebook::Graph::AccessToken::Response - The Facebook access token request response.

=head1 VERSION

version 1.0300

=head1 Description

You'll be given one of these as a result of calling the C<request> method from a L<Facebook::Graph::AccessToken> object.

=head1 METHODS

=head2 token ()

Returns the token string.

=head2 expires ()

Returns the time alotted to this token. If undefined then the token is forever.

=head2 response ()

Direct access to the L<HTTP::Response> object.

=head1 LEGAL

Facebook::Graph is Copyright 2010 Plain Black Corporation (L<http://www.plainblack.com>) and is licensed under the same terms as Perl itself.

=cut
