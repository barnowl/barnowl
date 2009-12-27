use strict;
use warnings;

package BarnOwl::Module::Twitter::Completion;

my %users;

sub complete_account {
    my @nicks = grep {defined} map {$_->nickname} @BarnOwl::Module::Twitter::twitter_handles;
}

sub complete_user {
    my $service = shift;
    if(!$service) {
        return map {keys %$_} values %users;
    }
    return keys %{$users{$service} || {}};
}

sub on_message {
    my $m = shift;
    return unless $m->type eq 'Twitter';

    $users{$m->service}{$m->recipient} = 1;
    $users{$m->service}{$m->sender} = 1;
}

sub complete_twitter {
    my $ctx = shift;
    return unless $ctx->word == 1;
    return complete_account();
}

sub complete_twitter_user_account {
    my $ctx = shift;
    if ($ctx->word == 1) {
        my $nick = $ctx->words->[2];
        if ($nick) {
            my  @completions = eval {
                complete_user(BarnOwl::Module::Twitter::find_account($nick)->{cfg}{service});
            };
            return @completions if (!$@);
        }
        return complete_user();
    } elsif ($ctx->word == 2) {
        return complete_account();
    }
}

BarnOwl::Completion::register_completer('twitter' => \&complete_twitter);
BarnOwl::Completion::register_completer('twitter-direct' => \&complete_twitter_user_account);
BarnOwl::Completion::register_completer('twitter-atreply' => \&complete_twitter_user_account);
BarnOwl::Completion::register_completer('twitter-unfollow' => \&complete_twitter_user_account);

$BarnOwl::Hooks::newMessage->add("BarnOwl::Module::Twitter::Completion::on_message");
