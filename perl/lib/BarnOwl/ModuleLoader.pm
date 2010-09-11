use strict;
use warnings;

package BarnOwl::ModuleLoader;

use PAR;

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

=h2 rescan_modules

Re-compute the list of available modules, and add the necessary items to @INC
and @PAR_INC.

We load modules from two directories, the system module dir, and the user module
directory. Modules can be in either of two forms: ${modname}.par, or else a
${modname}/ directory containing lib/${modname}.

We prefer to load modules from the user's directory, and if a module exists in
both packed and unpacked form in the same directory, we prefer the unpacked
module.

We walk the module directories in order of ascending priority -- user directory,
and then system directory.

When we walk the directories, we first check all things that are not named
Foo.par, add them to @INC, and add them to the module list. We then walk the
.par files and add them to @PAR_INC and update the module list.

It is important that we never add a module to @INC (or @PAR_INC) if we already
have a source for it, in order to get priorities right. The reason is that @INC
is processed before @PAR_INC, so if we had an unpacked system module and a
packed local module, if we added both, the system module would take priority.

=cut

sub rescan_modules {
    @PAR::PAR_INC = ();

    %modules = ();

    my @moddirs = ();
    push @moddirs, BarnOwl::get_config_dir() . "/modules";
    push @moddirs, BarnOwl::get_data_dir() . "/modules";

    for my $dir (@moddirs) {
        # Strip defunct entries from @INC
        @INC = grep {!/^\Q$dir/} @INC;

        opendir(my $dh, $dir) or next;
        my @ents = grep {!/^\./} readdir($dh);

        # Walk unpacked modules
        for my $f (grep {!/\.par$/} @ents) {
            if(-d "$dir/$f" && -d "$dir/$f/lib") {
                next if $modules{$f};

                unshift @INC, "$dir/$f/lib" unless grep m{^$dir/$f/lib$}, @INC;
                $modules{$f} = 1;
            }
        }

        # Walk parfiles
        for my $f (grep /\.par$/, @ents) {
            if(-f "$dir/$f" && $f =~ /^(.+)\.par$/) {
                next if $modules{$1};

                PAR->import("$dir/$f");
                $modules{$1} = 1;
            }
        }

        closedir($dh);
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
        $class->run_startup_hooks();
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
    $class->run_startup_hooks();
}

sub run_startup_hooks {
    my $class = shift;
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
