use strict;
use warnings;

package BarnOwl::ModuleLoader;

use lib (BarnOwl::get_data_dir() . "/modules/");
use PAR (BarnOwl::get_data_dir() . "/modules/*.par");
use PAR (BarnOwl::get_config_dir() . "/modules/*.par");

our %modules;

sub load_all {
    my $class = shift;
    $class->rescan_modules;
    PAR::reload_libs();

    for my $mod (keys %modules) {
        if(!defined eval "use BarnOwl::Module::$mod") {
            BarnOwl::error("Unable to load module $mod: \n$@\n") if $@;
        }
    }

    $BarnOwl::Hooks::startup->add(\&register_keybindings);
}

sub rescan_modules {
    PAR->import(BarnOwl::get_data_dir() . "/modules/*.par");
    PAR->import(BarnOwl::get_config_dir() . "/modules/*.par");
    my @modules;

    %modules = ();

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
}

sub reload_module {
    my $class = shift;
    my $module = shift;

    $class->rescan_modules();

    for my $m (keys %INC) {
        delete $INC{$m} if $m =~ m{^BarnOwl/Module/$module};
    }

    my $parfile;
    for my $p (@PAR::PAR_INC) {
        if($p =~ m/\Q$module\E[.]par$/) {
            $parfile = $p;
            last;
        }
    }

    local $SIG{__WARN__} = \&squelch_redefine;

    if(defined $parfile) {
        PAR::reload_libs($parfile);
    } elsif(!defined eval "use BarnOwl::Module::$module") {
        BarnOwl::error("Unable to load module $module: \n$@\n") if $@;
    }
}

sub reload_module_cmd {
    my $class = shift;
    shift; # Command
    my $module = shift;
    unless(defined($module)) {
        die("Usage: reload-module [MODULE]");
    };
    $class->reload_module($module);
}

sub register_keybindings {
    BarnOwl::new_command('reload-modules', sub {BarnOwl::ModuleLoader->reload}, {
                           summary => 'Reload all modules',
                           usage   => 'reload-modules',
                           description => q{Reloads all modules located in ~/.owl/modules and the system modules directory}
                          });
    BarnOwl::new_command('reload-module', sub {BarnOwl::ModuleLoader->reload_module_cmd(@_)}, {
                           summary => 'Reload one module',
                           usage   => 'reload-module [MODULE]',
                           description => q{Reloads a single module located in ~/.owl/modules or the system modules directory}
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
