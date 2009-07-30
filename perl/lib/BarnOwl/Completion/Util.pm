use strict;
use warnings;

package BarnOwl::Completion::Util;

use base qw(Exporter);
our @EXPORT_OK = qw(complete_flags);

use Getopt::Long;

sub complete_flags {
    my $ctx     = shift;
    my $no_args = shift;
    my $args    = shift;
    my $default = shift;


    my $idx = 1;
    my $flag = undef;

    my $word = 0;
    my $optsdone = 0;

    while($idx < $ctx->word) {
        my $word = $ctx->words->[$idx];
        if($flag) {
            undef $flag;
        } elsif($word =~ m{^--}) {
            if($word eq '--') {
                $optsdone = 1;
                last;
            }
            $flag = $word if(exists $args->{$word});
        } elsif ($word =~ m{^-}) {
            $word = "-" . substr($word, -1);
            $flag = $word if(exists $args->{$word});
        }
        $idx++;
    }

    if($flag) {
        my $c = $args->{$flag};
        if($c) {
            return $c->($ctx);
        }
        return;
    } else {
        return ($optsdone ? () : (@$no_args, keys %$args),
                $default ? ($default->($ctx)) : ());
    }
}
