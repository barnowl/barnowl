use warnings;
use strict;

=head1 NAME

BarnOwl::Module::AIM

=head1 DESCRIPTION

BarnOwl module implementing AIM support via Net::OSCAR

=cut

package BarnOwl::Module::AIM; 

use Net::OSCAR;

sub cmd_aimlogin { 
    my ($cmd, $user, $pass) = @_;
    if (undef $user) {
        BarnOwl::start_question('Username: ', sub {
                cmd_aimlogin($cmd, @_);
                });
    } elsif (undef $pass) {
        BarnOwl::start_password('Password: ', sub {
                cmd_aimlogin($cmd, $user, @_);
                });
    } else {
        my $oscar = Net::OSCAR->new();
        $oscar->set_callback_im_in(\&on_im_in);
        $oscar->signon($user, $pass);
    }
}

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

BarnOwl::new_command(aimlogin => \&cmd_aimlogin, {});

1;
