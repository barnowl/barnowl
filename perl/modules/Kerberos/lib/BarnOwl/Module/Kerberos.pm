use strict;
use warnings;

package BarnOwl::Module::Kerberos;

=head1 NAME

BarnOwl::Module::Kerberos

=head1 DESCRIPTION

This module allows someone to renew tickets within BarnOwl

=cut

use BarnOwl;
use AnyEvent;
use AnyEvent::Handle;
use IPC::Open3;

use Data::Dumper;

our $VERSION = 1.0;

BarnOwl::new_variable_bool(
    'aklog',
    {
        default => 1,
        summary => 'Enable running aklog on renew',
        description => "If set, aklog will be run during the renew command."
     }
 );

sub startup {
    register_commands();
}

sub register_commands {
    BarnOwl::new_command(
        'renew' => \&cmd_renew,
        {
            summary => 'Renew Kerberos Tickets',
            usage => 'renew',
            description => <<END_DESCR
Renews Kerberos Ticket
END_DESCR
        }
    );
}


$BarnOwl::Hooks::startup->add('BarnOwl::Module::Kerberos::startup');

################################################################################
######################## Owl command handlers ##################################
################################################################################


sub cmd_renew {
    BarnOwl::start_password("Password: ", \&do_renew );
    return "";
}


my $hdlin;
my $hdlerr;
my $kinit_watcher;

sub do_renew {

    my $password = shift;
    my ($stdin, $stdout, $stderr);
    use Symbol 'gensym'; $stderr = gensym;
    my $pid = open3($stdin, $stdout, $stderr, 'kinit', '-l7d') or die("Failed to run kinit");

    $hdlerr = new AnyEvent::Handle(fh => $stderr);
    $hdlin = new AnyEvent::Handle(fh => $stdin);

    my $output = "";

    $hdlin->push_write($password .  "\n");
    $hdlerr->push_read(line => sub {
        my ($hdl, $line) = @_;
        $output .= $line;
                     });
    close $stdout;
    $kinit_watcher = AnyEvent->child (pid => $pid, cb => sub {
        my ($pid, $status) = @_;
        if ($status != 0) {
            BarnOwl::error($output);
        } else {
            if (BarnOwl::getvar("aklog") eq 'on') {
                my $status = system('aklog');
                if ($status != 0) {
                    BarnOwl::error('Aklog Failed');
                }
            }
        }
    });

}


1;
