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

    while($idx < $ctx->word) {
        my $word = $ctx->words->[$idx];
        BarnOwl::debug("[completing] idx=$idx word=$word ctx->word=@{[$ctx->word]}");
        if($flag) {
            undef $flag;
        } elsif($word =~ m{^--}) {
            last if $word eq '--';
            $flag = $word if(exists $args->{$word});
        } elsif ($word =~ m{^-}) {
            $word = "-" . substr($word, -1);
            $flag = $word if(exists $args->{$word});
        }
        $idx++;
    }

    if($flag) {
        BarnOwl::debug("END: flag=$flag");
        my $c = $args->{$flag};
        if($c) {
            return $c->($ctx);
        }
        return;
    } else {
        return (@$no_args,
                keys %$args,
                $default ? ($default->($ctx)) : ());
    }
}
