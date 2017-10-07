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

Modules wishing to log errors sending outgoing messages should call
L<BarnOwl::Logging::log_outgoing_error> with the message that failed
to be sent.

=head2 EXPORTS

None by default.

=cut

use Exporter;

our @EXPORT_OK = qw();

our %EXPORT_TAGS = (all => [@EXPORT_OK]);

use File::Spec;

$BarnOwl::Hooks::newMessage->add("BarnOwl::Logging::log");
$BarnOwl::Hooks::startup->add("BarnOwl::Logging::_register_variables");

sub _register_variables {
    BarnOwl::new_variable_bool('logging',
        {
            default     => 0,
            summary     => 'turn personal logging on or off',
            description => "If this is set to on, personal messages are\n"
                         . "logged in the directory specified\n"
                         . "by the 'logpath' variable.  The filename in that\n"
                         . "directory is derived from the sender of the message."
        });

    BarnOwl::new_variable_bool('classlogging',
        {
            default     => 0,
            summary     => 'turn class logging on or off',
            description => "If this is set to on, class messages are\n"
                         . "logged in the directory specified by the\n"
                         . "'classpath' variable.  The filename in that\n"
                         . "directory is derived from the class to which\n"
                         . "the message was sent."
        });

    BarnOwl::new_variable_string('logfilter',
        {
            default     => '',
            summary     => 'name of a filter controlling which messages to log',
            description => "If non empty, any messages matching the given filter will be logged.\n"
                         . "This is a completely separate mechanism from the other logging\n"
                         . "variables like logging, classlogging, loglogins, loggingdirection,\n"
                         . "etc.  If you want this variable to control all logging, make sure\n"
                         . "all other logging variables are left off (the default)."
        });

    BarnOwl::new_variable_bool('loglogins',
        {
            default     => 0,
            summary     => 'enable logging of login notifications',
            description => "When this is enabled, BarnOwl will log login and logout notifications\n"
                         . "for zephyr or other protocols.  If disabled BarnOwl will not log\n"
                         . "login or logout notifications."
        });

    BarnOwl::new_variable_enum('loggingdirection',
        {
            default       => 'both',
            validsettings => [qw(in out both)],
            summary       => "specifies which kind of messages should be logged",
            description   => "Can be one of 'both', 'in', or 'out'.  If 'in' is\n"
                           . "selected, only incoming messages are logged, if 'out'\n"
                           . "is selected only outgoing messages are logged.  If 'both'\n"
                           . "is selected both incoming and outgoing messages are\n"
                           . "logged.\n\n"
                           . "Note that this variable applies to all messages. In\n"
                           . "particular, if this variable is set to 'out', the\n"
                           . "classlogging variable will have no effect, and no\n"
                           . "class messages (which are always incoming) will be\n"
                           . "logged."
        });

    BarnOwl::new_variable_string('logbasepath',
        {
            default       => '~/zlog',
            validsettings => '<path>',
            summary       => 'path for logging non-zephyr messages',
            description   => "Specifies a directory which must exist.\n"
                           . "Each non-zephyr protocol gets its own subdirectory in\n"
                           . "logbasepath, and messages get logged there.  Note that\n"
                           . "if the directory logbasepath/\$protocol does not exist,\n"
                           . "logging will fail for \$protocol."
        });

    BarnOwl::new_variable_string('logpath',
        {
            default       => '~/zlog/people',
            validsettings => '<path>',
            summary       => 'path for logging personal messages',
            description   => "Specifies a directory which must exist.\n"
                            . "Files will be created in the directory for each sender."
        });

    BarnOwl::new_variable_string('classlogpath',
        {
            default       => '~/zlog/class',
            validsettings => '<path>',
            summary       => 'path for logging class zephyrs',
            description   => "Specifies a directory which must exist.\n"
                           . "Files will be created in the directory for each class."
        });

    BarnOwl::new_variable_bool('log-to-subdirectories',
        {
            default     => 0,
            summary     => "log each protocol to its own subdirectory of logbasepath",
            description => "When this is enabled, BarnOwl will log each protocol to its own\n"
                         . "subdirectory of logbasepath.  When this is disabled, BarnOwl will\n"
                         . "instead log all non-zephyr non-loopback messages to the logpath,\n"
                         . "and prefix each filename with what would otherwise be the subdirectory\n"
                         . "name.\n\n"
                         . "If you enable this, be sure that the relevant directories exist;\n"
                         . "BarnOwl will not create them for you."
        });

}

=head2 sanitize_filename BASE_PATH FILENAME

Sanitizes C<FILENAME> and concatenates it with C<BASE_PATH>.

In any filename, C<"/">, any control characters (characters which
match C<[:cntrl:]>), and any initial C<"."> characters get replaced by
underscores.  If the resulting filename is empty, it is replaced with
C<"_EMPTY_">.

=cut

sub sanitize_filename {
    my $base_path = BarnOwl::Internal::makepath(shift);
    my $filename = shift;
    $filename =~ s/[[:cntrl:]\/]/_/g;
    $filename =~ s/^\./_/g; # handle ., .., and hidden files
    $filename = '_EMPTY_' if $filename eq '';
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
relative to C<MESSAGE->log_path>.  If C<MESSAGE->log_to_all_file>
returns true, then the filename C<"all"> is appended to the list of
filenames.

Filenames are sanitized by C<sanitize_filename>.

=cut

sub get_filenames {
    my ($m) = @_;
    my @filenames = $m->log_filenames;
    push @filenames, 'all' if $m->log_to_all_file;
    return map { sanitize_filename($m->log_path, $_) } @filenames;
}

=head2 should_log_message MESSAGE

Determines whether or not the passed message should be logged.

To customize the behavior of this method, override
L<BarnOwl::Message::should_log>.

=cut

sub should_log_message {
    my ($m) = @_;
    # If there's a logfilter and this message matches it, log.
    # pass quiet=1, because we don't care if the filter doesn't exist
    return 1 if BarnOwl::message_matches_filter($m, BarnOwl::getvar('logfilter'), 1);
    # otherwise we do things based on the logging variables
    # skip login/logout messages if appropriate
    return 0 if $m->is_loginout && BarnOwl::getvar('loglogins') eq 'off';
    # check direction
    return 0 if $m->is_outgoing && BarnOwl::getvar('loggingdirection') eq 'in';
    return 0 if $m->is_incoming && BarnOwl::getvar('loggingdirection') eq 'out';
    return $m->should_log;
}

=head2 log MESSAGE

Call this method to (potentially) log a message.

To customize the behavior of this method for your messages, override
L<BarnOwl::Message::log>, L<BarnOwl::Message::should_log>,
L<BarnOwl::Message::log_base_path>, and/or
L<BarnOwl::Message::log_filenames>.

=cut

sub log {
    my ($m) = @_;
    return unless defined $m;
    return unless BarnOwl::Logging::should_log_message($m);
    my $log_text = $m->log;
    foreach my $filename (BarnOwl::Logging::get_filenames($m)) {
        BarnOwl::Logging::enqueue_text($log_text, $filename);
    }
}

=head2 log_outgoing_error MESSAGE

Call this method to (potentially) log an error in sending an
outgoing message.  Errors get logged to the same file(s) as
successful messages.

To customize the behavior of this method for your messages, override
L<BarnOwl::Message::log_outgoing_error>,
L<BarnOwl::Message::should_log>,
L<BarnOwl::Message::log_base_path>, and/or
L<BarnOwl::Message::log_filenames>.

=cut

sub log_outgoing_error {
    my ($m) = @_;
    return unless BarnOwl::Logging::should_log_message($m);
    my $log_text = $m->log_outgoing_error;
    foreach my $filename (BarnOwl::Logging::get_filenames($m)) {
        BarnOwl::Logging::enqueue_text($log_text, $filename);
    }
}

1;
