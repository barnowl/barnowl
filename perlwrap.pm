# $Id$
#
# This is all linked into the binary and evaluated when perl starts up...
#
#####################################################################
#####################################################################

package owl;

# bootstrap in C bindings and glue
bootstrap owl 1.2;

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

# switch to package main when we're done
package main;

1;
