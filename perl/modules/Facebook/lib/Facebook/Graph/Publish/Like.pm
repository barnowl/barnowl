package Facebook::Graph::Publish::Like;
BEGIN {
  $Facebook::Graph::Publish::Like::VERSION = '1.0300';
}

use Any::Moose;
extends 'Facebook::Graph::Publish';

use constant object_path => '/likes';

no Any::Moose;
__PACKAGE__->meta->make_immutable;


=head1 NAME

Facebook::Graph::Publish::Like - Mark a post as something you like.

=head1 VERSION

version 1.0300

=head1 SYNOPSIS

 my $fb = Facebook::Graph->new;

 $fb->add_like($post_id)
    ->publish;

=head1 DESCRIPTION

Marks a post as something you like.

B<ATTENTION:> You must have the C<publish_stream> privilege to use this module.

=head1 METHODS


=head2 publish ( )

Posts the data and returns a L<Facebook::Graph::Response> object. The response object should contain a string of either 'true' or 'false'.

=head1 LEGAL

Facebook::Graph is Copyright 2010 Plain Black Corporation (L<http://www.plainblack.com>) and is licensed under the same terms as Perl itself.

=cut