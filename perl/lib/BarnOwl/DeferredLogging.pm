use strict;
use warnings;

package BarnOwl::DeferredLogging;

=head1 BarnOwl::DeferredLogging

=head1 DESCRIPTION

C<BarnOwl::DeferredLogging> adds variables relevant to deferred logging.

=head2 USAGE

Not Applicable.

=head2 EXPORTS

None by default.

=cut

use BarnOwl::Timer;
use Exporter;

our @EXPORT_OK = qw();

our %EXPORT_TAGS = (all => [@EXPORT_OK]);

# BarnOwl::DeferredLogging::_register_variables is called by BarnOwl::Hooks::_startup.

sub _register_variables {
    my $flush_logs_interval = 60;
    my $flush_logs_timer;
    my $set_interval = sub {
        my ($value) = @_;
        die "Expected integer" unless $value =~ /^-?[0-9]+$/;
        $flush_logs_interval = $value;
        $flush_logs_timer->stop if defined $flush_logs_timer;
        undef $flush_logs_timer;
        if ($flush_logs_interval > 0) {
            $flush_logs_timer = BarnOwl::Timer->new({
                    name     => 'flush-logs interval timer',
                    interval => 60 * $flush_logs_interval,
                    cb       => sub { BarnOwl::command("flush-logs", "-q"); }
                });
        }
    };
    # N.B. We use new_variable_full rather than new_variable_int so
    # that we can set a timer to flush logs when this value is
    # changed.
    BarnOwl::new_variable_full('flush-logs-interval',
        {
            default        => 60,
            summary        => 'how often should logs be flushed, in minutes',
            description    => "If this is set to a positive value n, deferred logs \n"
                            . "are flushed every n minutes.  If set to a negative or \n"
                            . "zero values, deferred logs are only flushed when the \n"
                            . "command :flush-logs is used.",
            get_tostring   => sub { "$flush_logs_interval" },
            set_fromstring => $set_interval,
            validsettings => "<int>",
            takes_on_off => 0,
        });
    # new_variable_full ignores 'default'; apply it, so the documented
    # default of 60 minutes takes effect.
    $set_interval->(60);
}

1;
