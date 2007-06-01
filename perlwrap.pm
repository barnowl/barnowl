# $Id$
#
# This is all linked into the binary and evaluated when perl starts up...
#
#####################################################################
#####################################################################
# XXX NOTE: This file is sourced before almost any barnowl
# architecture is loaded. This means, for example, that it cannot
# execute any owl commands. Any code that needs to do so should live
# in BarnOwl::Hooks::_startup

use strict;
use warnings;

package BarnOwl;

BEGIN {
# bootstrap in C bindings and glue
    *owl:: = \*BarnOwl::;
    bootstrap BarnOwl 1.2;
};

use lib(get_data_dir()."/lib");
use lib($ENV{HOME}."/.owl/lib");

our $configfile;

if(!$configfile && -f $ENV{HOME} . "/.barnowlconf") {
    $configfile = $ENV{HOME} . "/.barnowlconf";
}
$configfile ||= $ENV{HOME}."/.owlconf";

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
    return &BarnOwl::Hooks::_receive_msg($m);
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
        summary     => "",
        usage       => "",
        description => "",
        %{$args}
    );

    BarnOwl::new_command_internal($name, $func, $args{summary}, $args{usage}, $args{description});
}

=head2 new_variable_int NAME [{ARGS}]

=head2 new_variable_bool NAME [{ARGS}]

=head2 new_variable_string NAME [{ARGS}]

Add a new owl variable, either an int, a bool, or a string, with the
specified name.

ARGS can optionally contain the following keys:

=over 4

=item default

The default and initial value for the variable

=item summary

A one-line summary of the variable's purpose

=item description

A longer description of the function of the variable

=back

=cut

sub new_variable_int {
    unshift @_, \&BarnOwl::new_variable_int_internal, 0;
    goto \&_new_variable;
}

sub new_variable_bool {
    unshift @_, \&BarnOwl::new_variable_bool_internal, 0;
    goto \&_new_variable;
}

sub new_variable_string {
    unshift @_, \&BarnOwl::new_variable_string_internal, "";
    goto \&_new_variable;
}

sub _new_variable {
    my $func = shift;
    my $default_default = shift;
    my $name = shift;
    my $args = shift || {};
    my %args = (
        summary     => "",
        description => "",
        default     => $default_default,
        %{$args});
    $func->($name, $args{default}, $args{summary}, $args{description});
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
    die("smartfilter not supported for this message\n");
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

package BarnOwl::Message::Loopback;

use base qw( BarnOwl::Message );

# all loopback messages are private
sub is_private {
  return 1;
}

#####################################################################
#####################################################################

package BarnOwl::Message::AIM;

use base qw( BarnOwl::Message );

# all non-loginout AIM messages are private for now...
sub is_private {
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

package BarnOwl::Hook;

sub new {
    my $class = shift;
    return bless [], $class;
}

sub run {
    my $self = shift;
    my @args = @_;
    return map {$_->(@args)} @$self;
}

sub add {
    my $self = shift;
    my $func = shift;
    die("Not a coderef!") unless ref($func) eq 'CODE';
    push @$self, $func;
}

sub clear {
    my $self = shift;
    @$self = ();
}

package BarnOwl::Hooks;

use Exporter;

our @EXPORT_OK = qw($startup $shutdown
                    $receiveMessage $mainLoop
                    $getBuddyList);

our %EXPORT_TAGS = (all => [@EXPORT_OK]);

our $startup = BarnOwl::Hook->new;
our $shutdown = BarnOwl::Hook->new;
our $receiveMessage = BarnOwl::Hook->new;
our $mainLoop = BarnOwl::Hook->new;
our $getBuddyList = BarnOwl::Hook->new;

# Internal startup/shutdown routines called by the C code

sub _load_owlconf {
    # load the config  file
    if ( -r $BarnOwl::configfile ) {
        undef $@;
        package main;
        do $BarnOwl::configfile;
        die $@ if $@;
        package BarnOwl;
        if(*BarnOwl::format_msg{CODE}) {
            # if the config defines a legacy formatting function, add 'perl' as a style 
            BarnOwl::_create_style("perl", "BarnOwl::_format_msg_legacy_wrap",
                                   "User-defined perl style that calls BarnOwl::format_msg"
                                   . " with legacy global variable support");
            BarnOwl::set("-q default_style perl");
        }
    }
}

sub _startup {
    _load_owlconf();

    if(eval {require BarnOwl::ModuleLoader}) {
        eval {
            BarnOwl::ModuleLoader->load_all;
        };
        BarnOwl::error("$@") if $@;

    } else {
        BarnOwl::error("Can't load BarnOwl::ModuleLoader, loadable module support disabled:\n$@");
    }
    
    $startup->run(0);
    BarnOwl::startup() if *BarnOwl::startup{CODE};
}

sub _shutdown {
    $shutdown->run;
    
    BarnOwl::shutdown() if *BarnOwl::shutdown{CODE};
}

sub _receive_msg {
    my $m = shift;

    $receiveMessage->run($m);
    
    BarnOwl::receive_msg($m) if *BarnOwl::receive_msg{CODE};
}

sub _mainloop_hook {
    $mainLoop->run;
    BarnOwl::mainloop_hook() if *BarnOwl::mainloop_hook{CODE};
}

sub _get_blist {
    return join("\n", $getBuddyList->run);
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
        if(defined $m->subcontext) {
            $header .= ' / ' . $m->subcontext;
        }
        $header .= ' / @b{' . $m->pretty_sender . '}';
    }

    $header .= "  " . time_hhmm($m);
    my $sender = $m->long_sender;
    $sender =~ s/\n.*$//s;
    $header .= " " x (4 - ((length $header) % 4));
    $header .= "(" . $sender . '@color[default]' . ")";
    my $message = $header . "\n". indentBody($m);
    if($m->is_personal && $m->direction eq "in") {
        $message = BarnOwl::Style::boldify($message);
    }
    return $message;
}

sub indentBody($)
{
    my $m = shift;

    my $body = $m->body;
    if ($m->{should_wordwrap}) {
      $body = BarnOwl::wordwrap($body, BarnOwl::getnumcols()-8);
    }
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

# Shove a bunch of fake entries into @INC so modules can use or
# require them without choking
$::INC{$_} = 1 for (qw(BarnOwl.pm BarnOwl/Hooks.pm
                       BarnOwl/Message.pm BarnOwl/Style.pm));

1;

