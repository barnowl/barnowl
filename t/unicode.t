#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use Encode;

use Test::More qw(no_plan);

use BarnOwl;

my $unicode = "“hello”";

ok(Encode::is_utf8($unicode));
ok(Encode::is_utf8(BarnOwl::wordwrap($unicode, 100)));

is(BarnOwl::wordwrap($unicode, 100), $unicode);
