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

BarnOwl::new_variable_bool(
    'facebook:poll',
    {
        default => 1,
        summary => 'Poll Facebook for wall updates',
        # XXX: Make poll time configurable
        description => "If set, will poll Facebook every minute for updates.\n"
     }
 );

sub init {
    my $conffile = BarnOwl::get_config_dir() . "/facebook";
    my $cfg = {};
    if (open(my $fh, "<", "$conffile")) {
        my $raw_cfg = do {local $/; <$fh>};
        close($fh);

        eval { $cfg = from_json($raw_cfg); };
        if ($@) { BarnOwl::admin_message('Facebook', "Unable to parse $conffile: $@"); }
    }
    eval { $facebook_handle = BarnOwl::Module::Facebook::Handle->new($cfg); };
    if ($@) { BarnOwl::error($@); }
}

init();

# Should also add support for posting to other people's walls (this
# is why inline MESSAGE is not supported... yet).  However, see below:
# specifying USER is UI problematic.
BarnOwl::new_command('facebook' => \&cmd_facebook, {
    summary     => 'Post a status update to your wall from BarnOwl',
    usage       => 'facebook [USER]',
    description => 'Post a status update to your wall, or post on another user\'s wall. Autocomplete is supported.'
});

#BarnOwl::new_command('facebook-message' => \&cmd_facebook_direct, {
#    summary     => 'Send a Facebook message',
#    usage       => 'twitter-direct USER',
#    description => 'Send a private Facebook Message to USER.'
#});

BarnOwl::new_command('facebook-comment' => \&cmd_facebook_comment, {
    summary     => 'Comment on a wall post.',
    usage       => 'facebook-comment POST_ID',
    description => 'Comment on a friend\'s wall post.'
});

BarnOwl::new_command('facebook-auth' => \&cmd_facebook_auth, {
    summary     => 'Authenticate as a Facebook user.',
    usage       => 'facebook-auth [URL]',
    description => 'Authenticate as a Facebook user.  URL should be the page'
                ."\nFacebook redirects you to after OAuth login.  If no URL"
                ."\nis specified, output instructions for logging in."
});

BarnOwl::new_command('facebook-poll' => \&cmd_facebook_poll, {
    summary     => 'Force a poll of Facebook.',
    usage       => 'facebook-poll',
    description => 'Get updates (news, friends) from Facebook.'
});

sub cmd_facebook {
    my $cmd = shift;
    my $user = shift;

    return unless check_ready();

    BarnOwl::start_edit_win(
        defined $user ? "Write something to $user..." : "What's on your mind?",
        sub{ $facebook_handle->facebook($user, shift) }
    );
}

sub cmd_facebook_comment {
    my $cmd  = shift;
    my $post_id = shift;

    return unless check_ready();

    BarnOwl::start_edit_win(
        "Write a comment...",
        sub { $facebook_handle->facebook_comment($post_id, shift) }
    );
}

sub cmd_facebook_poll {
    my $cmd = shift;

    return unless check_ready();

    $facebook_handle->sleep(0);
    return;
}

sub cmd_facebook_auth {
    my $cmd = shift;
    my $url = shift;

    if ($facebook_handle->{logged_in}) {
        BarnOwl::message("Already logged in. (To force, run ':reload-module Facebook', or deauthorize BarnOwl.)");
        return;
    }

    $facebook_handle->facebook_auth($url);
}

sub check_ready {
    if (!$facebook_handle->{logged_in}) {
        BarnOwl::message("Need to login to Facebook first with ':facebook-auth'.");
        return 0;
    }
    return 1;
}

BarnOwl::filter(qw{facebook type ^facebook$});

sub complete_user { return keys %{$facebook_handle->{friends}}; }
BarnOwl::Completion::register_completer(facebook => sub { complete_user(@_) });

1;
