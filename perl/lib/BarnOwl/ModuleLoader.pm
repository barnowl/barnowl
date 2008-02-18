use strict;
use warnings;

package BarnOwl::ModuleLoader;

use lib (BarnOwl::get_data_dir() . "/modules/");
use PAR (BarnOwl::get_data_dir() . "/modules/*.par");
use PAR (BarnOwl::get_config_dir() . "/modules/*.par");

sub load_all {
    PAR::reload_libs();
    my %modules;
    my @modules;

    my @moddirs = ();
    push @moddirs, BarnOwl::get_data_dir() . "/modules";
    push @moddirs, BarnOwl::get_config_dir() . "/modules";
    
    for my $dir (@moddirs) {
        opendir(my $dh, $dir) or next;
        while(defined(my $f = readdir($dh))) {
            next if $f =~ /^\./;
            if(-f "$dir/$f" && $f =~ /^(.+)\.par$/) {
                $modules{$1} = 1;
            } elsif(-d "$dir/$f" && -d "$dir/$f/lib") {
                unshift @INC, "$dir/$f/lib" unless grep m{^$dir/$f/lib$}, @INC;
                $modules{$f} = 1;
            }
        }
        @modules = grep /\.par$/, readdir($dh);
        closedir($dh);
        for my $mod (@modules) {
            my ($class) = ($mod =~ /^(.+)\.par$/);
            $modules{$class} = 1;
        }
    }
    for my $class (keys %modules) {
        if(!defined eval "use BarnOwl::Module::$class") {
            # BarnOwl::error("Unable to load module $class: $!") if $!;
            BarnOwl::error("Unable to load module $class: $@") if $@;
        }
    }

    $BarnOwl::Hooks::startup->add(\&register_keybindings);
}

sub register_keybindings {
    BarnOwl::new_command('reload-modules', sub {BarnOwl::ModuleLoader->reload}, {
                           summary => 'Reload all modules',
                           usage   => 'reload-modules',
                           description => q{Reloads all modules located in ~/.owl/modules and the system modules directory}
                          });
}

sub reload {
    my $class = shift;
    for my $m (keys %INC) {
        delete $INC{$m} if $m =~ m{^BarnOwl/};
    }
    # Restore core modules from perlwrap.pm
    $INC{$_} = 1 for (qw(BarnOwl.pm BarnOwl/Hooks.pm
                         BarnOwl/Message.pm BarnOwl/Style.pm));

    $BarnOwl::Hooks::startup->clear;
    $BarnOwl::Hooks::getBuddyList->clear;
    $BarnOwl::Hooks::mainLoop->clear;
    $BarnOwl::Hooks::shutdown->clear;
    $BarnOwl::Hooks::receiveMessage->clear;
    local $SIG{__WARN__} = \&squelch_redefine;
    $class->load_all;
    $BarnOwl::Hooks::startup->run(1);
    BarnOwl::startup() if *BarnOwl::startup{CODE};
}

sub squelch_redefine {
    my $warning = shift;
    warn $warning unless $warning =~ /^Subroutine .+ redefined at/;
}

1;
