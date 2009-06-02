use strict;
use warnings;

package BarnOwl::Message::Admin;

use base qw( BarnOwl::Message );

sub header       { return shift->{"header"}; }


1;
