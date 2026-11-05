#!/usr/bin/env perl
use strict;
use warnings;

use Test::More;

use File::Basename;
use File::Path qw(make_path);
use File::Spec;
use File::Temp qw(tempdir);
BEGIN {require File::Spec->rel2abs("mock.pl", dirname($0));};

=head1 DESCRIPTION

BarnOwl::ModuleLoader loads PAR only for .par modules: loading PAR creates a
cache directory, and where that cannot be made it must not stop unpacked
modules from loading.

=cut

my $dir = tempdir(CLEANUP => 1);
my ($config, $data) = ("$dir/config", "$dir/data");

sub module {
    my ($root, $name) = @_;
    make_path("$root/lib/BarnOwl/Module");
    open(my $fh, '>', "$root/lib/BarnOwl/Module/$name.pm") or die "$root: $!";
    print $fh "package BarnOwl::Module::$name; our \$loaded = 1; 1;\n";
    close($fh);
}

module("$config/modules/Unpacked", 'Unpacked');
make_path("$data/modules");

my @errors;
{
    no warnings qw(redefine prototype);
    *BarnOwl::get_config_dir = sub { $config };
    *BarnOwl::get_data_dir = sub { $data };
    *BarnOwl::error = sub { push @errors, @_ };
}
require BarnOwl::Hooks;
require BarnOwl::ModuleLoader;

ok(!$INC{'PAR.pm'}, 'loading the module loader does not load PAR');
BarnOwl::ModuleLoader->load_all;
ok($BarnOwl::Module::Unpacked::loaded, 'an unpacked module loads');
ok(!$INC{'PAR.pm'}, 'PAR is not loaded without a .par module');
is_deeply(\@errors, [], 'no errors');

{
    # If PAR fails to load, an error is reported and unpacked modules still load
    my @fail_errors;
    local *BarnOwl::error = sub { push @fail_errors, @_ };
    open(my $fh, '>', "$data/modules/Broken.par") or die "$data: $!";
    close($fh);

    # Simulate PAR failing to load (e.g. sandbox unable to write /tmp)
    my $orig_inc = delete $INC{'PAR.pm'};
    my $hook = sub {
        my ($self, $file) = @_;
        die "unable to create cache directory\n" if $file =~ /^PAR/;
        return undef;
    };
    local @INC = ($hook, @INC);

    BarnOwl::ModuleLoader->load_all;
    ok($BarnOwl::Module::Unpacked::loaded, 'unpacked module still loads when PAR fails');
    is(scalar @fail_errors, 1, 'an error was reported when PAR failed to load');
    like($fail_errors[0] // '', qr/Unable to load PAR/, 'reported error mentions Unable to load PAR');

    unlink("$data/modules/Broken.par");
}

SKIP: {
    skip 'PAR and Archive::Zip are needed to build and test a .par file', 4
        unless eval { require Archive::Zip; require PAR; 1 };
    local $ENV{PAR_TMPDIR} = $dir;  # PAR's cache in our temporary directory, not /tmp
    module("$dir/par", 'Packed');
    my $zip = Archive::Zip->new;
    $zip->addTree("$dir/par/lib", 'lib');
    $zip->writeToFileNamed("$data/modules/Packed.par") == Archive::Zip::AZ_OK()
        or die "cannot write Packed.par";
    BarnOwl::ModuleLoader->load_all;
    ok($INC{'PAR.pm'}, 'a .par module loads PAR');
    ok($BarnOwl::Module::Packed::loaded, 'the .par module loads');
    is_deeply(\@errors, [], 'still no errors');
    # An unpacked module is found before anything inside a .par file, as with "use PAR".
    my ($unpacked) = grep { !ref($INC[$_]) && $INC[$_] eq "$config/modules/Unpacked/lib" } 0..$#INC;
    my ($hook) = grep { ref($INC[$_]) && $INC[$_] == \&PAR::find_par } 0..$#INC;
    ok(defined $unpacked && defined $hook && $unpacked < $hook,
       'unpacked module directories stay ahead of PAR in @INC');
}

done_testing;
