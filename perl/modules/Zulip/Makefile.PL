use strict;
use warnings;

use inc::Module::Install;

requires('AnyEvent::HTTP');
requires('AnyEvent::TLS');
requires('JSON');
requires('URI');
requires('URI::Encode');
requires('MIME::Base64');

barnowl_module('Zulip');

WriteAll;
