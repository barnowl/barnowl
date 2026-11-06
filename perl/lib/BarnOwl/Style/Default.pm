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

sub maybe {
    my $x = shift;
    return defined($x) ? $x : "";
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
            $header = ucfirst $m->type . $personal_context . " from ";
            if(defined($m->auth) && ($m->auth ne "YES")) {
                $header .= "UNAUTH: ";
            }
            $header .= maybe($m->pretty_sender);
        }
    } else {
        $header = $self->humanize($m->context, 1);
        if(defined $m->subcontext) {
            $header .= ' / ' . $self->humanize($m->subcontext, 1);
        }
        $header .= ' / ';
        if(defined($m->auth) && ($m->auth ne "YES")) {
            $header .= "UNAUTH: ";
        }
        $header .= '@b{' . maybe($m->pretty_sender) . '}';
        if (defined($m->realm) && $m->realm ne BarnOwl::zephyr_getrealm()) {
            $header .= ' {' . $self->humanize($m->realm, 1) . '}';
        }
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
    my $sender = $self->humanize($m->long_sender, 1);
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

# Characters that are not control characters, but that draw nothing or that
# reorder the text around them, so that a sender could use them to make text
# look different from what it is: the bidirectional embeddings, overrides and
# isolates, the invisible mathematical operators and the deprecated format
# controls (U+2061-2064, U+206A-206F), the Unicode tag characters, the zero
# width space, the word joiner and the zero width no-break space (byte order
# mark).  [:print:] does not exclude these, so they would reach the terminal as
# they are.
#
# Other format characters are part of ordinary text and are left alone: a
# single zero width space between characters of Khmer, Thai, Lao or Myanmar
# (where it separates words), the zero width (non-)joiner and the
# left-to-right, right-to-left and Arabic letter marks (Persian, Indic and
# Arabic or Hebrew text), the soft hyphen, and the variation selectors (emoji
# sequences, Han variation sequences).
# Under /x whitespace inside a bracketed class is a member of the class, so
# the classes are joined by | and nothing is written inside the brackets but
# code points.
my $zwsp_script = qr/[\p{Khmer}\p{Thai}\p{Lao}\p{Myanmar}]/;
my $spoofing = qr/
  [\x{2060}-\x{2064}\x{206A}-\x{206F}\x{FEFF}]
  | [\x{202A}-\x{202E}\x{2066}-\x{2069}\x{E0000}-\x{E007F}]
  | (?<!$zwsp_script) \x{200B}
  | \x{200B} (?!$zwsp_script)
/x;

# The flags of England, Scotland and Wales are an emoji followed by tag
# characters, and are shown as they are; any other tag characters are not.
my $tag_flag = qr/
  \x{1F3F4} \x{E0067} \x{E0062}
  (?: \x{E0065}\x{E006E}\x{E0067}      # eng
    | \x{E0073}\x{E0063}\x{E0074}      # sct
    | \x{E0077}\x{E006C}\x{E0073} )    # wls
  \x{E007F}
/x;

# The first group is a flag to leave alone, the second a character to show.
# The lookahead skips printable ASCII, which is most of any text, without
# trying the alternatives; none of them can match it.
my $humanize_oneline =
  qr/(?=[^\x20-\x7E])(?:($tag_flag)|([[:cntrl:]]|$spoofing))/;
my $humanize_multiline =
  qr/(?=[^\x20-\x7E\n])(?:($tag_flag)|([^[:print:]\n]|[\r\cK\f]|$spoofing))/;
my $humanize_short =
  qr/(?=[^\x20-\x7E])(?:($tag_flag)|[[:cntrl:]]|$spoofing)/;

=head3 humanize STRING [one_line]

Method that takes a STRING with control characters and makes it human
readable in such a way as to not do anything funky with the terminal.
Characters that draw nothing or reorder text (bidirectional embeddings,
overrides and isolates, invisible mathematical operators, deprecated format
controls, Unicode tag characters, zero width space, word joiner and byte order
mark) are made visible in the same way.  If one_line is true, be more
conservative about what we treat as control character (a newline is one, and
only control characters and the characters above are shown).  A string of
bytes is treated as characters (Latin-1).

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

  # Strings from messages are decoded, but a string made of bytes would come
  # back as bytes, which the C code reads as invalid UTF-8.  Treat it as
  # characters (Latin-1).
  utf8::upgrade($s);

  my $chars = $oneline ? $humanize_oneline : $humanize_multiline;

  $s =~ s/$chars/
    defined($1) ? $1 : "\@b($colorize" . _humanize_char($2) . ')'/eg;

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
  # See humanize.
  utf8::upgrade($s);

  $s =~ s/$humanize_short/defined($1) ? $1 : '?'/eg;

  return $s;
}

1;
