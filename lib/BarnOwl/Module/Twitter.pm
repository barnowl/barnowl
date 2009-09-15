use warnings;
use strict;

=head1 NAME

BarnOwl::Module::Twitter

=head1 DESCRIPTION

Post outgoing zephyrs from -c $USER -i status -O TWITTER to Twitter

=cut

package BarnOwl::Module::Twitter;

our $VERSION = 0.2;

use Net::Twitter;
use JSON;
use List::Util qw(first);

use BarnOwl;
use BarnOwl::Hooks;
use BarnOwl::Module::Twitter::Handle;

our @twitter_handles = ();
our $default_handle = undef;
my $user     = BarnOwl::zephyr_getsender();
my ($class)  = ($user =~ /(^[^@]+)/);
my $instance = "status";
my $opcode   = "twitter";
my $use_reply_to = 0;
my $next_service_to_poll = 0;

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

BarnOwl::new_variable_bool(
    'twitter:poll',
    {
        default => 1,
        summary => 'Poll Twitter for incoming messages',
        description => "If set, will poll Twitter every minute for normal updates,\n"
        . 'and every two minutes for direct message'
     }
 );

sub fail {
    my $msg = shift;
    undef @twitter_handles;
    BarnOwl::admin_message('Twitter Error', $msg);
    die("Twitter Error: $msg\n");
}

my $conffile = BarnOwl::get_config_dir() . "/twitter";
open(my $fh, "<", "$conffile") || fail("Unable to read $conffile");
my $raw_cfg = do {local $/; <$fh>};
close($fh);
eval {
    $raw_cfg = from_json($raw_cfg);
};
if($@) {
    fail("Unable to parse $conffile: $@");
}

$raw_cfg = [$raw_cfg] unless UNIVERSAL::isa $raw_cfg, "ARRAY";

for my $cfg (@$raw_cfg) {
    my $twitter_args = { username   => $cfg->{user} || $user,
                        password   => $cfg->{password},
                        source     => 'barnowl', 
                    };
    if (defined $cfg->{service}) {
        my $service = $cfg->{service};
        $twitter_args->{apiurl} = $service;
        my $apihost = $service;
        $apihost =~ s/^\s*http:\/\///;
        $apihost =~ s/\/.*$//;
        $apihost .= ':80' unless $apihost =~ /:\d+$/;
        $twitter_args->{apihost} = $cfg->{apihost} || $apihost;
        my $apirealm = "Laconica API";
        $twitter_args->{apirealm} = $cfg->{apirealm} || $apirealm;
    } else {
        $cfg->{service} = 'http://twitter.com';
    }

    my $twitter_handle;
    eval {
         $twitter_handle = BarnOwl::Module::Twitter::Handle->new($cfg, %$twitter_args);
    };
    if ($@) {
        BarnOwl::error($@);
        next;
    }
    push @twitter_handles, $twitter_handle;
}

$default_handle = first {$_->{cfg}->{default}} @twitter_handles;
if (!$default_handle && @twitter_handles) {
    $default_handle = $twitter_handles[0];
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
        for my $handle (@twitter_handles) {
            $handle->twitter($m->body) if $handle->{cfg}->{publish_tweets};
        }
    }
}

sub poll_messages {
    return unless @twitter_handles;

    my $handle = $twitter_handles[$next_service_to_poll];
    $next_service_to_poll = ($next_service_to_poll + 1) % scalar(@twitter_handles);
    
    $handle->poll_twitter() if $handle->{cfg}->{poll_for_tweets};
    $handle->poll_direct() if $handle->{cfg}->{poll_for_dms};
}

sub find_account {
    my $name = shift;
    my $handle = first {$_->{cfg}->{account_nickname} eq $name} @twitter_handles;
    if ($handle) {
        return $handle;
    } else {
        die("No such Twitter account: $name\n");
    }
}

sub find_account_default {
    my $name = shift;
    if(defined($name)) {
        return find_account($name);
    } else {
        return $default_handle;
    }
}

sub twitter {
    my $account = shift;

    my $sent = 0;
    if (defined $account) {
        my $handle = find_account($account);
        $handle->twitter(@_);
    } 
    else {
        # broadcast
        for my $handle (@twitter_handles) {
            $handle->twitter(@_) if $handle->{cfg}->{publish_tweets};
        }
    }
}

BarnOwl::new_command(twitter => \&cmd_twitter, {
    summary     => 'Update Twitter from BarnOwl',
    usage       => 'twitter [ACCOUNT] [MESSAGE]',
    description => 'Update Twitter on ACCOUNT. If MESSAGE is provided, use it as your status.'
    . "\nIf no ACCOUNT is provided, update all services which have publishing enabled."
    . "\nOtherwise, prompt for a status message to use."
   });

BarnOwl::new_command('twitter-direct' => \&cmd_twitter_direct, {
    summary     => 'Send a Twitter direct message',
    usage       => 'twitter-direct USER [ACCOUNT]',
    description => 'Send a Twitter Direct Message to USER on ACCOUNT (defaults to default_sender,'
    . "\nor first service if no default is provided)"
   });

BarnOwl::new_command( 'twitter-atreply' => sub { cmd_twitter_atreply(@_); },
    {
    summary     => 'Send a Twitter @ message',
    usage       => 'twitter-atreply USER [ACCOUNT]',
    description => 'Send a Twitter @reply Message to USER on ACCOUNT (defaults to default_sender,' 
    . "\nor first service if no default is provided)"
    }
);

BarnOwl::new_command( 'twitter-follow' => sub { cmd_twitter_follow(@_); },
    {
    summary     => 'Follow a user on Twitter',
    usage       => 'twitter-follow USER [ACCOUNT]',
    description => 'Follow USER on Twitter ACCOUNT (defaults to default_sender, or first service'
    . "\nif no default is provided)"
    }
);

BarnOwl::new_command( 'twitter-unfollow' => sub { cmd_twitter_unfollow(@_); },
    {
    summary     => 'Stop following a user on Twitter',
    usage       => 'twitter-unfollow USER [ACCOUNT]',
    description => 'Stop following USER on Twitter ACCOUNT (defaults to default_sender, or first'
    . "\nservice if no default is provided)"
    }
);

sub cmd_twitter {
    my $cmd = shift;
    my $account = shift;
    if (defined $account) {
        if(@_) {
            my $status = join(" ", @_);
            twitter($account, $status);
            return;
        }
    }
    BarnOwl::start_edit_win('What are you doing?' . (defined $account ? " ($account)" : ""), sub{twitter($account, shift)});
}

sub cmd_twitter_direct {
    my $cmd = shift;
    my $user = shift;
    die("Usage: $cmd USER\n") unless $user;
    my $account = find_account_default(shift);
    BarnOwl::start_edit_win("$cmd $user " . ($account->nickname||""),
                            sub { $account->twitter_direct($user, shift) });
}

sub cmd_twitter_atreply {
    my $cmd  = shift;
    my $user = shift || die("Usage: $cmd USER [In-Reply-To ID]\n");
    my $id   = shift;
    my $account = find_account_default(shift);

    BarnOwl::start_edit_win("Reply to \@" . $user . ($account->nickname ? (" on " . $account->nickname) : ""),
                            sub { $account->twitter_atreply($user, $id, shift) });
}

sub cmd_twitter_follow {
    my $cmd = shift;
    my $user = shift;
    die("Usage: $cmd USER\n") unless $user;
    my $account = shift;
    find_account_default($account)->twitter_follow($user);
}

sub cmd_twitter_unfollow {
    my $cmd = shift;
    my $user = shift;
    die("Usage: $cmd USER\n") unless $user;
    my $account = shift;
    find_account_default($account)->twitter_unfollow($user);
}

eval {
    $BarnOwl::Hooks::receiveMessage->add("BarnOwl::Module::Twitter::handle_message");
    $BarnOwl::Hooks::mainLoop->add("BarnOwl::Module::Twitter::poll_messages");
};
if($@) {
    $BarnOwl::Hooks::receiveMessage->add(\&handle_message);
    $BarnOwl::Hooks::mainLoop->add(\&poll_messages);
}

BarnOwl::filter(qw{twitter type ^twitter$});

1;
