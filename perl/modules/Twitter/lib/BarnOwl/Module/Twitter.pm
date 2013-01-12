use warnings;
use strict;

=head1 NAME

BarnOwl::Module::Twitter

=head1 DESCRIPTION

Post outgoing zephyrs from -c $USER -i status -O TWITTER to Twitter

=cut

package BarnOwl::Module::Twitter;

our $VERSION = 0.2;

use JSON;
use List::Util qw(first);

use BarnOwl;
use BarnOwl::Hooks;
use BarnOwl::Module::Twitter::Handle;
use BarnOwl::Module::Twitter::Completion;

our @twitter_handles = ();
our $default_handle = undef;

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
        default     => $ENV{USER},
        summary     => 'Class to watch for Twitter messages',
        description => $desc
    }
);
BarnOwl::new_variable_string(
    'twitter:instance',
    {
        default => 'status',
        summary => 'Instance on twitter:class to watch for Twitter messages.',
        description => $desc
    }
);
BarnOwl::new_variable_string(
    'twitter:opcode',
    {
        default => 'twitter',
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

if (open(my $fh, "<", "$conffile")) {
    read_config($fh);
    close($fh);
}

sub read_config {
    my $fh = shift;

    my $raw_cfg = do {local $/; <$fh>};
    close($fh);
    eval {
        $raw_cfg = from_json($raw_cfg);
    };
    if($@) {
        fail("Unable to parse $conffile: $@");
    }

    $raw_cfg = [$raw_cfg] unless UNIVERSAL::isa $raw_cfg, "ARRAY";

    # Perform some sanity checking on the configuration.
  {
      my %nicks;
      my $default = 0;

      for my $cfg (@$raw_cfg) {
          if(! exists $cfg->{user}) {
              fail("Account has no username set.");
          }
          my $user = $cfg->{user};
          if(! exists $cfg->{password}) {
              fail("Account $user has no password set.");
          }
          if(@$raw_cfg > 1&&
             !exists($cfg->{account_nickname}) ) {
              fail("Account $user has no account_nickname set.");
          }
          if($cfg->{account_nickname}) {
              if($nicks{$cfg->{account_nickname}}++) {
                  fail("Nickname " . $cfg->{account_nickname} . " specified more than once.");
              }
          }
          if($cfg->{default} || $cfg->{default_sender}) {
              if($default++) {
                  fail("Multiple accounts marked as 'default'.");
              }
          }
      }
  }

    # If there is only a single account, make publish_tweets default to
    # true.
    if (scalar @$raw_cfg == 1 && !exists($raw_cfg->[0]{publish_tweets})) {
        $raw_cfg->[0]{publish_tweets} = 1;
    }

    for my $cfg (@$raw_cfg) {
        my $twitter_args = { username   => $cfg->{user},
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

}

sub match {
    my $val = shift;
    my $pat = shift;
    return $pat eq "*" || ($val eq $pat);
}

sub handle_message {
    my $m = shift;
    my ($class, $instance, $opcode) = map{BarnOwl::getvar("twitter:$_")} qw(class instance opcode);
    if($m->type eq 'zephyr'
       && $m->sender eq BarnOwl::zephyr_getsender()
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
    # If we are reloaded into a BarnOwl with the old
    # BarnOwl::Module::Twitter loaded, it still has a main loop hook
    # that will call this function every second. If we just delete it,
    # it will get the old version, which will call poll on each of our
    # handles every second. However, they no longer include the time
    # check, and so we will poll a handle every second until
    # ratelimited.

    # So we include an empty function here.
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

BarnOwl::new_command( 'twitter-retweet' => sub { cmd_twitter_retweet(@_) },
    {
    summary     => 'Retweet the current Twitter message',
    usage       => 'twitter-retweet [ACCOUNT]',
    description => <<END_DESCRIPTION
Retweet the current Twitter message using ACCOUNT (defaults to the
account that received the tweet).
END_DESCRIPTION
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

BarnOwl::new_command('twitter-count-chars' => \&cmd_count_chars, {
    summary     => 'Count the number of characters in the edit window',
    usage       => 'twitter-count-chars',
    description => <<END_DESCRIPTION
Displays the number of characters entered in the edit window so far.
END_DESCRIPTION
   });

$BarnOwl::Hooks::getQuickstart->add( sub { twitter_quickstart(@_); } );

sub twitter_quickstart {
    return <<'EOF'
@b[Twitter]:
Add your Twitter account to ~/.owl/twitter, like:
  {"user":"nelhage", "password":"sekrit"}
Run :reload-module Twitter, then use :twitter to tweet.
EOF
}


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
    BarnOwl::start_edit_win("What's happening?" . (defined $account ? " ($account)" : ""), sub{twitter($account, shift)});
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
    BarnOwl::Editwin::insert_text("\@$user ");
}

sub cmd_twitter_retweet {
    my $cmd = shift;
    my $account = shift;
    my $m = BarnOwl::getcurmsg();
    if(!$m || $m->type ne 'Twitter') {
        die("$cmd must be used with a Twitter message selected.\n");
    }

    $account = $m->account unless defined($account);
    find_account($account)->twitter_retweet($m);
    return;
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

use BarnOwl::Editwin qw(:all);
sub cmd_count_chars {
    my $cmd = shift;
    my $text = save_excursion {
        move_to_buffer_start();
        set_mark();
        move_to_buffer_end();
        get_region();
    };
    my $len = length($text);
    BarnOwl::message($len);
    return $len;
}

eval {
    $BarnOwl::Hooks::receiveMessage->add("BarnOwl::Module::Twitter::handle_message");
};
if($@) {
    $BarnOwl::Hooks::receiveMessage->add(\&handle_message);
}



BarnOwl::filter(qw{twitter type ^twitter$});

1;
