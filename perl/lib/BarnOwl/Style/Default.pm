use strict;
use warnings;

package BarnOwl::Style::Default;
use POSIX qw(strftime);

################################################################################
# Branching point for various formatting functions in this style.
################################################################################
sub format_message
{
    my $self = shift;
    my $m    = shift;
    my $fmt;

    if ( $m->is_loginout) {
        $fmt = $self->format_login($m);
    } elsif($m->is_ping && $m->is_personal) {
        $fmt = $self->format_ping($m);
    } elsif($m->is_admin) {
        $fmt = $self->format_admin($m);
    } else {
        $fmt = $self->format_chat($m);
    }
    $fmt = BarnOwl::Style::boldify($fmt) if $self->should_bold($m);
    $fmt = $self->humanize($fmt);
    return $fmt;
}

sub should_bold {
    my $self = shift;
    my $m = shift;
    return $m->is_personal && $m->direction eq "in";
}

sub description {"Default style";}

BarnOwl::create_style("default", "BarnOwl::Style::Default");

################################################################################

sub format_time {
    my $self = shift;
    my $m = shift;
    my $dateformat = BarnOwl::time_format('get_time_format');
    return strftime($dateformat, localtime($m->unix_time));
}

sub format_login {
    my $self = shift;
    my $m = shift;
    return sprintf(
        '@b<%s%s> for @b(%s) (%s) %s',
        uc( $m->login ),
        $m->login_type,
        $m->pretty_sender,
        $m->login_extra,
        $self->format_time($m)
       );
}

sub format_ping {
    my $self = shift;
    my $m = shift;
    my $personal_context = $m->personal_context;
    $personal_context = ' [' . $personal_context . ']' if $personal_context;
    return "\@b(PING)" . $personal_context . " from \@b(" . $m->pretty_sender . ")";
}

sub format_admin {
    my $self = shift;
    my $m = shift;
    return "\@bold(OWL ADMIN)\n" . $self->indent_body($m);
}

sub format_chat {
    my $self = shift;
    my $m = shift;
    my $header = $self->chat_header($m);
    return $header . "\n". $self->indent_body($m);
}

sub chat_header {
    my $self = shift;
    my $m = shift;
    my $header;
    if ( $m->is_personal ) {
        my $personal_context = $m->personal_context;
        $personal_context = ' [' . $self->humanize($personal_context, 1) . ']' if $personal_context;

        if ( $m->direction eq "out" ) {
            $header = ucfirst $m->type . $personal_context . " sent to " . $m->pretty_recipient;
        } else {
            $header = ucfirst $m->type . $personal_context . " from " . $m->pretty_sender;
        }
    } else {
        $header = $self->humanize($m->context, 1);
        if(defined $m->subcontext) {
            $header .= ' / ' . $self->humanize($m->subcontext, 1);
        }
        $header .= ' / @b{' . $m->pretty_sender . '}';
    }

    if($m->opcode) {
        $header .= " [" . $self->humanize($m->opcode, 1) . "]";
    }
    $header .= "  " . $self->format_time($m);
    $header .= $self->format_sender($m);
    return $header;
}

sub format_sender {
    my $self = shift;
    my $m = shift;
    my $sender = $m->long_sender;
    $sender =~ s/\n.*$//s;
    if (BarnOwl::getvar('colorztext') eq 'on') {
      return "  (" . $sender . '@color[default]' . ")";
    } else {
      return "  ($sender)";
    }
}

sub indent_body
{
    my $self = shift;
    my $m = shift;

    my $body = $m->body;
    if ($m->{should_wordwrap}) {
      $body = BarnOwl::wordwrap($body, BarnOwl::getnumcols()-9);
    }
    # replace newline followed by anything with
    # newline plus four spaces and that thing.
    $body =~ s/\n(.)/\n    $1/g;
    # Trim trailing newlines.
    $body =~ s/\n*$//;
    return "    ".$body;
}

=head3 humanize STRING [one_line]

Method that takes a STRING with control characters and makes it human
readable in such a way as to not do anything funky with the terminal.
If one_line is true, be more conservative about what we treat as
control character.

=cut

sub humanize
{
  my $self = shift;
  my $s = shift;
  my $oneline = shift;
  sub _humanize_char
  {
    my $c = ord(shift);

    if ($c < ord(' ')) {
      return ('^' . chr($c + ord('@')));
    } elsif ($c == 255) {
      return ('^?');
    } else {
      return (sprintf('\\x{%x}', $c));
    }
  }
  my $colorize = (BarnOwl::getvar('colorztext') eq 'on')
    ? '@color(cyan)' : '';

  my $chars = $oneline ? qr/[[:cntrl:]]/ : qr/[^[:print:]\n]|[\r\cK\f]/;

  $s =~ s/($chars)/
    "\@b($colorize" . _humanize_char($1) . ')'/eg;

  return $s;
}

=head3 humanize_short STRING

As above, but always be conservative, and replace with a '?' instead
of something more elaborate.

=cut

sub humanize_short
{
  my $self = shift;
  my $s = shift;

  $s =~ s/[[:cntrl:]]/?/g;

  return $s;
}

1;
