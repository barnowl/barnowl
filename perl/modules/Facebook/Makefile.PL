use strict;
use warnings;

use inc::Module::Install;

# requires('Facebook::Graph');
requires('Lingua::EN::Keywords'); # maybe this should just be optional

barnowl_module('Facebook');

WriteAll;
