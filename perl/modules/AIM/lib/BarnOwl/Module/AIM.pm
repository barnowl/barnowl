use warnings;
use strict;

=head1 NAME

BarnOwl::Module::AIM

=head1 DESCRIPTION

BarnOwl module implementing AIM support via Net::OSCAR

=cut

package BarnOwl::Module::AIM; 

use Net::OSCAR;

our @oscars;

sub on_im_in {
    my ($oscar, $sender, $message, $is_away) = @_;
    my $msg = BarnOwl::Message->new(
            type => 'AIM',
            sender => $sender,
            body => $message,
            away => $is_away,
            );
    BarnOwl::queue_message($msg);
}

sub cmd_aimlogin { 
=comment
    my ($cmd, $user, $pass) = @_;
    if (!defined $user) {
        BarnOwl::start_question('Username: ', sub {
                cmd_aimlogin($cmd, @_);
                });
    } elsif (!defined $pass) {
        BarnOwl::start_password('Password: ', sub {
                cmd_aimlogin($cmd, $user, @_);
                });
    } else {
=cut
    {
        my $oscar = Net::OSCAR->new();
        my ($user, $pass) = ('...', '...');
        $oscar->set_callback_im_in(\&on_im_in);
        $oscar->signon(
                screenname => $user,
                password => $pass
                );
        push @oscars, $oscar;
    }
}
BarnOwl::new_command(aimlogin => \&cmd_aimlogin, {});

sub main_loop {
    for my $oscar (@oscars) {
        $oscar->do_one_loop();
    }
}
$BarnOwl::Hooks::mainLoop->add(\&main_loop);

1;

# vim: set sw=4 et cin:
