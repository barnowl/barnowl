use strict;
use warnings;

package BarnOwl::Completion::Util;

use base qw(Exporter);
our @EXPORT_OK = qw(complete_flags complete_file);

use Getopt::Long;
use Cwd qw(abs_path);
use File::Basename qw(dirname basename);


sub complete_flags {
    my $ctx     = shift;
    my $no_args = shift;
    my $args    = shift;
    my $default = shift;

    my %options = ();
    %options = @_ if @_;

    my $idx = 1;
    my $flag = undef;

    my $argct = 0;
    my $optsdone = 0;

    my %flags_seen;

    while($idx < $ctx->word) {
        my $word = $ctx->words->[$idx];
        if($flag) {
            undef $flag;
        } elsif($word =~ m{^--}) {
            if($word eq '--') {
                $optsdone = 1;
                $idx++;
                last;
            }
            $flag = $word if(exists $args->{$word});
        } elsif ($word =~ m{^-}) {
            $word = "-" . substr($word, -1);
            $flags_seen{$word} = 1; # record flag
            $flag = $word if(exists $args->{$word});
        } else {
            $argct++;
            if ($options{stop_at_nonflag}) {
                $optsdone = 1;
                $idx++;
                last;
            }
        }
        $idx++;
    }
    # Account for any words we skipped
    $argct += $ctx->word - $idx;

    if($flag) {
        my $c = $args->{$flag};
        if($c) {
            return $c->($ctx);
        }
        return;
    } else {
        my @opts = $optsdone ? () : (@$no_args, keys %$args);
        # filter out flags we've seen if needbe
        @opts = grep {!$flags_seen{$_}} @opts unless $options{repeat_flags};
        return (@opts, $default ? ($default->($ctx, $argct)) : ());
    }
}

sub expand_tilde {
    # Taken from The Perl Cookbook, recipe 7.3
    my $path = shift;
    $path =~ s{ ^ ~ ( [^/]* ) }
                { $1
                  ? (getpwnam($1))[7]
                  : ( $ENV{HOME} || $ENV{LOGDIR}
                      || (getpwuid($>))[7]
                     )
              }ex;
    return $path;
}

sub splitfile {
    my $path = shift;
    if ($path =~ m{^(.*/)([^/]*)$}) {
        return ($1, $2);
    } else {
        return ('', $path);
    }
}

sub complete_file {
    my $string = shift;

    return ['~/', '~/', 0] if $string eq '~';

    my $path = abs_path(expand_tilde($string));
    my $dir;
    if ($string =~ m{/$} || $string eq '' || basename($string) eq '.') {
        $dir = $path;
    } else {
        $dir = dirname($path);
    }
    return unless -d $dir;

    my ($pfx, $base) = splitfile($string);
    
    opendir(my $dh, $dir) or return;
    my @dirs = readdir($dh);
    close($dh);

    my @out;
    for my $d (@dirs) {
        # Skip dotfiles unless explicitly requested
        if($d =~ m{^[.]} && $base !~ m{^[.]}) {
            next;
        }
        
        my ($text, $value, $done) = ($d, "${pfx}${d}", 1);

        if (-d "$dir/$d") {
            $text .= "/";
            $value .= "/";
            $done = 0;
        }
        push @out, [$text, $value, $done];
    }
    return @out;
}
