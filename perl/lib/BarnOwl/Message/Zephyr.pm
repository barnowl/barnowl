use strict;
use warnings;

package BarnOwl::Message::Zephyr;

use constant WEBZEPHYR_PRINCIPAL => "daemon.webzephyr";
use constant WEBZEPHYR_CLASS     => "webzephyr";
use constant WEBZEPHYR_OPCODE    => "webzephyr";

use base qw( BarnOwl::Message );

sub strip_realm {
    my $sender = shift;
    my $realm = BarnOwl::zephyr_getrealm();
    $sender =~ s/\@$realm$//;
    return $sender;
}

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
	    && $m->is_private);
}

sub is_mail {
    my ($m) = @_;
    return ((lc($m->class) eq "mail") && $m->is_private);
}

sub pretty_sender {
    my ($m) = @_;
    return strip_realm($m->sender);
}

sub pretty_recipient {
    my ($m) = @_;
    return strip_realm($m->recipient);
}

# Portion of the reply command that preserves the context
sub context_reply_cmd {
    my $m = shift;
    my $class = "";
    if (lc($m->class) ne "message") {
        $class = "-c " . BarnOwl::quote($m->class);
    }
    my $instance = "";
    if (lc($m->instance) ne "personal") {
        $instance = "-i " . BarnOwl::quote($m->instance);
    }
    if (($class eq "") or  ($instance eq "")) {
        return $class . $instance;
    } else {
        return $class . " " . $instance;
    }
}

sub personal_context {
    my ($m) = @_;
    return $m->context_reply_cmd();
}

sub short_personal_context {
    my ($m) = @_;
    if(lc($m->class) eq 'message')
    {
        if(lc($m->instance) eq 'personal')
        {
            return '';
        } else {
            return $m->instance;
        }
    } else {
        return $m->class;
    }
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

sub zephyr_cc {
    my $self = shift;
    return $1 if $self->body =~ /^\s*cc:\s+([^\n]+)/i;
    return undef;
}

sub replycmd {
    my $self = shift;
    my $sender = shift;
    $sender = 0 unless defined $sender;
    my ($class, $instance, $to, $cc);
    if($self->is_outgoing) {
        return $self->{zwriteline};
    }

    if($sender && $self->opcode eq WEBZEPHYR_OPCODE) {
        $class = WEBZEPHYR_CLASS;
        $instance = $self->pretty_sender;
        $instance =~ s/-webzephyr$//;
        $to = WEBZEPHYR_PRINCIPAL;
    } elsif($self->class eq WEBZEPHYR_CLASS
            && $self->is_loginout) {
        $class = WEBZEPHYR_CLASS;
        $instance = $self->instance;
        $to = WEBZEPHYR_PRINCIPAL;
    } elsif($self->is_loginout) {
        $class = 'MESSAGE';
        $instance = 'PERSONAL';
        $to = $self->sender;
    } elsif($sender && !$self->is_private) {
        # Possible future feature: (Optionally?) include the class and/or
        # instance of the message being replied to in the instance of the 
        # outgoing personal reply
        $class = 'MESSAGE';
        $instance = 'PERSONAL';
        $to = $self->sender;
    } else {
        $class = $self->class;
        $instance = $self->instance;
        $to = $self->recipient;
        $cc = $self->zephyr_cc();
        if($to eq '*' || $to eq '') {
            $to = '';
        } elsif($to !~ /^@/) {
            $to = $self->sender;
        }
    }

    my $cmd;
    if(lc $self->opcode eq 'crypt') {
        $cmd = 'zcrypt';
    } else {
        $cmd = 'zwrite';
    }

    my $context_part = $self->context_reply_cmd();
    $cmd .= " " . $context_part unless ($context_part eq '');
    if ($to ne '') {
        $to = strip_realm($to);
        if (defined $cc) {
            my @cc = grep /^[^-]/, ($to, split /\s+/, $cc);
            my %cc = map {$_ => 1} @cc;
            delete $cc{strip_realm(BarnOwl::zephyr_getsender())};
            @cc = keys %cc;
            $cmd .= " -C " . join(" ", @cc);
        } else {
            if(BarnOwl::getvar('smartstrip') eq 'on') {
                $to = BarnOwl::zephyr_smartstrip_user($to);
            }
            $cmd .= " $to";
        }
    }
    return $cmd;
}

sub replysendercmd {
    my $self = shift;
    return $self->replycmd(1);
}


1;
