use strict;
use warnings;

package BarnOwl::Message;

use BarnOwl::Message::Admin;
use BarnOwl::Message::AIM;
use BarnOwl::Message::Generic;
use BarnOwl::Message::Loopback;
use BarnOwl::Message::Zephyr;

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
sub unix_time   { return shift->{"unix_time"}; }
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
sub is_personal { return BarnOwl::message_matches_filter(shift, "personal"); }
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

# This is for back-compat with old messages that set these properties
# New protocol implementations are encourages to user override these
# methods.
sub replycmd         { return shift->{replycmd}};
sub replysendercmd   { return shift->{replysendercmd}};

sub pretty_sender    { return shift->sender; }
sub pretty_recipient { return shift->recipient; }

# Override if you want a context (instance, network, etc.) on personals
sub personal_context { return ""; }
# extra short version, for use where space is especially tight
# (eg, the oneline style)
sub short_personal_context { return ""; }

sub delete_and_expunge {
    my ($m) = @_;
    &BarnOwl::command("delete-and-expunge --quiet --id " . $m->id);
}

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

=head2 log MESSAGE

Returns the text that should be written to a file to log C<MESSAGE>.

=cut

sub log {
    my ($m) = @_;
    return $m->log_header . "\n\n" . $m->log_body . "\n\n";
}

=head2 log_header MESSAGE

Returns the header of the message, for logging purposes.
If you override L<BarnOwl::Message::log>, this method is not called.

=cut

sub log_header {
    my ($m) = @_;
    my $sender = $m->sender;
    my $recipient = $m->recipient;
    my $timestr = $m->time;
    return "From: <$sender> To: <$recipient>\n"
         . "Time: $timestr";
}

=head2 log_body MESSAGE

Returns the body of the message, for logging purposes.
If you override L<BarnOwl::Message::log>, this method is not called.

=cut

sub log_body {
    my ($m) = @_;
    if ($m->is_loginout) {
        return uc($m->login)
            . $m->login_type
            . ($m->login_extra ? ' at ' . $m->login_extra : '');
    } else {
        return $m->body;
    }
}

=head2 log_filenames MESSAGE

Returns a list of filenames to which this message should be logged.
The filenames should be relative to the path returned by
C<log_base_path>.  See L<BarnOwl::Logging::get_filenames> for the
specification of valid filenames, and for what happens if this
method returns an invalid filename.

=cut

sub log_filenames {
    my ($m) = @_;
    my $filename = lc($m->type);
    $filename = "unknown" if !defined $filename || $filename eq '';
    if ($m->is_incoming && $m->pretty_sender) {
        $filename .= ":" . $m->pretty_sender;
    } elsif ($m->is_outgoing && $m->pretty_recipient) {
        $filename .= ":" . $m->pretty_recipient;
    }
    return ($filename);
}

=head2 log_to_all_file MESSAGE

There is an C<all> file.  This method determines if C<MESSAGE>
should get logged to it, in addition to any files returned by
C<log_filenames>.

It defaults to returning true if and only if C<MESSAGE> is outgoing.

=cut

sub log_to_all_file {
    my ($m) = @_;
    return $m->is_outgoing;
}

=head2 log_base_path MESSAGE

Returns the base path for logging, the folder in which all messages
of this class get logged.

Defaults to the BarnOwl variable C<logpath>.

=cut

sub log_base_path {
    return BarnOwl::getvar('logpath');
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


1;
