#!/usr/bin/env perl
use strict;
use warnings;

use Test::More qw(no_plan);

use BarnOwl;

=head1 DESCRIPTION

The sender and recipient of a message, and the host and tty of a login,
are chosen by whoever sent it.  The default style gave them to the
terminal as they were, and format_message only made the control characters
other than newline visible, so a sender with a newline in its name drew a
second line that looks like part of the message.  Check that every such
header field is on one line, with the newline made visible, and that the
body is left alone.

=cut

# What the real ones need a running zephyr connection or a clock for.
{
    no warnings 'redefine';
    *BarnOwl::zephyr_getrealm = sub () { 'ATHENA.MIT.EDU' };
    *BarnOwl::Style::Default::format_time = sub { '12:00' };
    my $real_getvar = \&BarnOwl::getvar;
    *BarnOwl::getvar = sub {
        return 'off' if $_[0] eq 'colorztext';
        return $real_getvar->(@_);
    };
}

{
    package TestMessage;
    our @ISA = ('BarnOwl::Message::Zephyr');
    # The real one asks the filter code, which needs a full message.
    sub is_personal { return shift->{personal}; }
}

sub message {
    my %f = (
        type      => 'zephyr',
        direction => 'in',
        login     => 'none',
        class     => 'message',
        instance  => 'personal',
        opcode    => '',
        realm     => 'ATHENA.MIT.EDU',
        auth      => 'YES',
        sender    => 'alice@ATHENA.MIT.EDU',
        recipient => 'bob@ATHENA.MIT.EDU',
        zsig      => 'zsig',
        body      => 'hi',
        @_);
    return bless {%f}, 'TestMessage';
}

sub plain {
    return BarnOwl::ztext_stylestrip(shift);
}

my $default = 'BarnOwl::Style::Default';
my $oneline = 'BarnOwl::Style::OneLine';

# Ordinary names are drawn as before.
is(plain($default->format_message(message())),
   "message / personal / alice  12:00  (zsig)\n    hi",
   'ordinary class message');
is(plain($default->format_message(message(personal => 1))),
   "Zephyr from alice  12:00  (zsig)\n    hi",
   'ordinary personal message');

# Name, what the name is shown as.
my @hostile = (
    ["a\nb",                 'a^Jb'],
    ["\nb",                  '^Jb'],
    ["a\n",                  'a^J'],
    ["a\n\nb",               'a^J^Jb'],
    ["a\r\nb",               'a^M^Jb'],
    ["alice\nFrom: root",    'alice^JFrom: root'],
    ["a\n\x{1b}[2Jb",        'a^J^[[2Jb'],
    ["a\n\x01b",             'a^J^Ab'],
);

for my $t (@hostile) {
    my ($name, $shown) = @$t;
    my $label = $shown;

    # Everything but the body of the message is the header: one line.
    my $out = $default->format_message(message(sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), "message / personal / $shown  12:00  (zsig)\n    hi",
       "class message, sender: $label");
    is(scalar(() = $out =~ /\n/g), 1, "class message, sender, one newline: $label");

    $out = $default->format_message(message(personal => 1, sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), "Zephyr from $shown  12:00  (zsig)\n    hi",
       "personal in, sender: $label");
    is(scalar(() = $out =~ /\n/g), 1, "personal in, sender, one newline: $label");

    $out = $default->format_message(message(personal => 1, auth => 'NO',
                                            sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), "Zephyr from UNAUTH: $shown  12:00  (zsig)\n    hi",
       "unauthenticated personal, sender: $label");
    is(scalar(() = $out =~ /\n/g), 1, "unauthenticated personal, one newline: $label");

    $out = $default->format_message(message(auth => 'NO', sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), "message / personal / UNAUTH: $shown  12:00  (zsig)\n    hi",
       "unauthenticated class message, sender: $label");
    is(scalar(() = $out =~ /\n/g), 1, "unauthenticated class message, one newline: $label");

    $out = $default->format_message(message(personal => 1, direction => 'out',
                                            recipient => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), "Zephyr sent to $shown  12:00  (zsig)\n    hi",
       "personal out, recipient: $label");
    is(scalar(() = $out =~ /\n/g), 1, "personal out, recipient, one newline: $label");

    # Login, logout and ping messages have no body: no newline at all.
    $out = $default->format_message(message(login => 'login', sender => "$name\@ATHENA.MIT.EDU",
                                            hostname => 'host', fields => ['h', 'x', 'tty']));
    is(plain($out), "LOGIN for $shown (host tty) 12:00", "login, sender: $label");
    unlike($out, qr/\n/, "login, sender, no newline: $label");

    $out = $default->format_message(message(login => 'logout', sender => 'alice@ATHENA.MIT.EDU',
                                            hostname => 'host', fields => ['h', 'x', "t\nty"]));
    is(plain($out), "LOGOUT for alice (host t^Jty) 12:00", "logout, tty with newline");
    unlike($out, qr/\n/, "logout, tty with newline, no newline");

    $out = $default->format_message(message(personal => 1, opcode => 'ping',
                                            sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), "PING from $shown", "ping, sender: $label");
    unlike($out, qr/\n/, "ping, sender, no newline: $label");
}

# The newline is shown as the other control characters are: bold ^J.
is($default->format_message(message(sender => "a\nb\@ATHENA.MIT.EDU")),
   "message / personal / \@b{a\@b(^J)b}  12:00  (zsig)\n    hi",
   'newline in a sender is shown as bold ^J');

# The body is not changed: its newlines stay, and are indented.
is(plain($default->format_message(message(body => "one\ntwo\n\nthree"))),
   "message / personal / alice  12:00  (zsig)\n    one\n    two\n\n    three",
   'newlines in the body are kept');

# OneLine: one line, with the control characters shown as ?.
sub oneline_prefix {
    my ($dirsym, @cols) = @_;
    return sprintf('%s %-13.13s %-11.11s %-12.12s ', $dirsym, @cols);
}
for my $t (@hostile) {
    my ($name, $shown) = @$t;
    (my $q = $name) =~ s/[[:cntrl:]]/?/g;
    my $label = $shown;

    my $out = $oneline->format_message(message(sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), oneline_prefix('<', 'message', 'personal', $q) . 'hi',
       "oneline class message, sender: $label");
    unlike($out, qr/\n/, "oneline class message, sender, no newline: $label");

    $out = $oneline->format_message(message(personal => 1, sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), oneline_prefix('<', 'zephyr', '', $q) . 'hi',
       "oneline personal in, sender: $label");
    unlike($out, qr/\n/, "oneline personal in, sender, no newline: $label");

    $out = $oneline->format_message(message(personal => 1, direction => 'out',
                                            recipient => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), oneline_prefix('>', 'zephyr', '', $q) . 'hi',
       "oneline personal out, recipient: $label");
    unlike($out, qr/\n/, "oneline personal out, recipient, no newline: $label");

    $out = $oneline->format_message(message(login => 'login', sender => "$name\@ATHENA.MIT.EDU",
                                            hostname => 'host', fields => ['h', 'x', 'tty']));
    is(plain($out), oneline_prefix('<', 'zephyr', 'LOGIN', $q) . 'at host tty',
       "oneline login, sender: $label");
    unlike($out, qr/\n/, "oneline login, sender, no newline: $label");

    $out = $oneline->format_message(message(login => 'logout', sender => 'alice@ATHENA.MIT.EDU',
                                            hostname => 'host', fields => ['h', 'x', "t\nty"]));
    is(plain($out), oneline_prefix('<', 'zephyr', 'LOGOUT', 'alice') . 'at host t?ty',
       "oneline logout, tty with newline: $label");
    unlike($out, qr/\n/, "oneline logout, tty with newline, no newline: $label");

    $out = $oneline->format_message(message(personal => 1, opcode => 'ping',
                                            sender => "$name\@ATHENA.MIT.EDU"));
    is(plain($out), oneline_prefix('<', 'zephyr', 'PING', $q),
       "oneline ping, sender: $label");
    unlike($out, qr/\n/, "oneline ping, sender, no newline: $label");
}
