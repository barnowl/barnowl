#!/usr/bin/env perl
use strict;
use warnings;

use Test::More qw(no_plan);

use BarnOwl;

=head1 DESCRIPTION

The class, instance, realm, opcode, sender and recipient of a zephyr are
chosen by whoever sent it, and the styles put them into ztext.  Check that
they are shown as they were sent and cannot change how the rest of the
message is drawn.  The zsig is ztext on purpose, so it is not escaped.

=cut

# What the real ones need a running zephyr connection or a clock for.
{
    no warnings 'redefine';
    *BarnOwl::zephyr_getrealm = sub () { 'ATHENA.MIT.EDU' };
    *BarnOwl::Style::Default::format_time = sub { '12:00' };
}
my $colorztext = 'off';
{
    no warnings 'redefine';
    my $real_getvar = \&BarnOwl::getvar;
    *BarnOwl::getvar = sub {
        return $colorztext if $_[0] eq 'colorztext';
        return $real_getvar->(@_);
    };
}

{
    package TestMessage;
    our @ISA = ('BarnOwl::Message::Zephyr');
    # The real one asks the filter code, which needs a full message.
    sub is_personal { return shift->{personal}; }

    # A protocol other than zephyr, with names it chose itself.
    package TestOther;
    our @ISA = ('BarnOwl::Message');
    sub is_personal { return shift->{personal}; }
    sub context { return shift->{network}; }
    sub subcontext { return shift->{room}; }
    sub long_sender { return shift->{from}; }
    sub login_extra { return shift->{status}; }
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
    my $z = shift;
    return BarnOwl::ztext_stylestrip($z);
}

sub other {
    my %f = (
        type      => 'irc',
        direction => 'in',
        login     => 'none',
        network   => 'net',
        room      => '#room',
        sender    => 'nick',
        from      => 'nick!user@host.example',
        body      => 'hi',
        @_);
    return bless {%f}, 'TestOther';
}

my $default = 'BarnOwl::Style::Default';
my $oneline = 'BarnOwl::Style::OneLine';

# Ordinary names are drawn as before: the same text, and for names without an
# @ the same ztext.
is($default->chat_header(message(class => 'help', instance => 'urgent')),
   'help / urgent / @b{alice}  12:00  (zsig)',
   'ordinary class message');
is(plain($default->chat_header(message(sender => 'a@b.example', realm => 'ZONE.EXAMPLE',
                                       opcode => 'auto', instance => 'x'))),
   'message / x / a@b.example {ZONE.EXAMPLE} [auto]  12:00  (zsig)',
   'foreign realm and opcode');

# Hostile values for every field, class messages.
my @hostile = (
    '@b(x', '@color(red)x', '@bold{', 'a@', '@@', '@b(x)@b(', 'x)@b(y', 'a}b', 'a]b>c',
    'a}b]c>d)e@b(',
);
my $n = 0;
for my $h (@hostile) {
    my %f = (class => "c$h", instance => "i$h", opcode => "o$h",
             realm => "r$h", sender => "s$h\@ATHENA.MIT.EDU");
    my $m = message(%f);
    is(plain($default->format_message($m)),
       "c$h / i$h / s$h {r$h} [o$h]  12:00  (zsig)\n    hi",
       "class message, hostile fields: $h");

    is(plain($default->format_message(message(personal => 1, direction => 'out',
                                              recipient => "r$h\@ATHENA.MIT.EDU"))),
       "Zephyr sent to r$h  12:00  (zsig)\n    hi",
       "personal out, hostile recipient: $h");
    is(plain($default->format_message(message(personal => 1, sender => "s$h\@ATHENA.MIT.EDU"))),
       "Zephyr from s$h  12:00  (zsig)\n    hi",
       "personal in, hostile sender: $h");
    my $context = BarnOwl::quote(BarnOwl::Message::Zephyr::context_reply_cmd("c$h", "i$h"));
    is(plain($default->format_message(message(personal => 1, class => "c$h", instance => "i$h"))),
       "Zephyr [$context] from alice  12:00  (zsig)\n    hi",
       "personal in, hostile class and instance: $h");
    is(plain($default->format_message(message(auth => 'NO', class => "c$h", instance => "i$h",
                                              sender => "s$h\@ATHENA.MIT.EDU"))),
       "c$h / i$h / UNAUTH: s$h  12:00  (zsig)\n    hi",
       "unauthenticated class message, hostile fields: $h");
    is(plain($default->format_message(message(auth => 'NO', personal => 1,
                                              class => "c$h", instance => "i$h",
                                              sender => "s$h\@ATHENA.MIT.EDU"))),
       "Zephyr [$context] from UNAUTH: s$h  12:00  (zsig)\n    hi",
       "unauthenticated personal, hostile fields: $h");

    is(plain($default->format_message(message(personal => 1, direction => 'out',
                                              opcode => 'ping',
                                              sender => "s$h\@ATHENA.MIT.EDU"))),
       "PING from s$h",
       "ping, hostile sender: $h");
    is(plain($default->format_message(message(login => 'login', hostname => "H$h",
                                              sender => "s$h\@ATHENA.MIT.EDU",
                                              fields => ['h', 'x', 'tty']))),
       "LOGIN for s$h (" . lc("H$h") . " tty) 12:00",
       "login, hostile sender and host: $h");
}

# The sender is bold, and only the sender, even when it holds a closing delimiter.
is($default->chat_header(message(sender => 'a}b@ATHENA.MIT.EDU')),
   'message / personal / @b[a}b]  12:00  (zsig)',
   'closing brace in sender picks another delimiter');
is($default->chat_header(message(sender => '@b(x@ATHENA.MIT.EDU')),
   'message / personal / @b{@@b(x}  12:00  (zsig)',
   '@ in sender is escaped inside the bold');
is($default->chat_header(message(sender => 'a}b]c>d)e@ATHENA.MIT.EDU')),
   'message / personal / a}b]c>d)e  12:00  (zsig)',
   'sender holding all four closers is not bold');
is($default->chat_header(message(instance => '@b(x')),
   'message / @@b(x / @b{alice}  12:00  (zsig)',
   'the instance cannot start a ztext command');

# A zsig is ztext on purpose and is not escaped.  Control characters in it are still
# made visible, and with colorztext on a colour reset follows it.
is($default->chat_header(message(zsig => '@b(Name)')),
   'message / personal / @b{alice}  12:00  (@b(Name))',
   'zsig keeps its ztext');
is(plain($default->chat_header(message(zsig => '@b(Name) @i(here)'))),
   'message / personal / alice  12:00  (Name here)',
   'zsig formatting is not shown as markup');
is($default->chat_header(message(zsig => "z\x01")),
   'message / personal / @b{alice}  12:00  (z@b(^A))',
   'control character in a zsig is made visible');
$colorztext = 'on';
is($default->chat_header(message(zsig => '@color(red)x')),
   'message / personal / @b{alice}  12:00  (@color(red)x@color[default])',
   'zsig colour is reset with colorztext on');
$colorztext = 'off';

# Other protocols give names and URLs that were never meant to be ztext.
for my $h (@hostile) {
    is(plain($default->format_message(other(network => "n$h", room => "r$h", sender => "s$h",
                                            from => "f$h"))),
       "n$h / r$h / s$h  12:00  (f$h)\n    hi",
       "other protocol, hostile fields: $h");
    is(plain($default->format_message(other(personal => 1, sender => "s$h", from => "f$h"))),
       "Irc from s$h  12:00  (f$h)\n    hi",
       "other protocol personal, hostile fields: $h");
    is(plain($default->format_message(other(login => 'login', sender => "s$h", status => "st$h"))),
       "LOGIN for s$h (st$h) 12:00",
       "other protocol login, hostile sender and status: $h");
}
is($default->chat_header(other(from => 'nick!user@host.example')),
   'net / #room / @b{nick}  12:00  (nick!user@@host.example)',
   'a name with an @ is escaped');

# OneLine: the columns are escaped after they have been cut to width.
sub oneline_prefix {
    my ($dirsym, @cols) = @_;
    return sprintf('%s %-13.13s %-11.11s %-12.12s ', $dirsym, @cols);
}
for my $h (@hostile) {
    is(plain($oneline->format_message(message(class => "c$h", instance => "i$h",
                                              sender => "s$h\@ATHENA.MIT.EDU"))),
       oneline_prefix('<', "c$h", "i$h", "s$h") . 'hi',
       "oneline, hostile columns: $h");
    is(plain($oneline->format_message(message(personal => 1, direction => 'out',
                                              recipient => "r$h\@ATHENA.MIT.EDU"))),
       oneline_prefix('>', 'zephyr', '', "r$h") . 'hi',
       "oneline personal out, hostile recipient: $h");
    is(plain($oneline->format_message(message(login => 'logout', hostname => "H$h",
                                              sender => "s$h\@ATHENA.MIT.EDU",
                                              fields => ['h', 'x', 'tty']))),
       oneline_prefix('<', 'zephyr', 'LOGOUT', "s$h") . 'at ' . lc("H$h") . ' tty',
       "oneline logout, hostile sender and host: $h");
    is(plain($oneline->format_message(message(personal => 1, opcode => 'ping',
                                              sender => "s$h\@ATHENA.MIT.EDU"))),
       oneline_prefix('<', 'zephyr', 'PING', "s$h"),
       "oneline ping, hostile sender: $h");
}
is(plain($oneline->format_message(message(instance => '@' x 40))),
   oneline_prefix('<', 'message', '@' x 40, 'alice') . 'hi',
   'oneline column stays 11 wide when it is all @');

# helpers
is($default->escape_ztext('a@b@@c'), 'a@@b@@@@c', 'escape_ztext doubles every @');
is($default->escape_ztext(undef), '', 'escape_ztext of undef');
is($default->bold_ztext('x'), '@b{x}', 'bold_ztext default pair');
is($default->bold_ztext('a}b'), '@b[a}b]', 'bold_ztext skips } ');
is($default->bold_ztext('a}b]c'), '@b<a}b]c>', 'bold_ztext skips } and ]');
is($default->bold_ztext('a}b]c>d'), '@b(a}b]c>d)', 'bold_ztext skips } ] >');
is($default->bold_ztext('a}b]c>d)e'), 'a}b]c>d)e', 'bold_ztext leaves it unbold if all four occur');


1;
