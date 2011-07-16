use strict;
use warnings;

use inc::Module::Install;

# requires('Facebook::Graph');
# Facebook::Graph deps:
requires('AnyEvent::HTTP'); # async only
requires('Any::Moose' => '0.13');
requires('DateTime' => '0.61');
requires('DateTime::Format::Strptime' => '1.4000');
requires('JSON' => '2.16');
requires('MIME::Base64::URLSafe' => '0.01');
requires('Ouch' => '0.0400');
requires('URI' => '1.54');
requires('URI::Encode' => '0.02');
# End deps
requires('Lingua::EN::Keywords'); # maybe this should just be optional

barnowl_module('Facebook');

WriteAll;
