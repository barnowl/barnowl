use warnings;
use strict;

=head1 NAME

BarnOwl::Module::Facebook::Handle

=head1 DESCRIPTION

Contains everything needed to send and receive messages from Facebook

=cut

package BarnOwl::Module::Facebook::Handle;

use Facebook::Graph;

use List::Util qw(reduce);

eval { require Lingua::EN::Keywords; };
if ($@) {
    *keywords = sub {
        # stupidly pick the longest one, and only return one.
        my $sentence = shift;
        $sentence =~ s/[[:punct:]]+/ /g;
        my @words = split(' ', lc($sentence));
        return () unless @words;
        return (reduce{ length($a) > length($b) ? $a : $b } @words,);
    };
} else {
    *keywords = \&Lingua::EN::Keywords::keywords;
}

use JSON;
use Date::Parse;
use POSIX;

use Scalar::Util qw(weaken);

use BarnOwl;
use BarnOwl::Message::Facebook;

our $app_id = 235537266461636; # for application 'barnowl'

# Unfortunately, Facebook does not offer a comment stream, in the same
# way we can get a post stream using the news feed.  This makes it a bit
# difficult to de-duplicate comments we have already seen.  We use a
# simple heuristic to fix this: we check if the comment's time is dated
# from before our last update, and don't re-post if it's dated before.
# Be somewhat forgiving, since it's better to duplicate a post than to
# drop one.  Furthermore, we must use Facebook's idea of time, since the
# server BarnOwl is running on may be desynchronized.  So we need to
# utilize Facebook's idea of time, not ours.  We do this by looking at
# all of the timestamps we see while processing an update, and take the
# latest one and increment it by one second.
#
# What properties do we get with this setup?
#
#   - We get comment updates only for the latest N posts on a news feed.
#   Any later ones, you have to use Facebook's usual mechanisms (e.g.
#   email notifications).
#
#   - Processing a poll is relatively expensive, since we have to
#   iterate over N new posts.  It might be worthwhile polling for new
#   comments less frequently than polling for new posts.

sub fail {
    my $self = shift;
    my $msg  = shift;
    undef $self->{facebook};
    die("[Facebook] Error: $msg\n");
}

sub new {
    my $class = shift;
    my $cfg = shift;

    my $self = {
        'cfg'  => $cfg,
        'facebook' => undef,

        # Ideally this should be done using Facebook realtime updates,
        # but we can't assume that the BarnOwl lives on a publically
        # addressable server (XXX maybe we can setup an option for this.)
        'last_friend_poll' => 0,
        'friend_timer' => undef,

        # Initialized with our 'time', but will be synced to Facebook
        # soon enough. (Subtractive amount is just to preseed with some
        # values.)
        'last_poll' => time - 60 * 60 * 24 * 2,
        'timer' => undef,

        # Message polling not implemented yet
        #'last_message_poll' => time,
        #'message_timer' => undef,

        # yeah yeah, inelegant, I know.  You can try using
        # $fb->authorize, but at time of writing (1.0300) they didn't support
        # the response_type parameter.
        # 'login_url' => 'https://www.facebook.com/dialog/oauth?client_id=235537266461636&scope=read_stream,read_mailbox,publish_stream,offline_access&redirect_uri=http://www.facebook.com/connect/login_success.html&response_type=token',
        # minified to fit in most terminal windows.
        'login_url' => 'http://goo.gl/yA42G',

        'logged_in' => 0,

        # would need another hash for topic de-dup
        'topics' => {},

        # deduplicated map of names to user ids
        'friends' => {},
    };

    bless($self, $class);

    $self->{facebook} = Facebook::Graph->new( app_id => $app_id );
    $self->facebook_do_auth;

    return $self;
}

=head2 sleep N

Stop polling Facebook for N seconds.

=cut

sub sleep {
    my $self  = shift;
    my $delay = shift;

    # prevent reference cycles
    my $weak = $self;
    weaken($weak);

    # Stop any existing timers.
    if (defined $self->{friend_timer}) {
        $self->{friend_timer}->stop;
        $self->{friend_timer} = undef;
    }
    if (defined $self->{timer}) {
        $self->{timer}->stop;
        $self->{timer} = undef;
    }
    if (defined $self->{message_timer}) {
        # XXX doesn't do anything right now
        $self->{message_timer}->stop;
        $self->{message_timer} = undef;
    }

    $self->{friend_timer} = BarnOwl::Timer->new({
        name     => "Facebook friend poll",
        after    => $delay,
        interval => 60 * 60 * 24,
        cb       => sub { $weak->poll_friends if $weak }
       });
    $self->{timer} = BarnOwl::Timer->new({
        name     => "Facebook poll",
        after    => $delay,
        interval => 90,
        cb       => sub { $weak->poll_facebook if $weak }
       });
    # XXX implement message polling
}

sub poll_friends {
    my $self = shift;

    return unless BarnOwl::getvar('facebook:poll') eq 'on';
    return unless $self->{logged_in};

    my $friends = eval { $self->{facebook}->fetch('me/friends'); };
    if ($@) {
        warn "Poll failed $@";
        return;
    }

    $self->{last_friend_poll} = time;
    $self->{friends} = {};

    for my $friend ( @{$friends->{data}} ) {
        if (defined $self->{friends}{$friend->{name}}) {
            # XXX We should try a little harder here, rather than just
            # tacking on a number.  Ideally, we should be able to
            # calculate some extra piece of information that the user
            # needs to disambiguate between the two users.  An old
            # version of Facebook used to disambiguate with your primary
            # network (so you might have Edward Yang (MIT) and Edward
            # Yang (Cambridge), the idea being that users in the same
            # network would probably have already disambiguated
            # themselves with middle names or nicknames.  We no longer
            # get network information, since Facebook axed that
            # information, but the Education/Work fields may still be
            # a reasonable approximation (but which one do you pick?!
            # The most recent one.)  Since getting this information
            # involves extra queries, there are also caching and
            # efficiency concerns.
            #   We may want a facility for users to specify custom
            # aliases for Facebook users, which are added into this
            # hash.  See also username support.
            warn "Duplicate friend name " . $friend->{name};
            my $name = $friend->{name};
            my $i = 2;
            while (defined $self->{friends}{$friend->{name} . ' ' . $i}) { $i++; }
            $self->{friends}{$friend->{name} . ' ' . $i} = $friend->{id};
        } else {
            $self->{friends}{$friend->{name}} = $friend->{id};
        }
    }

    # XXX We should also have support for usernames, and not just real
    # names. However, since this data is not returned by the friends
    # query, it would require a rather expensive set of queries. We
    # might try to preserve old data, but all-in-all it's a bit
    # complicated, so we don't bother.
}

sub poll_facebook {
    my $self = shift;

    #return unless ( time - $self->{last_poll} ) >= 60;
    return unless BarnOwl::getvar('facebook:poll') eq 'on';
    return unless $self->{logged_in};

    #BarnOwl::message("Polling Facebook...");

    # XXX Oh no! This blocks the user interface.  Not good.
    # Ideally, we should have some worker thread for polling facebook.
    # But BarnOwl is probably not thread-safe >_<

    my $old_topics = $self->{topics};
    $self->{topics} = {};

    my $updates = eval {
        $self->{facebook}
             ->query
             ->from("my_news")
             # Not using this, because we want to pick up comment
             # updates. We need to manually de-dup, though.
             # ->where_since( "@" . $self->{last_poll} )
             ->limit_results( 200 )
             ->request()
             ->as_hashref()
    };
    if ($@) {
        warn "Poll failed $@";
        return;
    }

    my $new_last_poll = $self->{last_poll};
    for my $post ( reverse @{$updates->{data}} ) {
        # No app invites, thanks! (XXX make configurable)
        if ($post->{type} eq 'link' && $post->{application}) {
            next;
        }

        # XXX Filtering out interest groups for now
        # A more reasonable strategy may be to show their
        # posts, but not the comments.
        if (defined $post->{from}{category}) {
            next;
        }

        # XXX Need to somehow access Facebook's user hiding
        # mechanism

        # There can be multiple recipients! Strange! Pick the first one.
        my $name    = $post->{to}{data}[0]{name} || $post->{from}{name};
        my $name_id = $post->{to}{data}[0]{id} || $post->{from}{id};
        my $post_id  = $post->{id};

        if (defined $old_topics->{$post_id}) {
            $self->{topics}->{$post_id} = $old_topics->{$post_id};
        } else {
            my @keywords = keywords($post->{name} || $post->{message});
            my $topic = $keywords[0] || 'personal';
            $topic =~ s/ /-/g;
            $self->{topics}->{$post_id} = $topic;
        }

        # Only handle post if it's new
        my $created_time = str2time($post->{created_time});
        if ($created_time >= $self->{last_poll}) {
            # XXX indexing is fragile
            my $msg = BarnOwl::Message->new(
                type      => 'Facebook',
                sender    => $post->{from}{name},
                sender_id => $post->{from}{id},
                name      => $name,
                name_id   => $name_id,
                direction => 'in',
                body      => $self->format_body($post),
                post_id   => $post_id,
                topic     => $self->get_topic($post_id),
                time      => asctime(localtime $created_time),
                # XXX The intent is to get the 'Comment' link, which also
                # serves as a canonical link to the post.  The {name}
                # field should equal 'Comment'.
                zsig      => $post->{actions}[0]{link},
               );
            BarnOwl::queue_message($msg);
        }

        # This will have funky interleaving of times (they'll all be
        # sorted linearly), but since we don't expect too many updates between
        # polls this is pretty acceptable.
        my $updated_time = str2time($post->{updated_time});
        if ($updated_time >= $self->{last_poll} && defined $post->{comments}{data}) {
            for my $comment ( @{$post->{comments}{data}} ) {
                my $comment_time = str2time($comment->{created_time});
                if ($comment_time < $self->{last_poll}) {
                    next;
                }
                my $msg = BarnOwl::Message->new(
                    type      => 'Facebook',
                    sender    => $comment->{from}{name},
                    sender_id => $comment->{from}{id},
                    name      => $name,
                    name_id   => $name_id,
                    direction => 'in',
                    body      => $comment->{message},
                    post_id    => $post_id,
                    topic     => $self->get_topic($post_id),
                    time      => asctime(localtime $comment_time),
                   );
                BarnOwl::queue_message($msg);
            }
        }
        if ($updated_time + 1 > $new_last_poll) {
            $new_last_poll = $updated_time + 1;
        }
    }
    # old_topics gets GC'd

    $self->{last_poll} = $new_last_poll;
}

sub format_body {
    my $self = shift;

    my $post = shift;

    # XXX implement optional URL minification
    if ($post->{type} eq 'status') {
        return $post->{message};
    } elsif ($post->{type} eq 'link' || $post->{type} eq 'video' || $post->{type} eq 'photo') {
        return $post->{name}
          . ($post->{caption} ? " (" . $post->{caption} . ")\n" : "\n")
          . $post->{link}
          . ($post->{description} ? "\n\n" . $post->{description} : "")
          . ($post->{message} ? "\n\n" . $post->{message} : "");
    } else {
        return "(unknown post type " . $post->{type} . ")";
    }
}

sub facebook {
    my $self = shift;

    my $user = shift;
    my $msg = shift;

    if (!defined $self->{facebook} || !$self->{logged_in}) {
        BarnOwl::admin_message('Facebook', 'You are not currently logged into Facebook.');
        return;
    }
    if (defined $user) {
        $user = $self->{friends}{$user} || $user;
        $self->{facebook}->add_post( $user )->set_message( $msg )->publish;
    } else {
        $self->{facebook}->add_post->set_message( $msg )->publish;
    }
    $self->sleep(0);
}

sub facebook_comment {
    my $self = shift;

    my $post_id = shift;
    my $msg = shift;

    $self->{facebook}->add_comment( $post_id )->set_message( $msg )->publish;
    $self->sleep(0);
}

sub facebook_auth {
    my $self = shift;

    my $url = shift;
    # http://www.facebook.com/connect/login_success.html#access_token=TOKEN&expires_in=0
    $url =~ /access_token=([^&]+)/; # XXX Ew regex

    $self->{cfg}->{token} = $1;
    if ($self->facebook_do_auth) {
        my $raw_cfg = to_json($self->{cfg});
        BarnOwl::admin_message('Facebook', "Add this as the contents of your ~/.owl/facebook file:\n$raw_cfg");
    }
}

sub facebook_do_auth {
    my $self = shift;
    if ( ! defined $self->{cfg}->{token} ) {
        BarnOwl::admin_message('Facebook', "Login to Facebook at ".$self->{login_url}
            . "\nand run command ':facebook-auth URL' with the URL you are redirected to.");
        return 0;
    }
    $self->{facebook}->access_token($self->{cfg}->{token});
    # Do a quick check to see if things are working
    my $result = eval { $self->{facebook}->fetch('me'); };
    if ($@) {
        BarnOwl::admin_message('Facebook', "Failed to authenticate! Login to Facebook at ".$self->{login_url}
            . "\nand run command ':facebook-auth URL' with the URL you are redirected to.");
        return 0;
    } else {
        my $name = $result->{'name'};
        BarnOwl::admin_message('Facebook', "Successfully logged in to Facebook as $name!");
        $self->{logged_in} = 1;
        $self->sleep(0); # start polling
        return 1;
    }
}

sub get_topic {
    my $self = shift;

    my $post_id = shift;

    return $self->{topics}->{$post_id} || 'personal';
}

1;
