use warnings;
use strict;

=head1 NAME

BarnOwl::Module::Twitter

=head1 DESCRIPTION

Post outgoing zephyrs from -c $USER -i status -O TWITTER to Twitter

=cut

package BarnOwl::Module::Twitter;

use Net::Twitter;
use JSON;

use BarnOwl;
use BarnOwl::Hooks;
use BarnOwl::Message::Twitter;
use HTML::Entities;

my $twitter;
my $user     = BarnOwl::zephyr_getsender();
my ($class)  = ($user =~ /(^[^@]+)/);
my $instance = "status";
my $opcode   = "twitter";

sub fail {
    my $msg = shift;
    undef $twitter;
    BarnOwl::admin_message('Twitter Error', $msg);
    die("Twitter Error: $msg\n");
}

my $desc = <<'END_DESC';
BarnOwl::Module::Twitter will watch for authentic zephyrs to
-c $twitter:class -i $twitter:instance -O $twitter:opcode
from your sender and mirror them to Twitter.

A value of '*' in any of these fields acts a wildcard, accepting
messages with any value of that field.
END_DESC
BarnOwl::new_variable_string(
    'twitter:class',
    {
        default     => $class,
        summary     => 'Class to watch for Twitter messages',
        description => $desc
    }
);
BarnOwl::new_variable_string(
    'twitter:instance',
    {
        default => $instance,
        summary => 'Instance on twitter:class to watch for Twitter messages.',
        description => $desc
    }
);
BarnOwl::new_variable_string(
    'twitter:opcode',
    {
        default => $opcode,
        summary => 'Opcode for zephyrs that will be sent as twitter updates',
        description => $desc
    }
);

my $conffile = BarnOwl::get_config_dir() . "/twitter";
open(my $fh, "<", "$conffile") || fail("Unable to read $conffile");
my $cfg = do {local $/; <$fh>};
close($fh);
eval {
    $cfg = from_json($cfg);
};
if($@) {
    fail("Unable to parse ~/.owl/twitter: $@");
}

$twitter  = Net::Twitter->new(username   => $cfg->{user} || $user,
                              password   => $cfg->{password},
                              source => 'barnowl');

eval {
    $twitter->{ua}->timeout(1);
};

if(!defined($twitter->verify_credentials())) {
    fail("Invalid twitter credentials");
}

sub match {
    my $val = shift;
    my $pat = shift;
    return $pat eq "*" || ($val eq $pat);
}

sub handle_message {
    my $m = shift;
    ($class, $instance, $opcode) = map{BarnOwl::getvar("twitter:$_")} qw(class instance opcode);
    if($m->sender eq $user
       && match($m->class, $class)
       && match($m->instance, $instance)
       && match($m->opcode, $opcode)
       && $m->auth eq 'YES') {
        twitter($m->body);
    }
}

my $last_poll = 0;
my $last_id   = undef;
unless(defined($last_id)) {
    $last_id = $twitter->friends_timeline({count => 1})->[0]{id};
}

sub poll_messages {
    return unless ( time - $last_poll ) >= 45;
    $last_poll = time;
    my $timeline = $twitter->friends_timeline( { since_id => $last_id } );
    unless(defined($timeline)) {
        BarnOwl::error("Twitter returned error ... rate-limited?");
        # Sleep for 15 minutes
        $last_poll = time + 60*15;
        return;
    };
    if ( scalar @$timeline ) {
        for my $tweet ( reverse @$timeline ) {
            if ( $tweet->{id} <= $last_id ) {
                next;
            }
            my $msg = BarnOwl::Message->new(
                type      => 'Twitter',
                sender    => $tweet->{user}{screen_name},
                recipient => $cfg->{user} || $user,
                direction => 'in',
                source    => decode_entities($tweet->{source}),
                location  => decode_entities($tweet->{user}{location}||""),
                body      => decode_entities($tweet->{text})
               );
            BarnOwl::queue_message($msg);
        }
        $last_id = $timeline->[0]{id};
    } else {
        # BarnOwl::message("No new tweets...");
    }
}

sub twitter {
    my $msg = shift;
    if(defined $twitter) {
        $twitter->update($msg);
    }
}

BarnOwl::new_command(twitter => \&cmd_twitter, {
    summary     => 'Update Twitter from BarnOwl',
    usage       => 'twitter [message]',
    description => 'Update Twitter. If MESSAGE is provided, use it as your status.'
    . "\nOtherwise, prompt for a status message to use."
   });

sub cmd_twitter {
    my $cmd = shift;
    if(@_) {
        my $status = join(" ", @_);
        twitter($status);
    } else {
      BarnOwl::start_edit_win('What are you doing?', \&twitter);
    }
}

eval {
    $BarnOwl::Hooks::receiveMessage->add("BarnOwl::Module::Twitter::handle_message");
    $BarnOwl::Hooks::mainLoop->add("BarnOwl::Module::Twitter::poll_messages");
};
if($@) {
    $BarnOwl::Hooks::receiveMessage->add(\&handle_message);
    $BarnOwl::Hooks::mainLoop->add(\&poll_messages);
}

BarnOwl::filter('twitter type ^twitter$');

1;
