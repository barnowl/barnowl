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
            direction => 'in',
            sender => $sender,
            origbody => $message,
            away => $is_away,
            body => zformat($message, $is_away),
            recipient => get_screenname($oscar),
            replycmd =>
                "aimwrite -a '" . get_screenname($oscar) . "' $sender",
            replysendercmd =>
                "aimwrite -a '" . get_screenname($oscar) . "' $sender",
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
        $oscar->set_callback_signon_done(sub ($) {
                BarnOwl::admin_message('AIM',
                    'Logged in to AIM as ' . shift->screenname);
        });
        $oscar->signon(
                screenname => $user,
                password => $pass
                );
        push @oscars, $oscar;
    }
}

sub cmd_aimwrite {
    my ($cmd, $recipient) = @_;
    BarnOwl::start_edit_win(join(' ', @_), sub {
            my ($body) = @_;
            my $oscar = get_oscar();
            my $sender = get_screenname($oscar);
            $oscar->send_im($recipient, $body);
            BarnOwl::queue_message(BarnOwl::Message->new(
                    type => 'AIM',
                    direction => 'in',
                    sender => $sender,
                    origbody => $body,
                    away => 0,
                    body => zformat($body, 0),
                    recipient => $recipient,
                    replycmd =>
                        "aimwrite -a '" . get_screenname($oscar) . "' $sender",
                    replysendercmd =>
                        "aimwrite -a '" . get_screenname($oscar) . "' $sender",
            ));
    });
}

BarnOwl::new_command(aimlogin => \&cmd_aimlogin, {});
BarnOwl::new_command(aimwrite => \&cmd_aimwrite, {});

sub main_loop() {
    for my $oscar (@oscars) {
        $oscar->do_one_loop();
    }
}
$BarnOwl::Hooks::mainLoop->add(\&main_loop);

### helpers ###

sub zformat($$) {
    # TODO subclass HTML::Parser
    my ($message, $is_away) = @_;
    if ($is_away) {
        return BarnOwl::boldify('[away]') . " $message";
    } else {
        return $message;
    }
}

sub get_oscar() {
    if (scalar @oscars == 0) {
        die "You are not logged in to AIM."
    } elsif (scalar @oscars == 1) {
        return $oscars[0];
    } else {
        my $m = BarnOwl::getcurmsg();
        if ($m && $m->type eq 'AIM') {
            for my $oscar (@oscars) {
                return $oscar if ($oscar->screenname eq $m->recipient);
            }
        }
    }
    die('You must specify an account with -a');
}

sub get_screenname($) {
# TODO qualify realm
    return shift->screenname;
}

1;

# vim: set sw=4 et cin:
