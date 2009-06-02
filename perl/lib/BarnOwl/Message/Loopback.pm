use strict;
use warnings;

package BarnOwl::Message::Loopback;

use base qw( BarnOwl::Message );

# all loopback messages are private
sub is_private {
  return 1;
}

sub replycmd {return 'loopwrite';}
sub replysendercmd {return 'loopwrite';}


1;
