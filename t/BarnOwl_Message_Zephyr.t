#!/usr/bin/env perl
use strict;
use warnings;

use Test::More qw(no_plan);

use BarnOwl;

# Test that casefold_principal gets it right.
#
# NB(jgross): I got it wrong enough times writing it that I figured
# putting in a test case for it was worth it.

sub combine_user_realm {
    my ($user, $realm) = @_;
    return $user . '@' . $realm if defined $realm and $realm ne '';
    return $user;
}

sub test_casefold_principal {
    my ($user, $realm) = @_;
    is(BarnOwl::Message::Zephyr::casefold_principal(combine_user_realm($user, $realm)),
       combine_user_realm(lc($user), uc($realm)));
}

test_casefold_principal('', '');
test_casefold_principal('FOO', '');
test_casefold_principal('FOO', 'athena.mit.edu');
test_casefold_principal('FOO@BAR', 'athena.mit.edu');
test_casefold_principal('', 'athena.mit.edu');

1;

