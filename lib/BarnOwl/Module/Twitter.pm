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

# Don't redefine variables if they already exist
# This is a workaround for http://barnowl.mit.edu/trac/ticket/44
# Which was fixed in svn r819
if((BarnOwl::getvar('twitter:class')||'') eq '') {
    my $desc = <<'END_DESC';
BarnOwl::Module::Twitter will watch for authentic zephyrs to
-c $twitter:class -i $twitter:instance -O $twitter:opcode
from your sender and mirror them to Twitter.

A value of '*' in any of these fields acts a wildcard, accepting
messages with any value of that field.
END_DESC
    BarnOwl::new_variable_string('twitter:class',
                               {
                                   default => $class,
                                   summary => 'Class to watch for Twitter messages',
                                   description => $desc
                                  });
    BarnOwl::new_variable_string('twitter:instance',
                               {
                                   default => $instance,
                                   summary => 'Instance on twitter:class to watch for Twitter messages.',
                                   description => $desc
                                  });
    BarnOwl::new_variable_string('twitter:opcode',
                               {
                                   default => $opcode,
                                   summary => 'Opcode for zephyrs that will be sent as twitter updates',
                                   description => $desc
                                  });
}

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
};
if($@) {
    $BarnOwl::Hooks::receiveMessage->add(\&handle_message);
}

1;
