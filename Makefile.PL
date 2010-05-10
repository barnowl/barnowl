use strict;
use warnings;

use inc::Module::Install;

requires('Net::Twitter::Lite');

barnowl_module('Twitter');

WriteAll;
