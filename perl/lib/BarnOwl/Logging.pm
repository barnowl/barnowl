use strict;
use warnings;

package BarnOwl::Logging;

=head1 BarnOwl::Logging

=head1 DESCRIPTION

C<BarnOwl::Logging> implements the internals of logging.  All customizations
to logging should be done in the appropriate subclass of L<BarnOwl::Message>.

=head2 USAGE

Modules wishing to customize how messages are logged should override the
relevant subroutines in the appropriate subclass of L<BarnOwl::Message>.

=head2 EXPORTS

None by default.

=cut

use Exporter;

our @EXPORT_OK = qw();

our %EXPORT_TAGS = (all => [@EXPORT_OK]);

use File::Spec;

=head2 sanitize_filename BASE_PATH FILENAME

Sanitizes C<FILENAME> and concatenates it with C<BASE_PATH>.

In any filename, C<"/"> and any control characters (characters which
match C<[:cntrl:]> get replaced by underscores.  If the resulting
filename is empty or equal to C<"."> or C<"..">, it is replaced with
C<"weird">.

=cut

sub sanitize_filename {
    my $base_path = BarnOwl::Internal::makepath(shift);
    my $filename = shift;
    $filename =~ s/[[:cntrl:]\/]/_/g;
    if ($filename eq '' || $filename eq '.' || $filename eq '..') {
        $filename = 'weird';
    }
    # The original C code also removed characters less than '!' and
    # greater than or equal to '~', marked file names beginning with a
    # non-alphanumeric or non-ASCII character as 'weird', and rejected
    # filenames longer than 35 characters.
    return File::Spec->catfile($base_path, $filename);
}

=head2 get_filenames MESSAGE

Returns a list of filenames in which to log the passed message.

This method calls C<log_filenames> on C<MESSAGE> to determine the list
of filenames to which C<MESSAGE> gets logged.  All filenames are
relative to C<MESSAGE->log_base_path>.  If C<MESSAGE->log_to_all_file>
returns true, then the filename C<"all"> is appended to the list of
filenames.

Filenames are sanitized by C<sanitize_filename>.

=cut

sub get_filenames {
    my ($m) = @_;
    my @filenames = $m->log_filenames;
    push @filenames, 'all' if $m->log_to_all_file;
    return map { sanitize_filename($m->log_base_path, $_) } @filenames;
}

# For ease of use in C
sub get_filenames_as_string {
    my @rtn;
    foreach my $filename (BarnOwl::Logging::get_filenames(@_)) {
        $filename =~ s/\n/_/g;
        push @rtn, $filename;
    }
    return join("\n", @rtn);
}

1;
