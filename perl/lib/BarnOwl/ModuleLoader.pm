use strict;
use warnings;

package BarnOwl::ModuleLoader;

use lib (BarnOwl::get_data_dir() . "/modules/");
use PAR (BarnOwl::get_data_dir() . "/modules/*.par");
use PAR ($ENV{HOME} . "/.owl/modules/*.par");

sub load_all {
    my %modules;
    my @modules;
    
    for my $dir ( BarnOwl::get_data_dir() . "/modules",
                  $ENV{HOME} . "/.owl/modules" ) {
        opendir(my $dh, $dir) or next;
        @modules = grep /\.par$/, readdir($dh);
        closedir($dh);
        for my $mod (@modules) {
            my ($class) = ($mod =~ /^(.+)\.par$/);
            $modules{$class} = 1;
        }
    }
    for my $class (keys %modules) {
        if(defined eval "use BarnOwl::Module::$class") {
            BarnOwl::error("Unable to load module $class: $@") if $@;
        }
    }
}

1;
