use strict;
use warnings;

use inc::Module::Install;

requires('Net::Twitter');

barnowl_module('Twitter');

WriteAll;
