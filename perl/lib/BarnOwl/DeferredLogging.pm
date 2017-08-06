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

$BarnOwl::Hooks::startup->add("BarnOwl::DeferredLogging::_register_variables");

sub _register_variables {
    my $flush_logs_interval = -1;
    my $flush_logs_timer;
    BarnOwl::new_variable_full('flush-logs-interval',
        {
            default        => 60,
            summary        => 'how often should logs be flushed, in minutes',
            description    => "If this is set to a positive value n, deferred logs \n"
                            . "are flushed every n minutes.  If set to a negative or \n"
                            . "zero values, deferred logs are only flushed when the \n"
                            . "command :flush-logs is used.",
            get_tostring   => sub { "$flush_logs_interval" },
            set_fromstring => sub {
                die "Expected integer" unless $_[0] =~ /^-?[0-9]+$/;
                $flush_logs_interval = 0 + $_[0];
                $flush_logs_timer->stop if defined $flush_logs_timer;
                undef $flush_logs_timer;
                if ($flush_logs_interval > 0) {
                    $flush_logs_timer = BarnOwl::Timer->new({
                            name     => 'flush-logs interval timer',
                            interval => 60 * $flush_logs_interval,
                            cb       => sub { BarnOwl::command("flush-logs", "-q"); }
                        });
                }
            },
            validsettings => "<int>",
            takes_on_off => 0,
        });
}

1;
