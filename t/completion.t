#!/usr/bin/env perl
use strict;
use warnings;

use Test::More qw(no_plan);

use File::Basename;
BEGIN {require (dirname($0) . "/mock.pl");};

use BarnOwl::Complete::Filter qw(complete_filter_expr);

=head1 DESCRIPTION

Basic tests for tab-completion functionality.

=cut

sub test_tokenize {
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    my $before_point = shift;
    my $after_point = shift;
    
    my $ctx = BarnOwl::Completion::Context->new($before_point,
                                                $after_point);
    is($ctx->line, $before_point . $after_point);
    is($ctx->point, length $before_point);

    test_ctx($ctx, @_);
}

sub test_ctx {
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    my $ctx = shift;

    my $words = shift;
    my $word = shift;
    my $word_point = shift;

    my $word_start = shift;
    my $word_end   = shift;

    is_deeply($ctx->words, $words);
    if (defined($word)) {
        is($ctx->word, $word, "Correct current word.");
        is($ctx->word_point, $word_point, "Correct point within word.");
        is($ctx->word_start, $word_start, "Correct start of word");
        is($ctx->word_end,   $word_end, "Correct end of word");
    }
}

sub test_shift {
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    my $before_point = shift;
    my $after_point = shift;
    my $shift = shift;
    
    my $ctx = BarnOwl::Completion::Context->new($before_point,
                                                $after_point);
    $ctx = $ctx->shift_words($shift);

    test_ctx($ctx, @_);
}


isa_ok(BarnOwl::Completion::Context->new('Hello, W', 'orld'), 'BarnOwl::Completion::Context');

no warnings 'qw';
test_tokenize('Hello, W', 'orld',
              [qw(Hello, World)], 1, 1, 7, 12);

test_tokenize('Hello, World', '',
              [qw(Hello, World)], 1, 5, 7, 12);

test_tokenize(" \t Hello, World", '',
              [qw(Hello, World)], 1, 5, 10, 15);

test_tokenize('', '',
              [''], 0, 0, 0, 0);

test_tokenize('   ', '',
              [''], 0, 0, 3, 3);

test_tokenize('Hello', 'World',
              [qw(HelloWorld)], 0, 5, 0, 10);

test_tokenize(' Hello', 'World',
              [qw(HelloWorld)], 0, 5, 1, 11);

test_tokenize('lorem ipsum dolor ', 'sit amet',
              [qw(lorem ipsum dolor sit amet)],
              3, 0, 18, 21);

test_tokenize(q{error "ls -l failed"}, q{},
              ['error', 'ls -l failed'],
              1, 12, 6, 20);

test_tokenize(q{"a long"' word'}, q{},
              ['a long word']);

test_tokenize(q{"'"}, q{}, [q{'}], 0, 1, 0, 3);

test_tokenize(q{"Hello, }, q{World"},
              [q{Hello, World}],
              0, 7, 0, 14);

test_tokenize(q{But 'Hello, }, q{World'},
              ['But', q{Hello, World}],
              1, 7, 4, 18);

test_tokenize(q{But "Hello, }, q{World"''''''""},
              ['But', q{Hello, World}],
              1, 7, 4, 26);

test_tokenize(q{}, q{''Hello},
              ['Hello'],
              0, 0, 0, 7);

test_tokenize(q{"Hello, }, q{World},
              [q{Hello, World}],
              0, 7, 0, 13);

test_tokenize(q{Hello    }, q{World},
              [qw{Hello World}],
              1, 0, 9, 14);

test_tokenize(q{Hello '' ""}, q{ World},
              ["Hello", '', '', 'World'],
              2, 0, 9, 11);

test_tokenize(q{zwrite -c }, q{},
              [qw(zwrite -c), ''],
              2, 0, 10, 10);

# It's not entirely clear what we should do here. Make a test for the
# current behavior, so we'll notice if it changes.
test_tokenize(q{Hello }, q{ World},
              [qw(Hello World)],
              1, -1, 7, 12);

## Test Context::shift
test_shift('lorem ipsum dolor ', 'sit amet', 0,
           [qw(lorem ipsum dolor sit amet)],
           3, 0, 18, 21);

test_shift('lorem ipsum dolor ', 'sit amet', 1,
           [qw(ipsum dolor sit amet)],
           2, 0, 12, 15);

test_shift('lorem ipsum dolor ', 'sit amet', 2,
           [qw(dolor sit amet)],
           1, 0, 6, 9);

test_shift('lorem ipsum dolor ', 'sit amet', 3,
           [qw(sit amet)],
           0, 0, 0, 3);

eval {
    my $before_point = 'lorem ipsum dolor';
    my $after_point = 'sit amet';
    my $shift = 4;

    my $ctx = BarnOwl::Completion::Context->new($before_point,
                                                $after_point);
    $ctx = $ctx->shift_words($shift);
};
like($@, qr/^Context::shift: Unable to shift /, "Correctly die when shifting away the point");

## Test common_prefix

is(BarnOwl::Completion::common_prefix(qw(a b)), '');
is(BarnOwl::Completion::common_prefix(qw(a aa)), 'a');

is(BarnOwl::Completion::common_prefix(qw(aa)), 'aa');

is(BarnOwl::Completion::common_prefix(qw(a ab abc)), 'a');

is(BarnOwl::Completion::common_prefix(qw(abc abcd)), 'abc');

is(BarnOwl::Completion::common_prefix(qw(abc abc)), 'abc');

is(BarnOwl::Completion::common_prefix('a', ''), '');

## Test complete_flags

use BarnOwl::Completion::Util qw(complete_flags);

# dummy complete_zwrite
sub complete_zwrite {
    my $ctx = shift;
    return complete_flags($ctx,
                          [qw(-n -C -m)],
                          {
                              "-c" => sub {qw(nelhage nethack sipb help)},
                              "-i" => sub {qw()},
                              "-r" => sub {qw(ATHENA.MIT.EDU ZONE.MIT.EDU ANDREW.CMU.EDU)},
                              "-O" => sub {qw()},
                          },
                          sub {qw(nelhage asedeno geofft)});
}

sub test_complete {
    my $before = shift;
    my $after = shift;
    my $words = shift;
    my $complete = shift || \&complete_zwrite;
    
    my $ctx = BarnOwl::Completion::Context->new($before, $after);

    local $Test::Builder::Level = $Test::Builder::Level + 1;

    my @got = $complete->($ctx);
    is_deeply([sort @got], [sort @$words]);
}

test_complete('zwrite -c ', '', [qw(nelhage nethack sipb help)]);

test_complete('zwrite -c nelhage', '', [qw(nelhage nethack sipb help)]);

test_complete('zwrite -c nelhage -i ', '', [qw()]);

test_complete('zwrite -c nelhage ', '',
              [qw(-n -C -m -i -r -O nelhage asedeno geofft)]);

test_complete('zwrite -c nelhage ', '-',
              [qw(-n -C -m -i -r -O nelhage asedeno geofft)]);

test_complete('zwrite -c nelhage -- ', '',
              [qw(nelhage asedeno geofft)]);

sub complete_word {
    my $ctx = shift;
    return complete_flags($ctx,
                          [qw(-a -b -c)],
                          {
                              "-d" => sub {qw(some words for completing)},
                          },
                          sub {$_[1]},
                          repeat_flags => 1);
}

test_complete('cmd -a -d foo -c hello ','',
              [qw(-a -b -c -d 1)], \&complete_word);

test_complete('cmd -a -d foo -c ','',
              [qw(-a -b -c -d 0)], \&complete_word);

# Test that words after -- are counted properly.
test_complete('cmd -- hi there ','',
              [qw(2)], \&complete_word);

test_complete('cmd --','',
              [qw(-a -b -c -d 0)], \&complete_word);

test_complete('cmd -- ','',
              [qw(0)], \&complete_word);

test_complete('cmd foo -- ','',
              [qw(1)], \&complete_word);

test_complete('cmd foo -- bar ','',
              [qw(2)], \&complete_word);


# Test the filter expression completer

sub toplevel_filter_expect {
    return [sort(keys %BarnOwl::Complete::Filter::filter_cmds, qw[( not true false])];
}

test_complete('', '',
              toplevel_filter_expect(),
              \&complete_filter_expr);

test_complete('not ', '',
              toplevel_filter_expect(),
              \&complete_filter_expr);

test_complete('true ', '',
              [qw[and or]],
              \&complete_filter_expr);

test_complete('( true ', '',
              [qw[and or )]],
              \&complete_filter_expr);

test_complete('( body static and body analysis and not false and class davidben and ( instance python or instance hotd ', '',
              [qw[and or )]],
              \&complete_filter_expr);

test_complete('type ', '',
              [qw[admin aim zephyr]],
              \&complete_filter_expr);

test_complete('direction ', '',
              [qw[in out none]],
              \&complete_filter_expr);

test_complete('login ', '',
              [qw[login logout none]],
              \&complete_filter_expr);

test_complete('deleted ', '',
              [qw[true false]],
              \&complete_filter_expr);

# Test complete_files
use BarnOwl::Completion::Util qw(complete_file);
use File::Temp;
use File::Path qw(mkpath);

my $tmpdir = File::Temp::tempdir(CLEANUP => 1);

# Make sure $tmpdir does not have a trailing /
$tmpdir =~ s{/$}{};
$ENV{HOME} = $tmpdir;

sub touch {
    my $path = shift;
    system("touch", "$path");
}

mkpath("$tmpdir/.owl/",
       "$tmpdir/.owl/modules/",
       "$tmpdir/Public/",
       "$tmpdir/Private/",
       "$tmpdir/.ours",
       "$tmpdir/www",
     {mode => 0700});
touch("$tmpdir/.zephyr.subs");
touch("$tmpdir/wheee");
touch("$tmpdir/.owl/startup");

sub completion_value {
    my $c = shift;
    return $c unless ref($c) eq 'ARRAY';
    return $c->[1];
}

sub test_file {
    my $spec  = shift;
    my $pfx   = shift;
    my $dirs  = shift;
    my $files = shift;

    my $expect = [ sort {$a->[1] cmp $b->[1]} 
        ((map {["$_/", defined($pfx)?"$pfx/$_/":"$_/", 0]} @$dirs),
         (map {["$_",  defined($pfx)?"$pfx/$_" :$_   , 1]} @$files))
       ];

    local $Test::Builder::Level = $Test::Builder::Level + 1;

    my @got = complete_file($spec);

    @got = grep {completion_value($_) =~ m{^\Q$spec\E}} @got;
    @got = sort {completion_value($a) cmp completion_value($b)} @got;

    use Data::Dumper;
    is_deeply(\@got, $expect);
}

is_deeply([complete_file("~")], [["~/", "~/", 0]]);

END { chdir("/"); }
chdir($tmpdir);
test_file("$tmpdir/", $tmpdir,
          [qw(Public Private www)],
          [qw(wheee)]);

test_file("./", ".",
          [qw(Public Private www)],
          [qw(wheee)]);

test_file("", undef, [qw(Public Private www)], [qw(wheee)]);

test_file("./.owl/", "./.owl",
          [qw(modules)],
          [qw(startup)]);

test_file("~/", "~",
          [qw(Public Private www)],
          [qw(wheee)]);

test_file("P", undef, [qw(Public Private)], []);

test_file("$tmpdir/.", $tmpdir,
          [qw(. .. .owl .ours)],
          [qw(.zephyr.subs)]);
1;

