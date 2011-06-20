use warnings;
use strict;

=head1 NAME

BarnOwl::Module::Facebook

=head1 DESCRIPTION

Integration with Facebook wall posting and commenting.

=cut

package BarnOwl::Module::Facebook;

our $VERSION = 0.1;

use JSON;

use BarnOwl;
use BarnOwl::Hooks;
use BarnOwl::Module::Facebook::Handle;

our $facebook_handle = undef;

# did not implement class monitoring
# did not implement multiple accounts

BarnOwl::new_variable_bool(
    'facebook:poll',
    {
        default => 1,
        summary => 'Poll Facebook for wall updates',
        # TODO: Make this configurable
        description => "If set, will poll Facebook every minute for updates.\n"
     }
 );

sub fail {
    my $msg = shift;
    # reset global state here
    BarnOwl::admin_message('Facebook Error', $msg);
    die("Facebook Error: $msg\n");
}

# We only load up when the conf file is present, to reduce resource
# usage.  Though, probably not by very much, so maybe a 'facebook-init'
# command would be more appropriate.

my $conffile = BarnOwl::get_config_dir() . "/facebook";

if (open(my $fh, "<", "$conffile")) {
    read_config($fh);
    close($fh);
}

sub read_config {
    my $fh = shift;
    my $raw_cfg = do {local $/; <$fh>};
    close($fh);

    my $cfg;
    if ($raw_cfg) {
        eval { $cfg = from_json($raw_cfg); };
        if($@) {
            fail("Unable to parse $conffile: $@");
        }
    } else {
        $cfg = {};
    }

    eval {
        $facebook_handle = BarnOwl::Module::Facebook::Handle->new($cfg);
    };
    if ($@) {
        BarnOwl::error($@);
        next;
    }
}

# Ostensibly here as a convenient shortcut for Perl hackery
sub facebook {
    $facebook_handle->facebook(@_);
}

# Should also add support for posting to other people's walls (this
# is why inline MESSAGE is not supported... yet).  However, see below:
# specifying USER is UI problematic.
BarnOwl::new_command('facebook' => \&cmd_facebook, {
    summary     => 'Post a status update to your wall from BarnOwl',
    usage       => 'facebook',
    description => 'Post a status update to your wall.'
});

# How do we allow people to specify the USER?
#BarnOwl::new_command('facebook-message' => \&cmd_facebook_direct, {
#    summary     => 'Send a Facebook message',
#    usage       => 'twitter-direct USER',
#    description => 'Send a private Facebook Message to USER.'
#});

BarnOwl::new_command('facebook-comment' => \&cmd_facebook_comment, {
    summary     => 'Comment on a wall post.',
    usage       => 'facebook-comment POSTID',
    description => 'Comment on a friend\'s wall post.  Using r is recommended.'
});

BarnOwl::new_command('facebook-auth' => \&cmd_facebook_auth, {
    summary     => 'Authenticate as a Facebook user.',
    usage       => 'facebook-auth URL',
    description => 'Authenticate as a Facebook user.  URL should be the page'
                ."\nFacebook redirects you to after OAuth login."
});

BarnOwl::new_command('facebook-poll' => \&cmd_facebook_poll, {
    summary     => 'Force a poll of Facebook.',
    usage       => 'facebook-poll',
    description => 'Get updates from Facebook.'
});

# XXX: UI: probably should bug out immediately if we're not logged in.

sub cmd_facebook {
    my $cmd = shift;
    BarnOwl::start_edit_win("What's on your mind?", sub{ facebook(shift) });
    # User prompt for other person's wall is "Write something..." which
    # we will ostensibly check for soon.
}

sub cmd_facebook_comment {
    my $cmd  = shift;
    my $postid   = shift;

    # XXX UI should give some (better) indication /which/ conversation
    # is being commented on
    BarnOwl::start_edit_win("Write a comment... ($postid)",
                            sub { $facebook_handle->facebook_comment($postid, shift) });
}

sub cmd_facebook_poll {
    my $cmd = shift;

    $facebook_handle->sleep(0);
    return;
}

sub cmd_facebook_auth {
    my $cmd = shift;
    my $url = shift;

    $facebook_handle->facebook_auth($url);
}

BarnOwl::filter(qw{facebook type ^facebook$});

1;
