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

sub fail {
    my $msg = shift;
    BarnOwl::admin_message('Twitter Error', $msg);
    die("Twitter Error: $msg\n");
}

my $user     = BarnOwl::zephyr_getsender();
my ($class)  = ($user =~ /(^[^@]+)/);
my $instance = "status";
my $opcode   = "twitter";

# Don't redefine variables if they already exist
# This is a workaround for http://barnowl.mit.edu/trac/ticket/44
# Which was fixed in svn r819
if(BarnOwl::getvar('twitter:class') eq '') {
    BarnOwl::new_variable_string('twitter:class',
                               {
                                   default => $class,
                                   summary => 'Class to watch for Twitter messages'
                                  });
    BarnOwl::new_variable_string('twitter:instance',
                               {
                                   default => $instance,
                                   summary => 'Instance on twitter:class to watch for Twitter messages'
                                  });
    BarnOwl::new_variable_string('twitter:opcode',
                               {
                                   default => $opcode,
                                   summary => 'Opcode for zephyrs that will be sent as twitter updates'
                                  });
}

my $conffile = BarnOwl::get_config_dir() . "/twitter";
open(my $fh, "<", "$conffile") || fail("Unable to read $conffile");
my $cfg = do {local $/; <$fh>};
close($fh);
eval {
    $cfg = from_json($cfg);
};
if(@!) {
    fail("Unable to parse ~/.owl/twitter: @!");
}

my $twitter  = Net::Twitter->new(username   => $cfg->{user} || $user,
                                 password   => $cfg->{password},
                                 clientname => 'BarnOwl');

if(!defined($twitter->verify_credentials())) {
    fail("Invalid twitter credentials");
}

sub handle_message {
    my $m = shift;
    ($class, $instance, $opcode) = map{BarnOwl::getvar("twitter:$_")} qw(class instance opcode);
    if($m->sender eq $user
       && $m->class eq $class
       && $m->instance eq $instance
       && $m->opcode eq $opcode
       && $m->auth eq 'YES') {
        twitter($m->body);
    }
}

sub twitter {
    my $msg = shift;
    $twitter->update($msg);
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

$BarnOwl::Hooks::receiveMessage->add(\&handle_message);

1;
