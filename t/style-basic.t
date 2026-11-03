#!/usr/bin/env perl
use strict;
use warnings;

use Test::More qw(no_plan);

use BarnOwl;

# The basic style is an alias for the default style: style.c calls
# format_message on it, so it has to format messages the same way.

can_ok('BarnOwl::Style::Basic', 'format_message');
can_ok('BarnOwl::Style::Basic', 'description');

is(BarnOwl::Style::Basic->description,
   "Compatibility alias for the default style",
   "basic style description");

ok(grep({ $_ eq 'basic' } @{BarnOwl::all_styles()}), 'basic style is registered');

{
    package TestMessage;
    our @ISA = ('BarnOwl::Message::Zephyr');
    sub is_personal { return shift->{personal}; }
    sub pretty_sender { return shift->sender; }
}

sub message {
    return TestMessage->new(
        type => 'zephyr',
        direction => 'in',
        login => 'none',
        class => 'message',
        instance => 'personal',
        opcode => '',
        sender => 'bob',
        recipient => 'alice',
        unix_time => 0,
        zsig => 'a zsig',
        body => "hello\nworld",
        @_,
    );
}

my %messages = (
    'class message'     => message(personal => 0, class => 'test', instance => 'inst'),
    'incoming personal' => message(personal => 1),
    'outgoing personal' => message(personal => 1, direction => 'out'),
    'ping'              => message(personal => 1, opcode => 'ping'),
    'admin'             => message(personal => 0, type => 'admin', header => 'ADMIN'),
    'login'             => message(personal => 0, login => 'login', hostname => 'test.host', fields => ['test.host', '', 'tty']),
    'hostile body'      => message(personal => 1, body => "a)>}]\@b(\n\cA\@b)"),
);

for my $name (sort keys %messages) {
    my $m = $messages{$name};
    my $basic = BarnOwl::Style::Basic->format_message($m);
    ok(defined($basic) && length($basic), "basic style formats a $name");
    is($basic, BarnOwl::Style::Default->format_message($m),
       "basic style formats a $name like the default style");
}

1;

