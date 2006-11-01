# $Id$
#
# This is all linked into the binary and evaluated when perl starts up...
#
#####################################################################
#####################################################################

package owl;


BEGIN {
# bootstrap in C bindings and glue
bootstrap owl 1.2;
};

use lib(get_data_dir()."/owl/lib");
use lib($::ENV{'HOME'}."/.owl/lib");


our $configfile = $::ENV{'HOME'}."/.owlconf";

# populate global variable space for legacy owlconf files 
sub _format_msg_legacy_wrap {
    my ($m) = @_;
    $m->legacy_populate_global();
    return &owl::format_msg($m);
}

# populate global variable space for legacy owlconf files 
sub _receive_msg_legacy_wrap {
    my ($m) = @_;
    $m->legacy_populate_global();
    return &owl::receive_msg($m);
}

# make owl::<command>("foo") be aliases to owl::command("<command> foo");
sub AUTOLOAD {
    my $called = $AUTOLOAD;
    $called =~ s/.*:://;
    $called =~ s/_/-/g;
    return &owl::command("$called ".join(" ",@_));
}

=head2 new_command NAME FUNC [{ARGS}]

Add a new owl command. When owl executes the command NAME, FUNC will
be called with the arguments passed to the command, with NAME as the
first argument.

ARGS should be a hashref containing any or all of C<summary>,
C<usage>, or C<description> keys.

=cut

sub new_command {
    my $name = shift;
    my $func = shift;
    my $args = shift || {};
    my %args = (
        summary     => undef,
        usage       => undef,
        description => undef,
        %{$args}
    );

    owl::new_command_internal($name, $func, $args{summary}, $args{usage}, $args{description});
}

#####################################################################
#####################################################################

package owl::Message;

sub new {
    my $class = shift;
    my %args = (@_);
    if($class eq __PACKAGE__ && $args{type}) {
        $class = "owl::Message::" . ucfirst $args{type};
    }
    return bless {%args}, $class;
}

sub type        { return shift->{"type"}; }
sub direction   { return shift->{"direction"}; }
sub time        { return shift->{"time"}; }
sub id          { return shift->{"id"}; }
sub body        { return shift->{"body"}; }
sub sender      { return shift->{"sender"}; }
sub recipient   { return shift->{"recipient"}; }
sub login       { return shift->{"login"}; }
sub is_private  { return shift->{"private"}; }

sub is_login    { return shift->login eq "login"; }
sub is_logout   { return shift->login eq "logout"; }
sub is_loginout { my $m=shift; return ($m->is_login or $m->is_logout); }
sub is_incoming { return (shift->{"direction"} eq "in"); }
sub is_outgoing { return (shift->{"direction"} eq "out"); }

sub is_deleted  { return shift->{"deleted"}; }

sub is_admin    { return (shift->{"type"} eq "admin"); }
sub is_generic  { return (shift->{"type"} eq "generic"); }
sub is_zephyr   { return (shift->{"type"} eq "zephyr"); }
sub is_aim      { return (shift->{"type"} eq "aim"); }
sub is_jabber   { return (shift->{"type"} eq "jabber"); }
sub is_icq      { return (shift->{"type"} eq "icq"); }
sub is_yahoo    { return (shift->{"type"} eq "yahoo"); }
sub is_msn      { return (shift->{"type"} eq "msn"); }
sub is_loopback { return (shift->{"type"} eq "loopback"); }

# These are overridden by appropriate message types
sub is_ping     { return 0; }
sub is_mail     { return 0; }
sub is_personal { return shift->is_private; }
sub class       { return undef; }
sub instance    { return undef; }
sub realm       { return undef; }
sub opcode      { return undef; }
sub header      { return undef; }
sub host        { return undef; }
sub hostname    { return undef; }
sub auth        { return undef; }
sub fields      { return undef; }
sub zsig        { return undef; }
sub zwriteline  { return undef; }
sub login_host  { return undef; }
sub login_tty   { return undef; }

sub pretty_sender { return shift->sender; }

sub delete {
    my ($m) = @_;
    &owl::command("delete --id ".$m->id);
}

sub undelete {
    my ($m) = @_;
    &owl::command("undelete --id ".$m->id);
}

# Serializes the message into something similar to the zwgc->vt format
sub serialize {
    my ($this) = @_;
    my $s;
    for my $f (keys %$this) {
	my $val = $this->{$f};
	if (ref($val) eq "ARRAY") {
	    for my $i (0..@$val-1) {
		my $aval;
		$aval = $val->[$i];
		$aval =~ s/\n/\n$f.$i: /g;
		$s .= "$f.$i: $aval\n";	   
	    }
	} else {
	    $val =~ s/\n/\n$f: /g;
	    $s .= "$f: $val\n";
	}
    }
    return $s;
}

# Populate the annoying legacy global variables
sub legacy_populate_global {
    my ($m) = @_;
    $owl::direction  = $m->direction ;
    $owl::type       = $m->type      ;
    $owl::id         = $m->id        ;
    $owl::class      = $m->class     ;
    $owl::instance   = $m->instance  ;
    $owl::recipient  = $m->recipient ;
    $owl::sender     = $m->sender    ;
    $owl::realm      = $m->realm     ;
    $owl::opcode     = $m->opcode    ;
    $owl::zsig       = $m->zsig      ;
    $owl::msg        = $m->body      ;
    $owl::time       = $m->time      ;
    $owl::host       = $m->host      ;
    $owl::login      = $m->login     ;
    $owl::auth       = $m->auth      ;
    if ($m->fields) {
	@owl::fields = @{$m->fields};
	@main::fields = @{$m->fields};
    } else {
	@owl::fields = undef;
	@main::fields = undef;
    }
}

#####################################################################
#####################################################################

package owl::Message::Admin;

@ISA = qw( owl::Message );

sub header       { return shift->{"header"}; }

#####################################################################
#####################################################################

package owl::Message::Generic;

@ISA = qw( owl::Message );

#####################################################################
#####################################################################

package owl::Message::AIM;

@ISA = qw( owl::Message );

# all non-loginout AIM messages are personal for now...
sub is_personal { 
    return !(shift->is_loginout);
}

#####################################################################
#####################################################################

package owl::Message::Zephyr;

@ISA = qw( owl::Message );

sub login_tty { 
    my ($m) = @_;
    return undef if (!$m->is_loginout);
    return $m->fields->[2];
}

sub login_host { 
    my ($m) = @_;
    return undef if (!$m->is_loginout);
    return $m->fields->[0];
}

sub zwriteline  { return shift->{"zwriteline"}; }

sub zsig        { return shift->{"zsig"}; }

sub is_ping     { return (lc(shift->opcode) eq "ping"); }

sub is_personal { 
    my ($m) = @_;
    return ((lc($m->class) eq "message")
	    && (lc($m->instance) eq "personal")
	    && $m->is_private);
}

sub is_mail { 
    my ($m) = @_;
    return ((lc($m->class) eq "mail") && $m->is_private);
}

sub pretty_sender {
    my ($m) = @_;
    my $sender = $m->sender;
    my $realm = owl::zephyr_getrealm();
    $sender =~ s/\@$realm$//;
    return $sender;
}

# These are arguably zephyr-specific
sub class       { return shift->{"class"}; }
sub instance    { return shift->{"instance"}; }
sub realm       { return shift->{"realm"}; }
sub opcode      { return shift->{"opcode"}; }
sub host        { return shift->{"hostname"}; }
sub hostname    { return shift->{"hostname"}; }
sub header      { return shift->{"header"}; }
sub auth        { return shift->{"auth"}; }
sub fields      { return shift->{"fields"}; }
sub zsig        { return shift->{"zsig"}; }

#####################################################################
#####################################################################

package owl::Message::Jabber;

@ISA = qw( owl::Message );

#####################################################################
#####################################################################
################################################################################
package owl;

# Arrays of function pointers to be called at specific times.
our @onStartSubs = ();
our @onReceiveMsg = undef;
our @onMainLoop = undef;
our @onGetBuddyList = undef;

################################################################################
# Mainloop hook and threading.
################################################################################

use threads;
use threads::shared;

# Shared thread shutdown flag.
# Consider adding a reload flag, so threads that should persist across reloads
# can distinguish the two events. We wouldn't want a reload to cause us to
# log out of and in to a perl-based IM session.
our $shutdown : shared;
$shutdown = 0;
our $reload : shared;
$reload = 0;

sub owl::mainloop_hook
{
    foreach (@onMainLoop)
    {
	&$_();
    }
    return;
}

################################################################################
# Startup and Shutdown code
################################################################################
sub owl::startup
{
# Modern versions of owl provides a great place to have startup stuff.
# Put things in ~/.owl/startup
    onStart();
}

sub owl::shutdown
{
# Modern versions of owl provides a great place to have shutdown stuff.
# Put things in ~/.owl/shutdown

# At this point I use owl::shutdown to tell any auxillary threads that they
# should terminate.
    $shutdown = 1;
    owl::mainloop_hook();
}

#Run this on start and reload. Adds modules and runs their startup functions.
sub onStart
{
    reload_init();
    #So that the user's .owlconf can have startsubs, we don't clear
    #onStartSubs; reload does however
    @onReceiveMsg = ();
    @onMainLoop = ();
    @onGetBuddyList = ();

    loadModules();
    foreach (@onStartSubs)
    {
	&$_();
    }
}
################################################################################
# Reload Code, taken from /afs/sipb/user/jdaniel/project/owl/perl
################################################################################
sub reload_hook (@) 
{
    
  @onStartSubs = ();
    onStart();
    return 1;
}

sub reload 
{
    # Shutdown existing threads.
    $reload = 1;
    owl::mainloop_hook();
    $reload = 0;
    @onMainLoop = ();
    
    # Do reload
    if (do "$ENV{HOME}/.owlconf" && reload_hook(@_))
    {
	return "owlconf reloaded";
    } 
    else
    {
        return "$ENV{HOME}/.owlconf load attempted, but error encountered:\n$@";
    }
}

sub reload_init () 
{
    owl::command('alias reload perl reload');
    owl::command('bindkey global "C-x C-r" command reload');
}

################################################################################
# Loads modules from ~/.owl/modules and owl's data directory
################################################################################

sub loadModules ()
{
my @modules;
foreach my $dir (owl::get_data_dir()."/owl/modules", $ENV{HOME}."/.owl/modules") {
  opendir(MODULES, $dir);
  # source ./modules/*.pl
  @modules  =  grep(/\.pl$/, readdir(MODULES));

foreach my $mod (@modules) {
  do "$dir/$mod";
}
closedir(MODULES);
}
}
sub queue_admin_msg
{
    my $err = shift;
    my $m = owl::Message->new(type => 'admin',
			      direction => 'none',
			      body => $err);
    owl::queue_message($m);
}


################################################################################
# Hooks into receive_msg()
################################################################################

sub owl::receive_msg
{
    my $m = shift;
    foreach (@onReceiveMsg)
    {
	&$_($m);
    }
}

################################################################################
# Hooks into get_blist()
################################################################################

sub owl::get_blist
{
    my $m = shift;
    foreach (@onGetBuddyList)
    {
	&$_($m);
    }
}

# switch to package main when we're done
package main;
# alias the hooks
foreach my $hook  qw (onStartSubs
onReceiveMsg
onMainLoop
onGetBuddyList ) {
  *{"main::".$hook} = \*{"owl::".$hook};
}

# load the config  file
if (-r $owl::configfile) {
do $owl::configfile or die $@;
}

1;
