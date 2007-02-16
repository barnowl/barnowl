# $Id$
#
# This is all linked into the binary and evaluated when perl starts up...
#
#####################################################################
#####################################################################
# XXX NOTE: This file is sourced before almost any barnowl
# architecture is loaded. This means, for example, that it cannot
# execute any owl commands. Any code that needs to do so, should
# create a function wrapping it and push it onto @onStartSubs


use strict;
use warnings;

package BarnOwl;


BEGIN {
# bootstrap in C bindings and glue
    *owl:: = \*BarnOwl::;
    bootstrap BarnOwl 1.2;
};

use lib(get_data_dir()."/lib");
use lib($::ENV{'HOME'}."/.owl/lib");

our $configfile;

$configfile ||= $::ENV{'HOME'}."/.owlconf";

# populate global variable space for legacy owlconf files 
sub _format_msg_legacy_wrap {
    my ($m) = @_;
    $m->legacy_populate_global();
    return &BarnOwl::format_msg($m);
}

# populate global variable space for legacy owlconf files 
sub _receive_msg_legacy_wrap {
    my ($m) = @_;
    $m->legacy_populate_global();
    return &BarnOwl::Hooks::receive_msg($m);
}

# make BarnOwl::<command>("foo") be aliases to BarnOwl::command("<command> foo");
sub AUTOLOAD {
    our $AUTOLOAD;
    my $called = $AUTOLOAD;
    $called =~ s/.*:://;
    $called =~ s/_/-/g;
    return &BarnOwl::command("$called ".join(" ",@_));
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

    no warnings 'uninitialized';
    BarnOwl::new_command_internal($name, $func, $args{summary}, $args{usage}, $args{description});
}

#####################################################################
#####################################################################

package BarnOwl::Message;

sub new {
    my $class = shift;
    my %args = (@_);
    if($class eq __PACKAGE__ && $args{type}) {
        $class = "BarnOwl::Message::" . ucfirst $args{type};
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
sub is_aim      { return (shift->{"type"} eq "AIM"); }
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

sub pretty_sender    { return shift->sender; }
sub pretty_recipient { return shift->recipient; }

sub delete {
    my ($m) = @_;
    &BarnOwl::command("delete --id ".$m->id);
}

sub undelete {
    my ($m) = @_;
    &BarnOwl::command("undelete --id ".$m->id);
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
    $BarnOwl::direction  = $m->direction ;
    $BarnOwl::type       = $m->type      ;
    $BarnOwl::id         = $m->id        ;
    $BarnOwl::class      = $m->class     ;
    $BarnOwl::instance   = $m->instance  ;
    $BarnOwl::recipient  = $m->recipient ;
    $BarnOwl::sender     = $m->sender    ;
    $BarnOwl::realm      = $m->realm     ;
    $BarnOwl::opcode     = $m->opcode    ;
    $BarnOwl::zsig       = $m->zsig      ;
    $BarnOwl::msg        = $m->body      ;
    $BarnOwl::time       = $m->time      ;
    $BarnOwl::host       = $m->host      ;
    $BarnOwl::login      = $m->login     ;
    $BarnOwl::auth       = $m->auth      ;
    if ($m->fields) {
	@BarnOwl::fields = @{$m->fields};
	@main::fields = @{$m->fields};
    } else {
	@BarnOwl::fields = undef;
	@main::fields = undef;
    }
}

sub smartfilter {
    die("smartfilter not supported for this message");
}

# Display fields -- overridden by subclasses when needed
sub login_type {""}
sub login_extra {""}
sub long_sender {""}

# The context in which a non-personal message was sent, e.g. a chat or
# class
sub context {""}

# Some indicator of context *within* $self->context. e.g. the zephyr
# instance
sub subcontext {""}

#####################################################################
#####################################################################

package BarnOwl::Message::Admin;

use base qw( BarnOwl::Message );

sub header       { return shift->{"header"}; }

#####################################################################
#####################################################################

package BarnOwl::Message::Generic;

use base qw( BarnOwl::Message );

#####################################################################
#####################################################################

package BarnOwl::Message::AIM;

use base qw( BarnOwl::Message );

# all non-loginout AIM messages are personal for now...
sub is_personal { 
    return !(shift->is_loginout);
}

#####################################################################
#####################################################################

package BarnOwl::Message::Zephyr;

use base qw( BarnOwl::Message );

sub login_type {
    return (shift->zsig eq "") ? "(PSEUDO)" : "";
}

sub login_extra {
    my $m = shift;
    return undef if (!$m->is_loginout);
    my $s = lc($m->host);
    $s .= " " . $m->login_tty if defined $m->login_tty;
    return $s;
}

sub long_sender {
    my $m = shift;
    return $m->zsig;
}

sub context {
    return shift->class;
}

sub subcontext {
    return shift->instance;
}

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
    my $realm = BarnOwl::zephyr_getrealm();
    $sender =~ s/\@$realm$//;
    return $sender;
}

sub pretty_recipient {
    my ($m) = @_;
    my $recip = $m->recipient;
    my $realm = BarnOwl::zephyr_getrealm();
    $recip =~ s/\@$realm$//;
    return $recip;
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
################################################################################
package BarnOwl;

################################################################################
# Mainloop hook
################################################################################

our $shutdown;
$shutdown = 0;
our $reload;
$reload = 0;

#Run this on start and reload. Adds modules
sub onStart
{
    reload_init();
    loadModules();
}
################################################################################
# Reload Code, taken from /afs/sipb/user/jdaniel/project/owl/perl
################################################################################
sub reload_hook (@)
{
    BarnOwl::Hooks::startup();
    return 1;
}

sub reload
{
    # Use $reload to tell modules that we're performing a reload.
  {
      local $reload = 1;
      BarnOwl::mainloop_hook() if *BarnOwl::mainloop_hook{CODE};
  }
    
  @BarnOwl::Hooks::onMainLoop = ();
  @BarnOwl::Hooks::onStartSubs = ();

  # Do reload
  package main;
  if (-r $BarnOwl::configfile) {
      undef $@;
      do $BarnOwl::configfile;
      BarnOwl::error("Error reloading $BarnOwl::configfile: $@") if $@;
  }
  BarnOwl::reload_hook(@_);
  package BarnOwl;
}

sub reload_init () 
{
    BarnOwl::command('alias reload perl BarnOwl::reload()');
    BarnOwl::command('bindkey global "C-x C-r" command reload');
}

################################################################################
# Loads modules from ~/.owl/modules and owl's data directory
################################################################################

sub loadModules () {
    my @modules;
    my $rv;
    foreach my $dir ( BarnOwl::get_data_dir() . "/modules",
                      $ENV{HOME} . "/.owl/modules" )
    {
        opendir( MODULES, $dir );

        # source ./modules/*.pl
        @modules = sort grep( /\.pl$/, readdir(MODULES) );

        foreach my $mod (@modules) {
            unless ($rv = do "$dir/$mod") {
                BarnOwl::error("Couldn't load $dir/$mod:\n $@") if $@;
                BarnOwl::error("Couldn't run $dir/$mod:\n $!") unless defined $rv;
            }
        }
        closedir(MODULES);
    }
}

sub _load_owlconf {
    # Only do this the first time
    return if $BarnOwl::reload;
    # load the config  file
    if ( -r $BarnOwl::configfile ) {
        undef $@;
        do $BarnOwl::configfile;
        die $@ if $@;
    }
}

push @BarnOwl::Hooks::onStartSubs, \&_load_owlconf;

package BarnOwl::Hooks;

# Arrays of subrefs to be called at specific times.
our @onStartSubs = ();
our @onReceiveMsg = ();
our @onMainLoop = ();
our @onGetBuddyList = ();

# Functions to call hook lists
sub runHook($@)
{
    my $hook = shift;
    my @args = @_;
    $_->(@args) for (@$hook);
}

sub runHook_accumulate($@)
{
    my $hook = shift;
    my @args = @_;
    return join("\n", map {$_->(@args)} @$hook);
}

################################################################################
# Startup and Shutdown code
################################################################################
sub startup
{
    # Modern versions of owl provides a great place to have startup stuff.
    # Put things in ~/.owl/startup

    #So that the user's .owlconf can have startsubs, we don't clear
    #onStartSubs; reload does however
    @onReceiveMsg = ();
    @onMainLoop = ();
    @onGetBuddyList = ();

    BarnOwl::onStart();

    runHook(\@onStartSubs);

    BarnOwl::startup() if *BarnOwl::startup{CODE};
}

sub shutdown
{
# Modern versions of owl provides a great place to have shutdown stuff.
# Put things in ~/.owl/shutdown

    # use $shutdown to tell modules that that's what we're doing.
    $BarnOwl::shutdown = 1;
    BarnOwl::mainloop_hook();

    BarnOwl::shutdown() if *BarnOwl::shutdown{CODE};
}

sub mainloop_hook
{
    runHook(\@onMainLoop);
    BarnOwl::mainloop_hook() if *BarnOwl::mainloop_hook{CODE};
}

################################################################################
# Hooks into receive_msg()
################################################################################

sub receive_msg
{
    my $m = shift;
    runHook(\@onReceiveMsg, $m);
    BarnOwl::receive_msg($m) if *BarnOwl::receive_msg{CODE};
}

################################################################################
# Hooks into get_blist()
################################################################################

sub get_blist
{
    return runHook_accumulate(\@onGetBuddyList);
}

################################################################################
# Built-in perl styles
################################################################################
package BarnOwl::Style::Default;
################################################################################
# Branching point for various formatting functions in this style.
################################################################################
sub format_message($)
{
    my $m = shift;

    if ( $m->is_loginout) {
        return format_login($m);
    } elsif($m->is_ping) {
        return ( "\@b(PING) from \@b(" . $m->pretty_sender . ")\n" );
    } elsif($m->is_admin) {
        return "\@bold(OWL ADMIN)\n" . indentBody($m);
    } else {
        return format_chat($m);
    }
}

BarnOwl::_create_style("default", "BarnOwl::Style::Default::format_message", "Default style");

################################################################################

sub time_hhmm {
    my $m = shift;
    my ($time) = $m->time =~ /(\d\d:\d\d)/;
    return $time;
}

sub format_login($) {
    my $m = shift;
    return sprintf(
        '@b<%s%s> for @b(%s) (%s) %s',
        uc( $m->login ),
        $m->login_type,
        $m->pretty_sender,
        $m->login_extra,
        time_hhmm($m)
       );
}

sub format_chat($) {
    my $m = shift;
    my $header;
    if ( $m->is_personal ) {
        if ( $m->direction eq "out" ) {
            $header = ucfirst $m->type . " sent to " . $m->pretty_recipient;
        } else {
            $header = ucfirst $m->type . " from " . $m->pretty_sender;
        }
    } else {
        $header = $m->context;
        if($m->subcontext) {
            $header .= " / " . $m->subcontext;
        }
        $header .= " / " . $m->pretty_sender;
    }

    $header .= "  " . time_hhmm($m);
    $header .= "\t(" . $m->long_sender . ")";
    my $message = $header . "\n". indentBody($m);
    if($m->is_private && $m->direction eq "in") {
        $message = BarnOwl::Style::boldify($message);
    }
    return $message;
}

sub indentBody($)
{
    my $m = shift;
    
    my $body = $m->body;
    # replace newline followed by anything with 
    # newline plus four spaces and that thing.
    $body =~ s/\n(.)/\n    $1/g;

    return "    ".$body;
}


package BarnOwl::Style;

# This takes a zephyr to be displayed and modifies it to be displayed
# entirely in bold.
sub boldify($)
{
    local $_ = shift;
    if ( !(/\)/) ) {
        return '@b(' . $_ . ')';
    } elsif ( !(/\>/) ) {
        return '@b<' . $_ . '>';
    } elsif ( !(/\}/) ) {
        return '@b{' . $_ . '}';
    } elsif ( !(/\]/) ) {
        return '@b[' . $_ . ']';
    } else {
        my $txt = "\@b($_";
        $txt =~ s/\)/\)\@b\[\)\]\@b\(/g;
        return $txt . ')';
    }
}


# switch to package main when we're done
package main;
# alias the hooks
{
    no strict 'refs';
    foreach my $hook  qw (onStartSubs
                          onReceiveMsg
                          onMainLoop
                          onGetBuddyList ) {
        *{"main::".$hook} = \*{"BarnOwl::Hooks::".$hook};
        *{"owl::".$hook} = \*{"BarnOwl::Hooks::".$hook};
    }
}

1;
